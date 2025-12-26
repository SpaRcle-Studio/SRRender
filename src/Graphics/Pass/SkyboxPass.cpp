//
// Created by Monika on 20.07.2022.
//

#include <Graphics/Pass/SkyboxPass.h>
#include <Graphics/Types/Skybox.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/Lighting/LightSystem.h>

#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/SkyboxPass.generated.hpp>

namespace SR_GRAPH_NS {
    SkyboxPass::~SkyboxPass() {
        if (m_skybox) {
            m_skybox->RemoveUsePoint();
        }
    }

    void SkyboxPass::SetSkybox(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        m_skyboxPath = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
        m_isSkyboxDirty = true;
        m_isRendered = false;

        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->SetDirty(true);
        }
    }

    void SkyboxPass::SetShader(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        m_shaderPath = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
        m_isShaderDirty = true;
        m_isRendered = false;

        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->SetDirty(true);
        }
    }

    bool SkyboxPass::Render() {
        m_isRendered = false;

        if (!UpdateParams()) {
            return false;
        }

        if (!m_skybox) {
            return false;
        }

        auto&& pShader = m_skybox->GetShader();
        if (!pShader) {
            return false;
        }

        if (pShader->Use() == ShaderBindResult::Failed) {
            return false;
        }

        m_isRendered = m_skybox->Draw();

        pShader->UnUse();

        return true;
    }

    void SkyboxPass::Update() {
        if (!m_skybox || !m_isRendered) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        SR_TRACY_ZONE;

        auto&& pShader = m_skybox->GetShader();
        auto&& pCamera = GetCamera();

        if (!pShader || !pShader->Ready() || !pCamera) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        GetPipeline()->SetCurrentShader(pShader.Get());

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        if (uboManager.BindNoDublicateUBO(m_skybox->GetVirtualUBO()) != Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("SkyboxPass::Update() : failed to bind UBO!");
            return;
        }

        SR_UNUSED_VARIABLE(pShader->Flush());

        if (pShader->BeginSharedUBO()) SR_LIKELY_ATTRIBUTE {
            pShader->SetMat4(SHADER_VIEW_NO_TRANSLATE_MATRIX, pCamera->GetView());
            pShader->SetMat4(SHADER_PROJECTION_MATRIX, pCamera->GetProjection());
            pShader->SetMat4(SHADER_PROJECTION_NO_FOV_MATRIX, pCamera->GetProjectionNoFOV());
            pShader->SetMat4(SHADER_VIEW_MATRIX, pCamera->GetViewTranslate());
            pShader->SetFloat(SHADER_TIME, static_cast<float_t>(SR_HTYPES_NS::Time::Instance().Clock()));
            pShader->SetVec3(SHADER_VIEW_POSITION, pCamera->GetPosition());
            pShader->SetVec3(SHADER_VIEW_DIRECTION, pCamera->GetViewDirection());
            pShader->SetMat4(SHADER_INVERSE_PROJECTION_MATRIX, pCamera->GetInverseProjection());
            pShader->SetMat4(SHADER_INVERSE_VIEW_MATRIX, pCamera->GetInverseViewTranslate());

            auto&& dirLightParams = GetRenderScene()->GetLightSystem()->GetDirectionalLightParams();

            pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_DIRECTION, dirLightParams.direction);
            pShader->SetVec3(SHADER_SUN_COLOR, dirLightParams.lightColor);
            pShader->SetVec3(SHADER_SKY_COLOR, dirLightParams.skyColor);
            pShader->SetVec3(SHADER_GROUND_COLOR, dirLightParams.groundColor);
            pShader->SetFloat(SHADER_SUN_INTENSITY, dirLightParams.intensity);
            pShader->SetFloat(SHADER_SHADOW_STRENGTH, dirLightParams.shadowStrength);
            pShader->SetFloat(SHADER_AMBIENT_INTENSITY, dirLightParams.ambientIntensity);

            pShader->EndSharedUBO();
        }
        else {
            SR_ERROR("SkyboxPass::Update() : failed to bind shared UBO!");
            return;
        }

        Super::Update();
    }

    bool SkyboxPass::UpdateParams() {
        if (m_isSkyboxDirty) {
            if (m_skybox) {
                m_skybox->RemoveUsePoint();
                m_skybox = nullptr;
            }

            if (!(m_skybox = m_skyboxPath.empty() ? SR_GTYPES_NS::Skybox::CreateEmpty(m_isQuad) : SR_GTYPES_NS::Skybox::Load(m_skyboxPath, m_isQuad))) {
                SR_ERROR("SkyboxPass::UpdateParams() : failed to load skybox!\n\tPath: {}", m_skyboxPath);
                return false;
            }
            else {
                m_skybox->AddUsePoint();
            }
            m_isSkyboxDirty = false;
        }

        if (m_skybox && m_isShaderDirty) {
            SR_SRSL_NS::ShaderMacrosParams shaderMacros;
            const uint32_t layers = GetColorLayersCount();
            for (uint32_t i = 0; i < layers; ++i) {
                shaderMacros.AddDefine(SR_SRSL_NS::SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[i]);
            }

            if (auto&& pShader = SR_GTYPES_NS::Shader::Load(m_shaderPath, shaderMacros)) {
                m_skybox->SetShader(pShader);
            }
            else {
                SR_ERROR("SkyboxPass::UpdateParams() : failed to load shader for skybox!\n\tPath: {}", m_shaderPath);
                return false;
            }
            m_isShaderDirty = false;
        }

        return true;
    }
}