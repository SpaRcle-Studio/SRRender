//
// Created by Monika on 06.06.2023.
//

#include <Graphics/Pass/CascadedShadowMapPass.h>

#include <Codegen/CascadedShadowMapPass.generated.hpp>

namespace SR_GRAPH_NS {
    void CascadedShadowMapPass::PostUpdate() {
        SR_TRACY_ZONE;

        /// обновляем строго в конце, чтобы не дергались тени
        if (auto&& pCamera = GetCamera()) {
            if (CheckCamera()) {
                m_directionalLightDirection = GetRenderScene()->GetLightSystem()->GetDirectionalLightDirection();
                m_cameraPosition = pCamera->GetPosition();
                m_cameraRotation = pCamera->GetRotation();
                m_screenSize = pCamera->GetSize();
                UpdateCascades();
            }
        }

        Super::PostUpdate();
    }

    void CascadedShadowMapPass::UpdateShaderDefines(SR_SRSL_NS::ShaderMacrosParams& defines) const {
        if (m_instancing) {
            defines.AddDefine("CASCADES_INSTANCING");
        }
        Super::UpdateShaderDefines(defines);
    }

    bool CascadedShadowMapPass::Init() {
        if (m_instancing && !GetPipeline()->IsShaderViewportIndexLayerSupported()) {
            SR_LOG("CascadedShadowMapPass::Init() : instancing is not supported! Falling back to non-instancing mode...");
            m_instancing = false;
            auto&& pFrameBufferController = GetTechnique()->GetFrameBufferController(GetPassName());
            if (pFrameBufferController) {
                pFrameBufferController->SetLayersCount(m_cascadeCount);
                pFrameBufferController->SetArrayLayersCount(1);
                SetRenderLayers(m_cascadeCount);
            }
            else {
                SR_ERROR("CascadedShadowMapPass::Init() : failed to find framebuffer controller \"{}\"!", GetPassName());
            }
        }

        return Super::Init();
    }

    bool CascadedShadowMapPass::Render() {
        SR_TRACY_ZONE;

        auto&& pPipeline = GetPipeline();

        if (auto&& pFrameBuffer = pPipeline->GetCurrentFrameBuffer()) {
            m_cascadeCount = m_instancing ? pFrameBuffer->GetArrayLayersCount() : pFrameBuffer->GetLayersCount();
        }

        pPipeline->SetDrawInstancesCount(m_instancing ? m_cascadeCount : 1);
        const bool result = Super::Render();
        pPipeline->ResetDrawInstancesCount();

        return result;
    }

    void CascadedShadowMapPass::Prepare() {
        SR_TRACY_ZONE;
        Super::Prepare();
    }

    void CascadedShadowMapPass::UpdateCascades() {
        SR_TRACY_ZONE;

        auto&& pCamera = GetCamera();
        if (!pCamera) {
            return;
        }

        std::vector<float_t> cascadeSplits;
        cascadeSplits.resize(m_cascadeCount);

        m_cascadeMatrices.resize(m_cascadeCount);
        m_cascadeSplitDepths.resize(m_cascadeCount);

        const float_t clipRange = m_far - m_near;

        const float_t minZ = m_near;
        const float_t maxZ = m_near + clipRange;

        const float_t range = maxZ - minZ;
        const float_t ratio = maxZ / minZ;

        for (uint32_t i = 0; i < m_cascadeCount; i++) {
            const float_t p = static_cast<float_t>(i + 1) / static_cast<float_t>(m_cascadeCount);
            const float_t log = minZ * std::pow(ratio, p);
            const float_t uniform = minZ + range * p;
            const float_t d = m_cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - m_near) / clipRange;
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
            radius = std::ceil(radius * 16.0f) / 16.0f;

            auto&& maxExtents = SR_MATH_NS::FVector3(radius);
            SR_MATH_NS::FVector3 minExtents = -maxExtents;

            SR_MATH_NS::Matrix4x4 lightViewMatrix = SR_MATH_NS::Matrix4x4::LookAt(frustumCenter + m_directionalLightDirection * -minExtents.z, frustumCenter, SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f));
            auto&& lightOrthoMatrix = SR_MATH_NS::Matrix4x4::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

            m_cascadeMatrices[i] = lightOrthoMatrix * lightViewMatrix;
            m_cascadeSplitDepths[i] = (m_near + splitDist * clipRange) * -1.0f;

            lastSplitDist = cascadeSplits[i];
        }
    }

    void CascadedShadowMapPass::UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader* pShader) {
        Super::UseUniformsFromAnotherPass(pShader);

        pShader->SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());
        pShader->SetValue<false>(SHADER_CASCADE_SPLITS, m_cascadeSplitDepths.data());
    }

    void CascadedShadowMapPass::UseConstants(SR_GTYPES_NS::Shader* pShader) {
        Super::UseConstants(pShader);

        pShader->SetConstInt(SHADER_PC_SHADOW_CASCADE_INDEX, GetPipeline()->GetCurrentFrameBufferLayer());
    }

    bool CascadedShadowMapPass::CheckCamera() {
        SR_TRACY_ZONE;

        auto&& pCamera = GetCamera();
        if (!pCamera) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        if (m_directionalLightDirection != GetRenderScene()->GetLightSystem()->GetDirectionalLightDirection()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_cameraPosition.Distance(pCamera->GetPosition()) > 1.0) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_cameraRotation != pCamera->GetRotation()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        if (m_screenSize != pCamera->GetSize()) SR_UNLIKELY_ATTRIBUTE {
            goto dirty;
        }

        return false;

    dirty:
        m_directionalLightDirection = GetRenderScene()->GetLightSystem()->GetDirectionalLightDirection();
        m_cameraPosition = pCamera->GetPosition();
        m_cameraRotation = pCamera->GetRotation();
        m_screenSize = pCamera->GetSize();

        return true;
    }

    void CascadedShadowMapPass::UseSharedUniforms(SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;

        Super::UseSharedUniforms(pShader);

        pShader->SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadeMatrices.data());

        pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_DIRECTION, m_directionalLightDirection);
    }
}