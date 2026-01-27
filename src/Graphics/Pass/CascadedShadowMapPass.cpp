//
// Created by Monika on 06.06.2023.
//

#include <Graphics/Pass/CascadedShadowMapPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Lighting/LightSystem.h>

#include <Codegen/CascadedShadowMapPass.generated.hpp>

namespace SR_GRAPH_NS {
    void CascadedShadowMapPass::PostUpdate() {
        SR_TRACY_ZONE;

        /// обновляем строго в конце, чтобы не дергались тени, так как данные используются двумя проходами рендера
        if (auto&& pCamera = CheckCamera()) {
            m_directionalLightDirection = GetRenderScene()->GetLightSystem()->GetDirectionalLightParams().direction;
            m_cameraPosition = pCamera->GetPosition();
            m_cameraRotation = pCamera->GetRotation();
            m_screenSize = pCamera->GetSize();
            UpdateCascades(pCamera);
        }

        Super::PostUpdate();
    }

    SR_NODISCARD CascadedShadowMapPass::RenderQueuePtr CascadedShadowMapPass::AllocateRenderQueue(uint32_t index) {
        auto&& pQueue = Super::AllocateRenderQueue(index);
        //if (pQueue) {
        //    pQueue->SetFrustumCullingAllowed(index >= 2);
        //}
        return pQueue;
    }

    void CascadedShadowMapPass::UpdateShaderDefines(SR_SRSL_NS::ShaderMacrosParams& defines) const {
        if (m_instancing) {
            defines.AddDefine("CASCADES_INSTANCING");
            if (IsFrustumCullingEnabled()) {
                defines.AddDefine("CASCADES_FRUSTUM_CULLING");
            }
        }
        Super::UpdateShaderDefines(defines);
    }

    bool CascadedShadowMapPass::Init() {
        m_instancing &= GetPipeline()->IsShaderViewportIndexLayerSupported();

        uint32_t renderLayers = 0;

        if (auto&& pController = GetTechnique()->GetFrameBufferController(GetPassName())) {
            if (m_instancing) {
                pController->SetLayersCount(1);
                pController->SetArrayLayersCount(m_cascadeCount);

                if (IsFrustumCullingEnabled()) {
                    if (m_lightFrustumCount > m_cascadeCount) {
                        SR_WARN("CascadedShadowMapPass::Init() : light frustum count ({}) is greater than cascade count ({}). Setting light frustum count to cascade count.", m_lightFrustumCount, m_cascadeCount);
                        m_lightFrustumCount = m_cascadeCount;
                    }
                }
                else {
                    m_lightFrustumCount = 0;
                }

                renderLayers = SR_CLAMP(m_lightFrustumCount + 1u, 1u, m_cascadeCount);
            }
            else {
                pController->SetLayersCount(m_cascadeCount);
                pController->SetArrayLayersCount(1);
                renderLayers = m_cascadeCount;
            }
        }
        else {
            SR_ERROR("CascadedShadowMapPass::Init() : failed to find framebuffer controller \"{}\"!", GetPassName());
            return false;
        }

        SetRenderLayers(renderLayers);

        return Super::Init();
    }

    bool CascadedShadowMapPass::Render() {
        SR_TRACY_ZONE;

        auto&& pPipeline = GetPipeline();

        bool result = false;

        if (m_instancing) {
            const uint8_t renderLayers = GetLayersCount();
            for (uint8_t renderLayer = 0; renderLayer < renderLayers; ++renderLayer) {
                if (renderLayer >= m_lightFrustumCount) {
                    pPipeline->SetDrawInstancesCount(m_cascadeCount - (renderLayers - 1), renderLayers - 1);
                    m_drawCascadeIndex = -1;
                }
                else {
                    m_drawCascadeIndex = renderLayer;
                    pPipeline->ResetDrawInstancesCount();
                }

                auto&& pQueue = GetRenderQueue(renderLayer);
                result |= pQueue && pQueue->Render();

                pPipeline->ResetDrawInstancesCount();
            }
        }
        else {
            result = Super::Render();
        }

        return result;
    }

    void CascadedShadowMapPass::Update() {
        SR_TRACY_ZONE;

        if (m_instancing) {
            for (auto&& pQueue : GetRenderQueues()) {
                pQueue->Update();
            }
        }
        else {
            Super::Update();
        }
    }

    bool CascadedShadowMapPass::Prepare() {
        SR_TRACY_ZONE;
        return Super::Prepare();
    }

    const Frustum& CascadedShadowMapPass::GetFrustum(uint32_t renderLayer) const {
        if (renderLayer < m_lightFrustumCount && !m_lightFrustums.empty()) {
            return m_lightFrustums[renderLayer];
        }
        return Super::GetFrustum(renderLayer);
    }

    void CascadedShadowMapPass::OnCameraParamsChanged() {
        m_cameraDirty = true;
    }

    void CascadedShadowMapPass::UpdateCascadesUnityStyle(SR_GTYPES_NS::Camera* pCamera) {
        SR_TRACY_ZONE;

        m_cascadeMatrices.resize(m_cascadeCount);
        m_lightFrustums.resize(m_cascadeCount);
        m_cascadeRadii.resize(m_cascadeCount);

        m_lastCascadeData.resize(m_cascadeCount);

        const SR_MATH_NS::FVector3 ndcCorners[8] = {
            {-1,  1, 0}, { 1,  1, 0},
            { 1, -1, 0}, {-1, -1, 0},
            {-1,  1, 1}, { 1,  1, 1},
            { 1, -1, 1}, {-1, -1, 1},
        };

        auto&& invCamera = (pCamera->GetProjection() * pCamera->GetViewTranslate()).Inverse();

        const float_t maxShadowDistance = m_maxShadowDistance * m_oneMeterUnit;
        m_cascadeSplitDepths.resize(4);
        m_cascadeSplitDepths[0] = m_split1 * m_oneMeterUnit;
        m_cascadeSplitDepths[1] = m_split2 * m_oneMeterUnit;
        m_cascadeSplitDepths[2] = m_split3 * m_oneMeterUnit;
        m_cascadeSplitDepths[3] = maxShadowDistance;

        float_t shadowMapSize = 0.f;
        if (auto&& pPass = GetFrameBufferPass()) {
            if (auto&& pFBO = pPass->GetFrameBuffer()) {
                shadowMapSize = SR_MAX(static_cast<float_t>(pFBO->GetWidth()), static_cast<float_t>(pFBO->GetHeight()));
            }
        }

        if (shadowMapSize <= 0.f) {
            SR_ERROR("CascadedShadowMapPass::UpdateCascades() : invalid shadow map size!");
            return;
        }

        const float cameraFar = pCamera->GetFar();
        const SR_MATH_NS::FVector3 lightDir = m_directionalLightDirection.Normalized();
        auto lightView = SR_MATH_NS::Matrix4x4::FromTranslate(pCamera->GetPosition().InverseAxis(SR_MATH_NS::Axis::Y)) *
                SR_MATH_NS::Matrix4x4::LookAt(SR_MATH_NS::FVector3() - lightDir * 1000.0f, SR_MATH_NS::FVector3(), SR_MATH_NS::FVector3::Up());

        for (uint32_t i = 0; i < m_cascadeCount; ++i) {
            const float lastSplit = (i == 0) ? 0.0f : m_cascadeSplitDepths[i - 1];
            const float split     = m_cascadeSplitDepths[i];

            // 1. Восстанавливаем фрустум камеры в world-space
            SR_MATH_NS::FVector3 frustumCornersWS[8];
            for (uint32_t j = 0; j < 8; ++j) {
                SR_MATH_NS::FVector4 corner = invCamera * SR_MATH_NS::FVector4(ndcCorners[j], 1.0f);
                frustumCornersWS[j] = corner.XYZ() / corner.w;
            }

            // 2. Усечение по split
            SR_MATH_NS::FVector3 frustumCornersWS_near[4], frustumCornersWS_far[4];
            for (uint32_t j = 0; j < 4; ++j) {
                frustumCornersWS_near[j] = frustumCornersWS[j];      // near
                frustumCornersWS_far[j]  = frustumCornersWS[j + 4];  // far
            }

            for (uint32_t j = 0; j < 4; ++j) {
                frustumCornersWS[j]     = frustumCornersWS_near[j] + (frustumCornersWS_far[j] - frustumCornersWS_near[j]) * (lastSplit / cameraFar);
                frustumCornersWS[j + 4] = frustumCornersWS_near[j] + (frustumCornersWS_far[j] - frustumCornersWS_near[j]) * (split / cameraFar);
            }

            // 3. Bounding box в light-space
            SR_MATH_NS::FVector3 minLS( FLT_MAX,  FLT_MAX,  FLT_MAX);
            SR_MATH_NS::FVector3 maxLS(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            for (uint32_t j = 0; j < 8; ++j) {
                SR_MATH_NS::FVector4 ls = lightView * SR_MATH_NS::FVector4(frustumCornersWS[j], 1.0f);
                SR_MATH_NS::FVector3 p = ls.XYZ();
                minLS = SR_MATH_NS::Min(minLS, p);
                maxLS = SR_MATH_NS::Max(maxLS, p);
            }

            // 4. Bounding sphere
            SR_MATH_NS::FVector3 frustumCenter = (minLS + maxLS) * 0.5f;
            float_t forwardOffset = (lastSplit + split) * 0.5f; // вдоль взгляда камеры
            frustumCenter += pCamera->GetViewDirection() * forwardOffset;

            float radius = 0.0f;
            for (uint32_t j = 0; j < 8; ++j) {
                SR_MATH_NS::FVector3 p = (lightView * SR_MATH_NS::FVector4(frustumCornersWS[j], 1.0f)).XYZ();
                radius = glm::max(radius, (p - frustumCenter).Length());
            }

            // 5. Snap к texel grid
            const float shadowMapTexelSize = (2.0f * radius) / shadowMapSize;

            // Snap относительно прошлого центра
            frustumCenter.x = std::floor((frustumCenter.x - m_lastCascadeData[i].center.x) / shadowMapTexelSize) * shadowMapTexelSize + m_lastCascadeData[i].center.x;
            frustumCenter.y = std::floor((frustumCenter.y - m_lastCascadeData[i].center.y) / shadowMapTexelSize) * shadowMapTexelSize + m_lastCascadeData[i].center.y;

            // Радиус выравниваем по texel и не уменьшаем
            radius = glm::max(radius, m_lastCascadeData[i].radius);
            radius = std::ceil(radius / shadowMapTexelSize) * shadowMapTexelSize;

            m_lastCascadeData[i].center = frustumCenter;
            m_lastCascadeData[i].radius   = radius;
            m_cascadeRadii[i] = radius;

            const float_t zOffset = radius * 4.f;

            // 6. Ortho projection
            const auto lightProj = SR_MATH_NS::Matrix4x4::Ortho(
                -radius, radius, radius, -radius,
                -maxLS.z - zOffset, -minLS.z + zOffset
            );

            m_cascadeMatrices[i] = lightProj * lightView;
        }

        for (uint32_t i = 0; i < m_cascadeCount; i++) {
            if (i < m_lightFrustumCount) {
                m_lightFrustums[i] = ExtractFrustum(m_cascadeMatrices[i]);
            }
        }
    }

    void CascadedShadowMapPass::UpdateCascades(SR_GTYPES_NS::Camera* pCamera) {
        SR_TRACY_ZONE;

        m_cascadeMatrices.resize(m_cascadeCount);
        m_lightFrustums.resize(m_cascadeCount);
        m_cascadeRadii.resize(m_cascadeCount);
        m_lastCascadeData.resize(m_cascadeCount);

        static const SR_MATH_NS::FVector4 ndcCorners[8] = {
            {-1,  1, 0, 1}, { 1,  1, 0, 1},
            { 1, -1, 0, 1}, {-1, -1, 0, 1},
            {-1,  1, 1, 1}, { 1,  1, 1, 1},
            { 1, -1, 1, 1}, {-1, -1, 1, 1},
        };

        const float_t cameraFar = pCamera->GetFar();
        const float_t maxShadowDistance = m_maxShadowDistance * m_oneMeterUnit;
        const SR_MATH_NS::FVector3& cameraPos = pCamera->GetPosition();
        const SR_MATH_NS::FVector3 lightDir = m_directionalLightDirection.Normalized();
        const SR_MATH_NS::FVector3 viewDir = pCamera->GetViewDirection().Normalized();

        auto&& invCamera = (pCamera->GetProjection() * pCamera->GetViewTranslate()).Inverse();

        m_cascadeSplitDepths.resize(4);
        m_cascadeSplitDepths[0] = m_split1 * m_oneMeterUnit;
        m_cascadeSplitDepths[1] = m_split2 * m_oneMeterUnit;
        m_cascadeSplitDepths[2] = m_split3 * m_oneMeterUnit;
        m_cascadeSplitDepths[3] = maxShadowDistance;

        const float_t shadowMapSize = GetCascadedMapResolution();
        if (shadowMapSize <= 0.f) {
            return;
        }

        for (uint32_t i = 0; i < m_cascadeCount; ++i) {
            const float_t split  = m_cascadeSplitDepths[i];
            const float_t fovY   = SR_RAD(pCamera->GetFOV());
            const float_t aspect = pCamera->GetAspect();
            const float_t tanHalfFovY = std::tan(fovY * 0.5f);
            const float_t cascadeRadius = split * tanHalfFovY * std::sqrt(1.0f + aspect * aspect);
            m_cascadeRadii[i] = cascadeRadius * 1.05f;
        }

        for (uint32_t i = 0; i < m_cascadeCount; ++i) {
            const float_t split = m_cascadeSplitDepths[i];
            const float_t lastSplit = (i == 0) ? 0.0f : m_cascadeSplitDepths[i - 1];

            const float_t forwardOffset = 0.65f * (split + lastSplit);
            const SR_MATH_NS::FVector3 offset = viewDir * forwardOffset;

            const SR_MATH_NS::FVector3 cascadeCenterWSTmp = cameraPos + offset;

            const SR_MATH_NS::FVector3 lightPosTmp = cascadeCenterWSTmp - lightDir * m_cascadeRadii[i] * 2.0f;
            auto&& lightViewTmp  = SR_MATH_NS::Matrix4x4::LookAt(lightPosTmp, cascadeCenterWSTmp, SR_MATH_NS::FVector3::Up());

            const float_t texelSize = (m_cascadeRadii[i] * 2.0f) / shadowMapSize;
            SR_MATH_NS::FVector3 centerLS = (lightViewTmp * SR_MATH_NS::FVector4(cascadeCenterWSTmp, 1.0f)).XYZ();
            centerLS.x = std::floor(centerLS.x / texelSize) * texelSize;
            centerLS.y = std::floor(centerLS.y / texelSize) * texelSize;

            const SR_MATH_NS::FVector3 cascadeCenterWS = (lightViewTmp.Inverse() * SR_MATH_NS::FVector4(centerLS, 1.0f)).XYZ();
            SR_MATH_NS::FVector3 lightPos = cascadeCenterWS - lightDir * m_cascadeRadii[i] * 2.0f;
            auto&& lightView = SR_MATH_NS::Matrix4x4::LookAt(lightPos, cascadeCenterWS, SR_MATH_NS::FVector3::Up());

            float zMult = 100.f;
            const auto lightProj = SR_MATH_NS::Matrix4x4::Ortho(
                -m_cascadeRadii[i], m_cascadeRadii[i],
                m_cascadeRadii[i], -m_cascadeRadii[i],
                -m_cascadeRadii[i] - zMult, m_cascadeRadii[i] + zMult
            );


            /*const SR_MATH_NS::FVector3 cascadeCenterWS = cameraPos + offset;

            // Восстанавливаем углы фрустума в world space
            SR_MATH_NS::FVector3 frustumCornersWS[8];
            for (uint32_t j = 0; j < 8; ++j) {
                SR_MATH_NS::FVector4 corner = invCamera * ndcCorners[j];
                frustumCornersWS[j] = corner.XYZ() / corner.w;
            }

            // Линейное усечение фрустума по сплитам
            for (uint32_t j = 0; j < 4; ++j) {
                SR_MATH_NS::FVector3 dir = frustumCornersWS[j + 4] - frustumCornersWS[j];
                frustumCornersWS[j]     += dir * (lastSplit / cameraFar);
                frustumCornersWS[j + 4]  = frustumCornersWS[j] + dir * ((split - lastSplit) / cameraFar);
            }

            // Light view по смещенному центру
            const SR_MATH_NS::FVector3 lightPos = cascadeCenterWS - lightDir * 1000.0f;
            auto lightView = SR_MATH_NS::Matrix4x4::LookAt(lightPos, cascadeCenterWS, SR_MATH_NS::FVector3::Up());

            // Преобразуем углы фрустума в light space для AABB
            SR_MATH_NS::FVector3 minLS(FLT_MAX), maxLS(-FLT_MAX);
            for (auto& c : frustumCornersWS) {
                SR_MATH_NS::FVector3 p = (lightView * SR_MATH_NS::FVector4(c, 1.0f)).XYZ();
                minLS = SR_MATH_NS::Min(minLS, p);
                maxLS = SR_MATH_NS::Max(maxLS, p);
            }

            // Стабилизация центра по texel, используя m_cascadeRadii
            const float texelSize = (m_cascadeRadii[i] * 2.0f) / shadowMapSize;
            SR_MATH_NS::FVector3 center = (minLS + maxLS) * 0.5f;
            center.x = std::floor(center.x / texelSize) * texelSize;
            center.y = std::floor(center.y / texelSize) * texelSize;

            // Вычисляем extents относительно стабилизированного центра
            float extentX = glm::max(maxLS.x - center.x, center.x - minLS.x);
            float extentY = glm::max(maxLS.y - center.y, center.y - minLS.y);
            float extentZ = maxLS.z - minLS.z;

            // Новый центр для light space
            minLS.x = center.x - extentX;
            maxLS.x = center.x + extentX;
            minLS.y = center.y - extentY;
            maxLS.y = center.y + extentY;

            // Проекция Ortho (НЕ симметричная по Z)
            constexpr float zMult = 10.0f; // Unity-style запас
            auto lightProj = SR_MATH_NS::Matrix4x4::Ortho(
                    minLS.x, maxLS.x,
                    maxLS.y, minLS.y,
                    -maxLS.z - zMult, -minLS.z + zMult
            );*/



            /*SR_MATH_NS::FVector3 frustumCornersWS[8];
            for (uint32_t j = 0; j < 8; ++j) {
                const SR_MATH_NS::FVector4 corner = invCamera * ndcCorners[j];
                frustumCornersWS[j] = corner.XYZ() / corner.w;
            }
            // Линейное обрезание фрустума
            for (uint32_t j = 0; j < 4; ++j) {
                const SR_MATH_NS::FVector3 dir = frustumCornersWS[j + 4] - frustumCornersWS[j];
                frustumCornersWS[j]     = frustumCornersWS[j] + dir * (lastSplit / cameraFar);
                frustumCornersWS[j + 4] = frustumCornersWS[j] + dir * ((split - lastSplit) / cameraFar);
            }

            SR_MATH_NS::FVector3 minLS( FLT_MAX,  FLT_MAX,  FLT_MAX);
            SR_MATH_NS::FVector3 maxLS(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            for (auto& c : frustumCornersWS) {
                const SR_MATH_NS::FVector3 p = (lightView * SR_MATH_NS::FVector4(c, 1.0f)).XYZ();
                minLS = SR_MATH_NS::Min(minLS, p);
                maxLS = SR_MATH_NS::Max(maxLS, p);
            }

            // Стабилизация центра по texel
            SR_MATH_NS::FVector3 center = (minLS + maxLS) * 0.5f;
            const float_t extentX = maxLS.x - minLS.x;
            const float_t extentY = maxLS.y - minLS.y;
            const float_t texelSizeX = extentX / shadowMapSize;
            const float_t texelSizeY = extentY / shadowMapSize;
            center.x = std::floor(center.x / texelSizeX) * texelSizeX;
            center.y = std::floor(center.y / texelSizeY) * texelSizeY;

            minLS.x = center.x - extentX * 0.5f;
            maxLS.x = center.x + extentX * 0.5f;
            minLS.y = center.y - extentY * 0.5f;
            maxLS.y = center.y + extentY * 0.5f;

            constexpr float_t zMult = 100.0f;
            const auto lightProj = SR_MATH_NS::Matrix4x4::Ortho(minLS.x, maxLS.x, maxLS.y, minLS.y, -maxLS.z - zMult, -minLS.z + zMult);*/

            m_cascadeMatrices[i] = lightProj * lightView;
        }

        for (uint32_t i = 0; i < m_cascadeCount; i++) {
            if (i < m_lightFrustumCount) {
                m_lightFrustums[i] = ExtractFrustum(m_cascadeMatrices[i]);
            }
        }

        /*// Используем near/far из камеры для правильного распределения каскадов
        // Но ограничиваем far для теней (как в Unity - обычно 200-300 метров)
        const float_t cameraNear = pCamera->GetNear();
        const float_t cameraFar = pCamera->GetFar();
        const float_t nearPlane = (m_near > 0.0f) ? m_near : cameraNear;
        // Ограничиваем дальность теней для лучшего качества вблизи (как в Unity)
        const float_t maxShadowDistance = 300.0f; // максимальная дальность теней
        const float_t farPlane = (m_far > 0.0f) ? SR_MIN(m_far, maxShadowDistance) : SR_MIN(cameraFar, maxShadowDistance);

        const float_t clipRange = farPlane - nearPlane;

        const float_t minZ = nearPlane;
        const float_t maxZ = nearPlane + clipRange;

        const float_t range = maxZ - minZ;
        const float_t ratio = maxZ / minZ;

        for (uint32_t i = 0; i < m_cascadeCount; i++) {
            const float_t p = static_cast<float_t>(i + 1) / static_cast<float_t>(m_cascadeCount);
            const float_t log = minZ * std::pow(ratio, p);
            const float_t uniform = minZ + range * p;
            const float_t d = m_cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearPlane) / clipRange;
        }

        // Получаем разрешение shadow map для стабилизации
        uint32_t shadowMapResolution = 4096; // значение по умолчанию
        if (auto&& pController = GetTechnique()->GetFrameBufferController(GetPassName())) {
            if (auto&& pFramebuffer = pController->GetFramebuffer()) {
                shadowMapResolution = SR_MAX(pFramebuffer->GetWidth(), pFramebuffer->GetHeight());
            }
        }

        float_t lastSplitDist = 0.0;

        for (uint32_t i = 0; i < m_cascadeCount; i++) {
            const float_t splitDist = cascadeSplits[i];

            SR_MATH_NS::FVector3 frustumCorners[8] = {
                SR_MATH_NS::FVector3(-1.0f,  1.0f, -1.0f),
                SR_MATH_NS::FVector3( 1.0f,  1.0f, -1.0f),
                SR_MATH_NS::FVector3( 1.0f, -1.0f, -1.0f),
                SR_MATH_NS::FVector3(-1.0f, -1.0f, -1.0f),
                SR_MATH_NS::FVector3(-1.0f,  1.0f,  1.0f),
                SR_MATH_NS::FVector3( 1.0f,  1.0f,  1.0f),
                SR_MATH_NS::FVector3( 1.0f, -1.0f,  1.0f),
                SR_MATH_NS::FVector3(-1.0f, -1.0f,  1.0f),
            };

            auto&& invCamera = (pCamera->GetProjection() * pCamera->GetViewTranslate()).Inverse();

            for (auto&& frustumCorner : frustumCorners) {
                SR_MATH_NS::FVector4 invCorner = invCamera * SR_MATH_NS::FVector4(frustumCorner, 1.0f);
                frustumCorner = (invCorner / invCorner.w).XYZ();
            }

            for (uint32_t j = 0; j < 4; j++) {
                SR_MATH_NS::FVector3 dist = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
            }

            auto&& frustumCenter = SR_MATH_NS::FVector3(0.0f);
            for (auto&& frustumCorner : frustumCorners) {
                frustumCenter += frustumCorner;
            }
            frustumCenter /= 8.0f;

            float_t radius = 0.0f;
            for (auto&& frustumCorner : frustumCorners) {
                float_t distance = (frustumCorner - frustumCenter).Length();
                radius = SR_MAX(radius, distance);
            }
            // Улучшенное snapping: используем более точное значение для ближних каскадов
            // Ближние каскады требуют более точного snapping для качества
            const float_t snapValue = (i == 0) ? 32.0f : (i == 1) ? 16.0f : 8.0f;
            radius = std::ceil(radius * snapValue) / snapValue;

            auto&& maxExtents = SR_MATH_NS::FVector3(radius);
            SR_MATH_NS::FVector3 minExtents = -maxExtents;

            // Вычисляем lightViewMatrix
            SR_MATH_NS::Matrix4x4 lightViewMatrix = SR_MATH_NS::Matrix4x4::LookAt(
                frustumCenter + m_directionalLightDirection * -minExtents.z, 
                frustumCenter, 
                SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f)
            );

            // Преобразуем все frustum corners в пространство света
            SR_MATH_NS::FVector3 lightSpaceMin = SR_MATH_NS::FVector3(SR_FLOAT_MAX);
            SR_MATH_NS::FVector3 lightSpaceMax = SR_MATH_NS::FVector3(-SR_FLOAT_MAX);

            for (auto&& frustumCorner : frustumCorners) {
                SR_MATH_NS::FVector4 lightSpaceCorner = lightViewMatrix * SR_MATH_NS::FVector4(frustumCorner, 1.0f);
                SR_MATH_NS::FVector3 lightSpaceCorner3 = lightSpaceCorner.XYZ() / lightSpaceCorner.w;
                
                lightSpaceMin.x = SR_MIN(lightSpaceMin.x, lightSpaceCorner3.x);
                lightSpaceMin.y = SR_MIN(lightSpaceMin.y, lightSpaceCorner3.y);
                lightSpaceMin.z = SR_MIN(lightSpaceMin.z, lightSpaceCorner3.z);
                
                lightSpaceMax.x = SR_MAX(lightSpaceMax.x, lightSpaceCorner3.x);
                lightSpaceMax.y = SR_MAX(lightSpaceMax.y, lightSpaceCorner3.y);
                lightSpaceMax.z = SR_MAX(lightSpaceMax.z, lightSpaceCorner3.z);
            }

            // Вычисляем размер texel в пространстве света для стабилизации
            const float_t worldUnitsPerTexel = (lightSpaceMax.x - lightSpaceMin.x) / static_cast<float_t>(shadowMapResolution);

            // Стабилизация: снапнем min/max к texel grid в пространстве света
            // Это предотвращает дрожание теней при движении камеры
            // Используем floor для min и ceil для max, чтобы гарантировать покрытие
            // Снапнем к большему значению (2-4 texel) для более стабильной стабилизации
            const float_t snapScale = 2.0f; // Снапнем к 2 texel для большей стабильности
            const float_t snappedTexelSize = worldUnitsPerTexel * snapScale;
            
            lightSpaceMin.x = std::floor(lightSpaceMin.x / snappedTexelSize) * snappedTexelSize;
            lightSpaceMin.y = std::floor(lightSpaceMin.y / snappedTexelSize) * snappedTexelSize;
            lightSpaceMax.x = std::ceil(lightSpaceMax.x / snappedTexelSize) * snappedTexelSize;
            lightSpaceMax.y = std::ceil(lightSpaceMax.y / snappedTexelSize) * snappedTexelSize;

            // Используем стабилизированные extents для ortho матрицы
            // Это стабилизирует матрицу и предотвращает дрожание
            // Проверяем порядок параметров: left, right, bottom, top
            // В OpenGL/Vulkan bottom обычно меньше top
            // Если каскады перевернуты, возможно нужно инвертировать Y
            auto&& lightOrthoMatrix = SR_MATH_NS::Matrix4x4::Ortho(
                lightSpaceMin.x, lightSpaceMax.x, 
                lightSpaceMin.y, lightSpaceMax.y, 
                0.0f, lightSpaceMax.z - lightSpaceMin.z
            );

            m_cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;
            m_cascadeSplitDepths[i] = (nearPlane + splitDist * clipRange) * -1.0f;

            if (i < m_lightFrustumCount) {
                m_lightFrustums[i] = ExtractFrustum(m_cascadeMatrices[i]);
            }

            lastSplitDist = cascadeSplits[i];
        }*/
    }

    void CascadedShadowMapPass::UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader& shader) {
        Super::UseUniformsFromAnotherPass(shader);

        shader.SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());
        shader.SetValue<false>(SHADER_CASCADE_SPLITS, m_cascadeSplitDepths.data());
        shader.SetValue<false>(SHADER_CASCADE_RADII, m_cascadeRadii.data());
    }

    void CascadedShadowMapPass::UseConstants(SR_GTYPES_NS::Shader& shader) {
        Super::UseConstants(shader);

        if (m_instancing) {
            shader.SetConstInt(SHADER_PC_SHADOW_CASCADE_INDEX, m_drawCascadeIndex);
        }
        else {
            shader.SetConstInt(SHADER_PC_SHADOW_CASCADE_INDEX, GetPipeline()->GetCurrentFrameBufferLayer());
        }
    }

    SR_GTYPES_NS::Camera* CascadedShadowMapPass::CheckCamera() {
        SR_TRACY_ZONE;

        auto&& pCamera = GetCamera();
        if (!pCamera) SR_UNLIKELY_ATTRIBUTE {
            return nullptr;
        }

        if (m_cameraDirty) SR_UNLIKELY_ATTRIBUTE {
            m_cameraDirty = false;
            goto dirty;
        }

        if (m_directionalLightDirection != GetRenderScene()->GetLightSystem()->GetDirectionalLightParams().direction) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        //if (m_cameraPosition.Distance(pCamera->GetPosition()) > 1.0) SR_UNLIKELY_ATTRIBUTE {

        if (m_cameraPosition != pCamera->GetPosition()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_cameraRotation != pCamera->GetRotation()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_screenSize != pCamera->GetSize()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        return nullptr;

    dirty:
        m_directionalLightDirection = GetRenderScene()->GetLightSystem()->GetDirectionalLightParams().direction;
        m_cameraPosition = pCamera->GetPosition();
        m_cameraRotation = pCamera->GetRotation();
        m_screenSize = pCamera->GetSize();

        return pCamera;
    }

    void CascadedShadowMapPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;

        Super::UseSharedUniforms(shader);

        shader.SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());
        shader.SetVec3(SHADER_DIRECTIONAL_LIGHT_DIRECTION, m_directionalLightDirection);
    }

    float_t CascadedShadowMapPass::GetCascadedMapResolution() const {
        float_t shadowMapSize = 0.f;
        if (auto&& pPass = GetFrameBufferPass()) {
            if (auto&& pFBO = pPass->GetFrameBuffer()) {
                shadowMapSize = SR_MAX(static_cast<float_t>(pFBO->GetWidth()), static_cast<float_t>(pFBO->GetHeight()));
            }
        }

        if (shadowMapSize <= 0.f) {
            SR_ERROR("CascadedShadowMapPass::UpdateCascades() : invalid shadow map size!");
            return 0.f;
        }
        return shadowMapSize;
    }
}