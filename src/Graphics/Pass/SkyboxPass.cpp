//
// Created by Monika on 20.07.2022.
//

#include <Graphics/Pass/SkyboxPass.h>
#include <Graphics/Types/Skybox.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Pipeline/IShaderProgram.h>

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

        if (m_skybox) {
            m_skybox->RemoveUsePoint();
            m_skybox = nullptr;
        }

        if (!(m_skybox = SR_GTYPES_NS::Skybox::Load(m_skyboxPath))) {
            SR_ERROR("SkyboxPass::Load() : failed to load skybox!\n\tPath: {}", m_skyboxPath);
            return;
        }
        else {
            m_skybox->AddUsePoint();
        }

        if (m_skybox && !m_shaderPath.empty()) {
            m_skybox->SetShader(SR_GTYPES_NS::Shader::Load(m_shaderPath));
        }
    }

    void SkyboxPass::SetShader(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        m_shaderPath = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

        if (m_skybox) {
            m_skybox->SetShader(SR_GTYPES_NS::Shader::Load(m_shaderPath));
        }
    }

    bool SkyboxPass::Render() {
        m_isRendered = false;

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
            pShader->SetFloat(SHADER_TIME, static_cast<float_t>(SR_HTYPES_NS::Time::Instance().Clock()));
            pShader->SetVec3(SHADER_VIEW_POSITION, pCamera->GetPosition());
            pShader->SetVec3(SHADER_VIEW_DIRECTION, pCamera->GetViewDirection());
            pShader->EndSharedUBO();
        }
        else {
            SR_ERROR("SkyboxPass::Update() : failed to bind shared UBO!");
            return;
        }

        Super::Update();
    }
}