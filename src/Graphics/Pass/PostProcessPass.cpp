//
// Created by Monika on 07.08.2022.
//

#include <Graphics/Pass/PostProcessPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/PostProcessPass.generated.hpp>

namespace SR_GRAPH_NS {
    PostProcessPass::~PostProcessPass() {
        if (m_shader) {
            m_onShaderReloaded.Reset();
            m_shader->RemoveUsePoint();
        }
    }

    bool PostProcessPass::PreRender() {
        return Super::PreRender();
    }

    bool PostProcessPass::Render() {
        SR_TRACY_ZONE;

        if (!m_shader || m_shader->Use() == ShaderBindResult::Failed) {
            return false;
        }

        if (m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_shader->UnUse();
                return false;
            }

            m_virtualDescriptor = DescriptorManager::Instance().AllocateDescriptorSet(m_virtualDescriptor);
        }

        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        if (GetPipeline()->GetCurrentBuildIteration() == 0) {
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
                UseSamplers(ShaderUseInfo(m_shader));
                m_descriptorManager.Flush();
            }
            GetPipeline()->GetCurrentShader()->FlushConstants();
        }

        if (result != DescriptorManager::BindResult::Failed) {
            GetPipeline()->Draw(m_vertices);
        }

        m_shader->UnUse();

        m_dirtyShader = false;

        return true;
    }

    void PostProcessPass::Update() {
        if (m_virtualUBO == SR_ID_INVALID) {
            return;
        }

        GetPipeline()->SetCurrentShader(m_shader.Get());

        if (m_shader && m_shader->BeginSharedUBO()) {
            SR_MATH_NS::FVector2 resolution;
            if (auto&& pCamera = GetRenderScene()->GetMainCamera()) {
                resolution = pCamera->GetSize().Cast<float_t>();
            }
            else {
                resolution = GetRenderScene()->GetSurfaceSize().Cast<float_t>();
            }

            m_shader->SetVec2(SHADER_RESOLUTION, resolution);

            m_shader->SetFloat(SHADER_TIME, static_cast<float_t>(SR_HTYPES_NS::Time::Instance().Clock()));

            if (auto&& pCamera = GetCamera()) {
                m_shader->SetVec3(SHADER_VIEW_POSITION, pCamera->GetPosition());
                m_shader->SetVec3(SHADER_VIEW_DIRECTION, pCamera->GetViewDirection());
                m_shader->SetMat4(SHADER_PROJECTION_MATRIX, pCamera->GetProjection());
                m_shader->SetMat4(SHADER_VIEW_MATRIX, pCamera->GetViewTranslate());
                m_shader->SetMat4(SHADER_VIEW_NO_TRANSLATE_MATRIX, pCamera->GetView());
            }

            m_shader->EndSharedUBO();
        }
        else {
            return;
        }

        if (m_uboManager.BindUBO(m_virtualUBO) == Memory::UBOManager::BindResult::Duplicated) {
            SR_ERROR("PostProcessPass::Update() : memory has been duplicated!");
        }

        SR_UNUSED_VARIABLE(m_shader->Flush());

        Super::Update();
    }

    void PostProcessPass::SetShader(const SR_UTILS_NS::Path& shaderPath) {
        m_shaderPath = shaderPath.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

        auto&& pShader = SR_GTYPES_NS::Shader::Load(m_shaderPath);
        if (!pShader) {
            SR_ERROR("PostProcessPass::SetShader() : failed to load shader: {}", shaderPath);
            return;
        }

        if (m_shader == pShader) {
            return;
        }

        m_dirtyShader = true;

        if (m_shader) {
            m_onShaderReloaded.Reset();
            m_shader->RemoveUsePoint();
            m_shader = nullptr;
        }

        if (!(m_shader = pShader)) {
            return;
        }

        m_shader->AddUsePoint();

        m_onShaderReloaded = pShader->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](auto&&) {
            m_dirtyShader = true;
        });
    }

    void PostProcessPass::DeInit() {
        auto&& uboManager = Memory::UBOManager::Instance();
        if (m_virtualUBO != SR_ID_INVALID && !uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("PostProcessPass::DeInit() : failed to free virtual uniform buffer object!");
        }
        if (m_virtualDescriptor != SR_ID_INVALID) {
            SR_GRAPH_NS::DescriptorManager::Instance().FreeDescriptorSet(&m_virtualDescriptor);
        }
        Super::DeInit();
    }

    void PostProcessPass::OnResize(const SR_MATH_NS::UVector2& size) {
        m_dirtyShader = true;
        Super::OnResize(size);
    }

    void PostProcessPass::OnMultisampleChanged() {
        m_dirtyShader = true;
        Super::OnMultisampleChanged();
    }
}