//
// Created by Monika on 20.07.2022.
//

#include <Graphics/Pass/SkyboxPass.h>
#include <Graphics/Types/Skybox.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GRAPH_NS {
    //SR_REGISTER_RENDER_PASS(SkyboxPass)

    SkyboxPass::~SkyboxPass() {
        if (m_skybox) {
            m_skybox->RemoveUsePoint();
        }
    }

    bool SkyboxPass::Load(const SR_XML_NS::Node &passNode) {
        SR_TRACY_ZONE;

        auto&& path = passNode.GetAttribute<SR_UTILS_NS::Path>();

        if (m_skybox) {
            m_skybox->RemoveUsePoint();
            m_skybox = nullptr;
        }

        if (!(m_skybox = SR_GTYPES_NS::Skybox::Load(path))) {
            SR_ERROR("SkyboxPass::Load() : failed to load skybox!\n\tPath: " + path.ToString());
            return false;
        }
        else {
            m_skybox->AddUsePoint();
        }

        if (m_skybox) {
            m_skybox->SetShader(SR_GTYPES_NS::Shader::Load(passNode.GetAttribute("Shader").ToString()));
        }

        return Super::Load(passNode);
    }

    bool SkyboxPass::Render() {
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

        m_skybox->Draw();

        pShader->UnUse();

        return true;
    }

    void SkyboxPass::Update() {
        if (!m_skybox) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        auto&& pShader = m_skybox->GetShader();

        if (!pShader || !pShader->Ready() || !m_camera) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        GetPassPipeline()->SetCurrentShader(pShader.Get());

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        if (uboManager.BindNoDublicateUBO(m_skybox->GetVirtualUBO()) != Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("SkyboxPass::Update() : failed to bind UBO!");
            return;
        }

        SR_UNUSED_VARIABLE(pShader->Flush());

        if (pShader->BeginSharedUBO()) SR_LIKELY_ATTRIBUTE {
            pShader->SetMat4(SHADER_VIEW_NO_TRANSLATE_MATRIX, m_camera->GetView());
            pShader->SetMat4(SHADER_PROJECTION_MATRIX, m_camera->GetProjection());
            pShader->SetMat4(SHADER_PROJECTION_NO_FOV_MATRIX, m_camera->GetProjectionNoFOV());
            pShader->SetFloat(SHADER_TIME, static_cast<float_t>(SR_HTYPES_NS::Time::Instance().Clock()));
            pShader->SetVec3(SHADER_VIEW_POSITION, m_camera->GetPosition());
            pShader->SetVec3(SHADER_VIEW_DIRECTION, m_camera->GetViewDirection());
            pShader->EndSharedUBO();
        }
        else {
            SR_ERROR("SkyboxPass::Update() : failed to bind shared UBO!");
            return;
        }

        BasePass::Update();
    }

    bool SkyboxPass::Init() {
        bool result = Super::Init();
        return result;
    }
}