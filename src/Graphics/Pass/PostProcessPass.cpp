//
// Created by Monika on 07.08.2022.
//

#include <Graphics/Pass/PostProcessPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/PostProcessPass.generated.hpp>

namespace SR_GRAPH_NS {
    PostProcessPass::~PostProcessPass() {
        m_onShaderReloaded.Reset();
        m_material.Reset();
    }

    bool PostProcessPass::Init() {
        SR_TRACY_ZONE;

        //SR_SRSL_NS::ShaderMacrosParams shaderMacros;
        //auto&& macros = GetRenderContext()->GetShaderMacros();
        //for (auto&& [key, value] : macros) {
        //    shaderMacros.SetParam(key, value);
        //}

        //if (auto&& pShader = m_material ? m_material->GetShader(shaderMacros) : nullptr) {
        //    m_onShaderReloaded = pShader->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](auto &&) {
        //        m_dirtyShader = true;
        //    });
        //}
        m_dirtyShader = true;

        return Super::Init();
    }

    bool PostProcessPass::PreRender() {
        return Super::PreRender();
    }

    bool PostProcessPass::Render() {
        SR_TRACY_ZONE;

        if (!m_material) {
            return false;
        }

        m_shaderParams.Clear();
        auto&& macros = GetRenderContext()->GetShaderMacros();
        for (auto&& [key, value] : macros) {
            m_shaderParams.SetParam(key, value);
        }

        SR_GTYPES_NS::Shader* pShader = m_material->GetShader(m_shaderParams);
        if (!pShader || pShader->Use() == ShaderBindResult::Failed) {
            return false;
        }

        if (m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                pShader->UnUse();
                return false;
            }

            m_virtualDescriptor = DescriptorManager::Instance().AllocateDescriptorSet(m_virtualDescriptor);
        }

        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        for (SR_UTILS_NS::StringAtom passName : m_useSSBOFromPasses) {
            if (auto&& pPass = GetTechnique()->FindPass(passName)) {
                pPass->UseSSBOFromAnotherPass(*pShader);
            }
            else {
                SR_WARN("PostProcessPass::Render() : pass not found for SSBO binding!\n\tPass name: {}", passName);
            }
        }

        if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            UseSamplers(*pShader);
            m_descriptorManager.Flush();
        }
        GetPipeline()->GetCurrentShader()->FlushConstants();

        if (result != DescriptorManager::BindResult::Failed) {
            GetPipeline()->Draw(m_vertices);
        }

        pShader->UnUse();

        m_dirtyShader = false;

        return true;
    }

    void PostProcessPass::Update() {
        if (m_virtualUBO == SR_ID_INVALID || !m_material) {
            return;
        }

        SR_TRACY_ZONE;

        SR_GTYPES_NS::Shader* pShader = m_material->GetShader(m_shaderParams);

        GetPipeline()->SetCurrentShader(pShader);

        if (pShader && pShader->BeginSharedUBO()) {
            SR_MATH_NS::FVector2 resolution;
            if (auto&& pCamera = GetRenderScene()->GetMainCamera()) {
                resolution = pCamera->GetSize().Cast<float_t>();
            }
            else {
                resolution = GetRenderScene()->GetSurfaceSize().Cast<float_t>();
            }

            auto&& pLightSystem = GetRenderScene()->GetLightSystem();

            pShader->SetVec2(SHADER_RESOLUTION, resolution);

            pShader->SetFloat(SHADER_TIME, static_cast<float_t>(SR_HTYPES_NS::Time::Instance().Clock()));

            if (auto&& pCamera = GetCamera()) {
                pShader->SetVec3(SHADER_VIEW_POSITION, pCamera->GetPosition());
                pShader->SetVec3(SHADER_VIEW_DIRECTION, pCamera->GetViewDirection());
                pShader->SetMat4(SHADER_PROJECTION_MATRIX, pCamera->GetProjection());
                pShader->SetMat4(SHADER_VIEW_MATRIX, pCamera->GetViewTranslate());
                pShader->SetMat4(SHADER_VIEW_NO_TRANSLATE_MATRIX, pCamera->GetView());
                pShader->SetMat4(SHADER_CAMERA_FAR, pCamera->GetFar());
                pShader->SetMat4(SHADER_CAMERA_NEAR, pCamera->GetNear());
                pShader->SetMat4(SHADER_INVERSE_PROJECTION_MATRIX, pCamera->GetInverseProjection());
                pShader->SetMat4(SHADER_INVERSE_VIEW_MATRIX, pCamera->GetInverseViewTranslate());
                pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_DIRECTION, pLightSystem->GetDirectionalLightParams().direction);
            }

            m_material->Use(*pShader);

            pShader->EndSharedUBO();
        }
        else {
            return;
        }

        if (m_uboManager.BindUBO(m_virtualUBO) == Memory::UBOManager::BindResult::Failed) {
            SR_ERROR("PostProcessPass::Update() : failed to bind UBO!");
        }
        else {
            SR_UNUSED_VARIABLE(pShader->Flush());
        }

        Super::Update();
    }

    void PostProcessPass::DeInit() {
        auto&& uboManager = Memory::UBOManager::Instance();
        if (m_virtualUBO != SR_ID_INVALID && !uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("PostProcessPass::DeInit() : failed to free virtual uniform buffer object!");
        }
        if (m_virtualDescriptor != SR_ID_INVALID) {
            SR_GRAPH_NS::DescriptorManager::Instance().FreeDescriptorSet(&m_virtualDescriptor);
        }
        m_samplers.MarkSamplersDirty();
        Super::DeInit();
    }

    void PostProcessPass::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        Super::UseSamplers(shader);
        m_samplers.UseSamplers(&shader);
        m_material->UseSamplers(shader);
    }

    void PostProcessPass::OnResize(const SR_MATH_NS::UVector2& size) {
        m_dirtyShader = true;
        m_samplers.MarkSamplersDirty();

        Super::OnResize(size);
    }

    void PostProcessPass::OnMultisampleChanged() {
        m_dirtyShader = true;
        m_samplers.MarkSamplersDirty();
        Super::OnMultisampleChanged();
    }

    void PostProcessPass::SetRenderTechnique(SR_GRAPH_NS::IRenderTechnique* pRenderTechnique) {
        BasePass::SetRenderTechnique(pRenderTechnique);
        m_samplers.SetRenderTechnique(pRenderTechnique);
    }

    bool PostProcessPass::Prepare() {
        Super::Prepare();
        return m_samplers.PrepareSamplers();
    }

    void PostProcessPass::SetShader(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        if (!m_material) {
            m_material = SRNew<UniqueMaterial>();
        }
        m_material->SetShader(path);
    }
}