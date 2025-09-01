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

    /*bool MeshDrawerPass::Load(const SR_XML_NS::Node& passNode) {
        m_allowedLayers.clear();
        m_disallowedLayers.clear();

        ClearOverrideShaders();

        if (auto&& shaderOverrideNode = passNode.TryGetNode("Shaders")) {
            for (auto&& overrideNode : shaderOverrideNode.TryGetNodes("Override")) {
                auto&& shaderPath = overrideNode.TryGetAttribute("Shader").ToString(std::string());
                const bool ignoreReplace = overrideNode.TryGetAttribute("Ignore").ToBool(false);
                const bool useMaterial = overrideNode.TryGetAttribute("UseMaterialUniforms").ToBool(false);
                const bool useSamplers = overrideNode.TryGetAttribute("UseMaterialSamplers").ToBool(false);

                if (shaderPath.empty() && !ignoreReplace) {
                    SR_ERROR("MeshDrawerPass::Load() : override shader is not set!");
                    continue;
                }

                ShaderUseInfo shaderReplaceInfo = {};
                shaderReplaceInfo.ignoreReplace = ignoreReplace;
                shaderReplaceInfo.useMaterialSamplers = useSamplers;
                shaderReplaceInfo.useMaterialUniforms = useMaterial;

                if (auto&& shaderTypeAttribute = overrideNode.TryGetAttribute("Type")) {
                    auto&& shaderType = SR_UTILS_NS::EnumReflector::FromString<SR_SRSL_NS::ShaderType>(shaderTypeAttribute.ToString());
                    if (m_shaderTypeReplacements.count(shaderType) == 1) {
                        SRHalt("Shader is already set!");
                        continue;
                    }

                    if (auto&& pShader = SR_GTYPES_NS::Shader::Load(shaderPath)) {
                        pShader->AddUsePoint();
                        shaderReplaceInfo.pShader = pShader;
                        m_shaderTypeReplacements[shaderType] = shaderReplaceInfo;
                    }
                }
                else if (auto&& shaderPathAttribute = overrideNode.TryGetAttribute("Path")) {
                    bool found = false;
                    for (auto&& [pShaderKey, pShader] : m_shaderReplacements) {
                        if (shaderPathAttribute.ToString() == pShaderKey->GetResourcePath().ToStringView()) {
                            found = true;
                            break;
                        }
                    }

                    if (found) {
                        SRHalt("Shader is already set!");
                        continue;
                    }

                    auto&& pKeyShader = SR_GTYPES_NS::Shader::Load(shaderPathAttribute.ToString());
                    if (pKeyShader) {
                        pKeyShader->AddUsePoint();
                    }
                    else {
                        SR_ERROR("MeshDrawerPass::Load() : failed to load key shader!\n\tPath: " + shaderPathAttribute.ToString());
                        continue;
                    }

                    if (ignoreReplace) {
                        pKeyShader->AddUsePoint();
                        shaderReplaceInfo.pShader = pKeyShader;
                        m_shaderReplacements[pKeyShader] = shaderReplaceInfo;
                        continue;
                    }

                    auto&& pShader = SR_GTYPES_NS::Shader::Load(shaderPath);
                    if (pShader) {
                        pShader->AddUsePoint();
                    }
                    else {
                        SR_ERROR("MeshDrawerPass::Load() : failed to load shader!\n\tPath: " + shaderPathAttribute.ToString());
                        pKeyShader->RemoveUsePoint();
                        continue;
                    }

                    shaderReplaceInfo.pShader = pShader;
                    m_shaderReplacements[pKeyShader] = shaderReplaceInfo;
                }
            }
        }

        if (auto&& allowedLayersNode = passNode.TryGetNode("AllowedLayers")) {
            for (auto&& layerNode : allowedLayersNode.TryGetNodes()) {
                m_allowedLayers.insert(SR_UTILS_NS::StringAtom(layerNode.NameView()));
            }
        }

        if (auto&& allowedLayersNode = passNode.TryGetNode("DisallowedLayers")) {
            for (auto&& layerNode : allowedLayersNode.TryGetNodes()) {
                m_disallowedLayers.insert(SR_UTILS_NS::StringAtom(layerNode.NameView()));
            }
        }

        m_useMaterials = passNode.TryGetAttribute("UseMaterials").ToBool(true);

        return Super::Load(passNode);
    }*/

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
        pMesh->UseMaterial();
    }

    void MeshDrawerPass::UseSharedUniforms(SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;

        pShader->SetFloat(SHADER_TIME, static_cast<float_t>(m_time.Clock()));

        if (auto&& pCamera = GetCamera()) SR_LIKELY_ATTRIBUTE {
            pShader->SetMat4(SHADER_VIEW_MATRIX, pCamera->GetViewTranslate());
            pShader->SetMat4(SHADER_PROJECTION_MATRIX, pCamera->GetProjection());
            pShader->SetMat4(SHADER_ORTHOGONAL_MATRIX, pCamera->GetOrthogonal());
            pShader->SetVec3(SHADER_VIEW_DIRECTION, pCamera->GetViewDirection());
            pShader->SetVec3(SHADER_VIEW_POSITION, pCamera->GetPosition());
        }

        pShader->SetVec3(SHADER_DIRECTIONAL_LIGHT_POSITION, GetRenderScene()->GetLightSystem()->GetDirectionalLightPosition());
        //if (m_cascadedShadowMapPass) {
        //    pShader->SetValue<false>(SHADER_CASCADE_LIGHT_SPACE_MATRICES, m_cascadedShadowMapPass->GetCascadeMatrices().data());
        //    pShader->SetValue<false>(SHADER_CASCADE_SPLITS, m_cascadedShadowMapPass->GetSplitDepths().data());
        //}
        //else if (m_shadowMapPass) {
        //    pShader->SetMat4(SHADER_LIGHT_SPACE_MATRIX, m_shadowMapPass->GetLightSpaceMatrix());
        //}
    }

    void MeshDrawerPass::UseConstants(SR_GTYPES_NS::Shader* pShader) {
    }

    RenderStrategy* MeshDrawerPass::GetRenderStrategy() const {
        return GetRenderScene()->GetRenderStrategy();
    }

    /*ShaderUseInfo MeshDrawerPass::ReplaceShader(SR_GTYPES_NS::Shader* pShader) const {
        if (!pShader) SR_UNLIKELY_ATTRIBUTE {
            return ShaderUseInfo(pShader);
        }

        if (!m_shaderReplacements.empty()) {
            if (auto&& pIt = m_shaderReplacements.find(pShader); pIt != m_shaderReplacements.end()) {
                if (pIt->second.ignoreReplace) {
                    return ShaderUseInfo(pShader);
                }
                return pIt->second;
            }
        }

        if (!m_shaderTypeReplacements.empty()) {
            if (auto&& pIt = m_shaderTypeReplacements.find(pShader->GetType()); pIt != m_shaderTypeReplacements.end()) {
                return pIt->second;
            }
        }

        return ShaderUseInfo(pShader);
    }*/

    /*void MeshDrawerPass::ClearOverrideShaders() {
        for (auto&& [type, replaceInfo] : m_shaderTypeReplacements) {
            if (replaceInfo.pShader) {
                replaceInfo.pShader->RemoveUsePoint();
            }
        }
        m_shaderTypeReplacements.clear();

        for (auto&& [pShaderKey, replaceInfo] : m_shaderReplacements) {
            pShaderKey->RemoveUsePoint();
            if (replaceInfo.pShader) {
                replaceInfo.pShader->RemoveUsePoint();
            }
        }
        m_shaderReplacements.clear();
    }*/

    void MeshDrawerPass::DeInit() {
        //ClearOverrideShaders();
        for (auto&& pRenderQueue : m_renderQueues) {
            pRenderQueue.AutoFree();
        }
        m_renderQueues.clear();
        Super::DeInit();
    }

    bool MeshDrawerPass::Init() {
        SR_TRACY_ZONE;

        //m_shadowMapPass = GetTechnique()->FindPass<ShadowMapPass>();
        //m_cascadedShadowMapPass = GetTechnique()->FindPass<CascadedShadowMapPass>();

        m_shaderMacros.Clear();
        for (auto&& definition : m_shaderDefines) {
            m_shaderMacros.AddDefine(definition);
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
}
