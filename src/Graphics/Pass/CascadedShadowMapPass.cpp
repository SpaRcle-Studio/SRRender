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

    void CascadedShadowMapPass::UpdateShaderDefines(SR_SRSL_NS::ShaderParams& params) const {
        if (m_instancing) {
            params.AddDefine("CASCADES_INSTANCING");
            if (IsFrustumCullingEnabled()) {
                params.AddDefine("CASCADES_FRUSTUM_CULLING");
            }
        }
        Super::UpdateShaderDefines(params);
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

    const Frustum& CascadedShadowMapPass::GetFrustum(uint32_t renderLayer, bool& isAvailable) const {
        if (renderLayer < m_lightFrustumCount && !m_lightFrustums.empty()) {
            return m_lightFrustums[renderLayer];
        }
        isAvailable = false;
        static const Frustum emptyFrustum;
        return emptyFrustum;
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

            // Стабилизация: снэп к сетке тексёлов относительно предыдущего кадра (как в UE5),
            // чтобы тени не "прыгали" при повороте камеры — движение только целыми тексёлами.
            const SR_MATH_NS::FVector3& lastCenterLS = m_lastCascadeData[i].center;
            centerLS.x = lastCenterLS.x + std::floor((centerLS.x - lastCenterLS.x) / texelSize) * texelSize;
            centerLS.y = lastCenterLS.y + std::floor((centerLS.y - lastCenterLS.y) / texelSize) * texelSize;
            m_lastCascadeData[i].center = centerLS;

            const SR_MATH_NS::FVector3 cascadeCenterWS = (lightViewTmp.Inverse() * SR_MATH_NS::FVector4(centerLS, 1.0f)).XYZ();
            SR_MATH_NS::FVector3 lightPos = cascadeCenterWS - lightDir * m_cascadeRadii[i] * 2.0f;
            auto&& lightView = SR_MATH_NS::Matrix4x4::LookAt(lightPos, cascadeCenterWS, SR_MATH_NS::FVector3::Up());

            float zMult = 100.f;
            const auto lightProj = SR_MATH_NS::Matrix4x4::Ortho(
                -m_cascadeRadii[i], m_cascadeRadii[i],
                m_cascadeRadii[i], -m_cascadeRadii[i],
                -m_cascadeRadii[i] - zMult, m_cascadeRadii[i] + zMult
            );

            m_cascadeMatrices[i] = lightProj * lightView;
        }

        for (uint32_t i = 0; i < m_cascadeCount; i++) {
            if (i < m_lightFrustumCount) {
                m_lightFrustums[i] = ExtractFrustum(m_cascadeMatrices[i]);
            }
        }
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