//
// Created by Monika on 18.01.2024.
//

#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Mesh.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <Codegen/MeshDrawerPass.generated.hpp>

namespace SR_GRAPH_NS {
    MeshDrawerPass::MeshDrawerPass()
        : Super()
        , m_time(SR_HTYPES_NS::Time::Instance())
    { }

    MeshDrawerPass::~MeshDrawerPass() = default;

    bool MeshDrawerPass::IsLayerAllowed(SR_UTILS_NS::StringAtom layer) const {
        if (m_allowedLayers.empty()) {
            if (m_disallowedLayers.empty()) {
                return true;
            }
            return m_disallowedLayers.count(layer) == 0;
        }
        else if (m_allowedLayers.count(layer) == 1) {
            return true;
        }

        if (m_disallowedLayers.empty()) {
            return m_allowedLayers.count(layer) == 1;
        }

        return m_disallowedLayers.count(layer) == 0;
    }

    bool MeshDrawerPass::Render() {
        const uint32_t layer = GetPipeline()->GetCurrentFrameBufferLayer();
        if (layer >= m_renderQueues.size()) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("MeshDrawerPass::Render() : out of bounds! Layer: {}, Queues: {}", layer, m_renderQueues.size());
            return false;
        }

        return m_renderQueues[layer]->Render();
    }

    void MeshDrawerPass::Update() {
        const uint32_t layer = GetPipeline()->GetCurrentFrameBufferLayer();
        if (layer >= m_renderQueues.size()) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("MeshDrawerPass::Update() : out of bounds! Layer: {}, Queues: {}", layer, m_renderQueues.size());
            return;
        }

        m_renderQueues[layer]->Update();
    }

    void MeshDrawerPass::UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh) {
        if (m_uniforms.material.useMaterial) {
            pMesh->UseMaterial();
        }
        else if (m_uniforms.material.modelMatrix) {
            pMesh->UseModelMatrix();
        }
    }

    void MeshDrawerPass::UseSharedUniforms(SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;

        if (m_uniforms.shared.time) {
            pShader->SetFloat(SHADER_TIME, static_cast<float_t>(m_time.Clock()));
        }

        if (m_uniforms.shared.camera) {
            if (auto&& pCamera = GetCamera()) SR_LIKELY_ATTRIBUTE {
                pShader->SetMat4(SHADER_VIEW_MATRIX, pCamera->GetViewTranslate());
                pShader->SetMat4(SHADER_PROJECTION_MATRIX, pCamera->GetProjection());
                pShader->SetMat4(SHADER_ORTHOGONAL_MATRIX, pCamera->GetOrthogonal());
                pShader->SetVec3(SHADER_VIEW_DIRECTION, pCamera->GetViewDirection());
                pShader->SetVec3(SHADER_VIEW_POSITION, pCamera->GetPosition());
            }
        }

        if (m_uniforms.shared.light) {
            pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_DIRECTION, GetRenderScene()->GetLightSystem()->GetDirectionalLightDirection());
        }

        for (SR_UTILS_NS::StringAtom anotherPass : m_uniforms.shared.useFromPass) {
            if (auto&& pAnotherPass = GetTechnique()->FindPass(anotherPass)) {
                pAnotherPass->UseUniformsFromAnotherPass(pShader);
            }
        }
    }

    void MeshDrawerPass::UseConstants(SR_GTYPES_NS::Shader* pShader) {
    }

    void MeshDrawerPass::UseSamplers(SR_GTYPES_NS::Shader* pShader) {
        Super::UseSamplers(pShader);
        m_samplers.UseSamplers(pShader);
    }

    RenderStrategy* MeshDrawerPass::GetRenderStrategy() const {
        return GetRenderScene()->GetRenderStrategy();
    }

    void MeshDrawerPass::DeInit() {
        for (auto&& pRenderQueue : m_renderQueues) {
            pRenderQueue.AutoFree();
        }
        m_renderQueues.clear();
        Super::DeInit();
    }

    bool MeshDrawerPass::Init() {
        SR_TRACY_ZONE;

        m_shaderMacros.Clear();

        for (auto&& definition : m_shaderDefines) {
            m_shaderMacros.AddDefine(definition);
        }

        auto&& macros = GetRenderContext()->GetShaderMacros();
        for (auto&& [key, value] : macros) {
            m_shaderMacros.SetParam(key, value);
        }

        if (m_renderLayers == 0) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("MeshDrawerPass::Init() : layers count is 0!");
            return false;
        }

        SRAssert(m_renderQueues.empty());

        m_renderQueues.resize(m_renderLayers);
        for (uint8_t i = 0; i < m_renderLayers; ++i) {
            m_renderQueues[i] = AllocateRenderQueue();
        }

        return Super::Init();
    }

    MeshDrawerPass::RenderQueuePtr MeshDrawerPass::AllocateRenderQueue() {
        return GetRenderStrategy()->BuildQueue(this);
    }

    void MeshDrawerPass::SetRenderTechnique(SR_GRAPH_NS::IRenderTechnique* pRenderTechnique) {
        BasePass::SetRenderTechnique(pRenderTechnique);
        m_samplers.SetRenderTechnique(pRenderTechnique);
    }

    void MeshDrawerPass::OnMultisampleChanged() {
        Super::OnMultisampleChanged();
        m_samplers.MarkSamplersDirty();
    }

    void MeshDrawerPass::OnResize(const SR_MATH_NS::UVector2& size) {
        Super::OnResize(size);
        m_samplers.MarkSamplersDirty();
    }

    void MeshDrawerPass::Prepare() {
        Super::Prepare();
        m_samplers.PrepareSamplers();
    }
}
