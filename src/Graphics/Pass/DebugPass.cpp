//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Pass/DebugPass.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Render/DebugRenderer.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(DebugPass)

    bool DebugPass::Load(const SR_XML_NS::Node& passNode) {
        if (auto&& shadersNode = passNode.TryGetNode("Shaders")) {
            for (auto&& shaderNode : shadersNode.TryGetNodes("Shader")) {
                auto&& id = shaderNode.GetAttribute("Id").ToString();
                auto&& path = shaderNode.GetAttribute("Path").ToString();
                m_shaders[id].pShader = SR_GTYPES_NS::Shader::Load(path);
            }
        }

        for (auto& [id, shaderInfo] : m_shaders) {
            if (!shaderInfo.pShader) {
                SR_ERROR("DebugRenderer::Load() : failed to load shader \"{}\"!", id);
                continue;
            }
            shaderInfo.pShader->AddUsePoint();
        }

        static std::vector<std::string> requiredShaders = {
            "line",
            "mesh"
        };

        for (auto&& shader : requiredShaders) {
            if (m_shaders.find(shader) == m_shaders.end()) {
                SR_ERROR("DebugRenderer::Load() : shader \"{}\" not set, but required!", shader);
                return false;
            }
        }

        if (m_shaders["line"].drawQueues.empty()) {
            m_shaders["line"].drawQueues.emplace_back();
        }

        return Super::Load(passNode);
    }

    void DebugPass::Prepare() {
        auto&& pDebugRenderer = GetRenderScene()->GetDebugRenderer();
        if (!pDebugRenderer) {
            return;
        }

        m_needUpdate = pDebugRenderer->IsRenderSceneChanged();

        if (m_needUpdate) {
            BuildQueues();
        }

        Super::Prepare();
    }

    bool DebugPass::Render() {
        SR_TRACY_ZONE;

        auto&& pDebugRenderer = GetRenderScene()->GetDebugRenderer();
        auto&& pPipeline = GetPassPipeline();
        if (!pDebugRenderer || !pPipeline) {
            return false;
        }

        m_hasRendered = false;
        m_needUpdate = true;

        for (auto& [id, shaderInfo] : m_shaders) {
            shaderInfo.uboUsed = 0;
        }

        BuildQueues();

        for (auto& [id, shaderInfo] : m_shaders) {
            auto&& pShader = shaderInfo.pShader;
            if (!pShader || shaderInfo.drawQueues.empty()) {
                continue;
            }

            pShader->Use();

            for (auto& queue : shaderInfo.drawQueues) {
                if (queue.empty()) {
                    continue;
                }

                const auto meshId = static_cast<int32_t>(queue.back().type);

                if (meshId == static_cast<int32_t>(DebugRenderer::DrawType::Line)) {
                    DrawQueue(pPipeline.GetUncheckedRef(), queue, shaderInfo, 2);
                    continue;
                }

                const Memory::BakedMesh& mesh = pDebugRenderer->GetMeshUnchecked(meshId);

                pPipeline->BindVBO(mesh.GetVBO());
                pPipeline->BindIBO(mesh.GetIBO());

                DrawQueue(pPipeline.GetUncheckedRef(), queue, shaderInfo, mesh.GetCountIndices());
            }

            pShader->UnUse();
        }

        return m_hasRendered;
    }

    void DebugPass::Update() {
        SR_TRACY_ZONE;

        if (!m_hasRendered || !m_camera) {
            return;
        }

        UpdateUBO(m_shaders["line"_atom], DebugRenderer::DrawType::Line);
        UpdateUBO(m_shaders["mesh"_atom], DebugRenderer::DrawType::Mesh);

        Super::Update();
    }

    bool DebugPass::Init() {
        return Super::Init();
    }

    void DebugPass::DeInit() {
        for (auto& [id, shaderInfo] : m_shaders) {
            shaderInfo.pShader->RemoveUsePoint();

            for (ShaderInfo::MemInfo& UBO : shaderInfo.UBOs) {
                m_uboManager.FreeUBO(&UBO.virtualUBO);
                m_descriptorManager.FreeDescriptorSet(&UBO.virtualDescriptor);
            }
        }
        m_shaders.clear();

        Super::DeInit();
    }

    void DebugPass::OnResize(const SR_MATH_NS::UVector2& size) {
        Super::OnResize(size);
    }

    void DebugPass::BuildQueues() {
        SR_TRACY_ZONE;

        auto&& pDebugRenderer = GetRenderScene()->GetDebugRenderer();
        auto&& pPipeline = GetPassPipeline();
        if (!pDebugRenderer || !pPipeline) {
            return;
        }

        ShaderInfo& lineShader = m_shaders["line"_atom];
        ShaderInfo& meshShader = m_shaders["mesh"_atom];

        m_linesCountCache = { static_cast<uint32_t>(lineShader.drawQueues.back().size()), 0 };
        m_meshesCountCache.resize(meshShader.drawQueues.size());
        for (size_t i = 0; i < m_meshesCountCache.size(); ++i) {
            m_meshesCountCache[i] = { static_cast<uint32_t>(meshShader.drawQueues[i].size()), 0 };
        }

        bool changed = false;

        auto&& timedObjects = pDebugRenderer->GetTimedObjects();
        auto&& objectsRef = timedObjects.GetObjects();
        for (auto&& [valid, timedObject] : objectsRef) {
            if (!valid) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            const auto& drawInfo = timedObject.drawInfo;

            if (drawInfo.type == DebugRenderer::DrawType::Line) {
                auto&& linesQueue = lineShader.drawQueues.back();
                if (linesQueue.size() <= m_linesCountCache.second) SR_UNLIKELY_ATTRIBUTE {
                    linesQueue.emplace_back(drawInfo);
                }
                else {
                    linesQueue[m_linesCountCache.second] = drawInfo;
                }
                ++m_linesCountCache.second;
            }
            else {
                if (meshShader.drawQueues.size() <= static_cast<size_t>(drawInfo.type)) {
                    meshShader.drawQueues.resize(static_cast<size_t>(drawInfo.type) + 1);
                    m_meshesCountCache.resize(static_cast<size_t>(drawInfo.type) + 1);
                }

                uint32_t& indexRef = m_meshesCountCache[static_cast<size_t>(drawInfo.type)].second;

                if (meshShader.drawQueues[static_cast<size_t>(drawInfo.type)].size() <= indexRef) SR_UNLIKELY_ATTRIBUTE {
                    meshShader.drawQueues[static_cast<size_t>(drawInfo.type)].emplace_back(drawInfo);
                }
                else {
                    changed |= meshShader.drawQueues[static_cast<size_t>(drawInfo.type)][indexRef].type != drawInfo.type;
                    meshShader.drawQueues[static_cast<size_t>(drawInfo.type)][indexRef] = drawInfo;
                }

                ++indexRef;
            }
        }

        changed |= m_linesCountCache.first != m_linesCountCache.second;
        lineShader.drawQueues.back().resize(m_linesCountCache.second);
        for (uint32_t i = 0; i < m_meshesCountCache.size(); ++i) {
            changed |= m_meshesCountCache[i].first != m_meshesCountCache[i].second;
            meshShader.drawQueues[i].resize(m_meshesCountCache[i].second);
        }

        if (changed) SR_UNLIKELY_ATTRIBUTE {
            GetPassPipeline()->SetDirty(true);
            m_hasRendered = false;
        }
    }

    void DebugPass::DrawQueue(Pipeline& pipeline, const std::vector<DebugRenderer::DrawInfo>& queue, ShaderInfo& shaderInfo, uint32_t indicesCount) {
        SR_TRACY_ZONE;

        for (auto&& drawInfo : queue) {
            if (shaderInfo.uboUsed >= shaderInfo.UBOs.size()) {
                auto&& memInfo = shaderInfo.UBOs.emplace_back();

                switch (drawInfo.type) {
                    case DebugRenderer::DrawType::Line:
                        memInfo.virtualUBO = m_uboManager.AllocateUBO(SR_ID_INVALID);
                        break;
                    case DebugRenderer::DrawType::None:
                        SRHalt("DebugPass::DrawQueue() : invalid draw type!");
                        break;
                    default:
                        memInfo.virtualUBO = m_uboManager.AllocateUBO(SR_ID_INVALID);
                        break;
                }

                memInfo.virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(SR_ID_INVALID);
            }

            if (m_uboManager.BindUBO(shaderInfo.UBOs[shaderInfo.uboUsed].virtualUBO) == Memory::UBOManager::BindResult::Failed) {
                SR_ERROR("DebugPass::DrawQueue() : failed to bind UBO!");
                continue;
            }

            if (m_descriptorManager.Bind(shaderInfo.UBOs[shaderInfo.uboUsed].virtualDescriptor) == DescriptorManager::BindResult::Failed) {
                SR_ERROR("DebugPass::DrawQueue() : failed to bind descriptor set!");
                continue;
            }

            if (pipeline.GetCurrentBuildIteration() == 0) {
                m_descriptorManager.Flush();
            }

            pipeline.DrawIndices(indicesCount);

            ++shaderInfo.uboUsed;

            m_hasRendered = true;
        }
    }

    void DebugPass::UpdateUBO(ShaderInfo& shaderInfo, DebugRenderer::DrawType type) {
        if (!shaderInfo.pShader || !shaderInfo.pShader->Ready()) {
            return;
        }

        GetPassPipeline()->SetCurrentShader(shaderInfo.pShader);

        if (shaderInfo.pShader->BeginSharedUBO()) {
            shaderInfo.pShader->SetMat4(SHADER_VIEW_MATRIX, m_camera->GetViewTranslate());
            shaderInfo.pShader->SetMat4(SHADER_PROJECTION_MATRIX, m_camera->GetProjection());
            shaderInfo.pShader->EndSharedUBO();
        }

        if (!m_needUpdate) {
            return;
        }

        uint32_t index = 0;

        for (auto& drawQueue : shaderInfo.drawQueues) {
            for (auto& drawInfo : drawQueue) {
                /** Если меш не был отрисован, то бинд не пройдет */
                if (m_uboManager.BindNoDublicateUBO(shaderInfo.UBOs[index].virtualUBO) == Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
                    if (type == DebugRenderer::DrawType::Line) {
                        shaderInfo.pShader->SetVec3(SHADER_LINE_START_POINT, drawInfo.start);
                        shaderInfo.pShader->SetVec3(SHADER_LINE_END_POINT, drawInfo.end);
                        shaderInfo.pShader->SetVec4(SHADER_LINE_COLOR, drawInfo.color);
                    }
                    else {
                         shaderInfo.pShader->SetVec4(SHADER_RGBA_VALUE, drawInfo.color);
                         shaderInfo.pShader->SetMat4(SHADER_MODEL_MATRIX, drawInfo.modelMatrix);
                    }

                    if (!shaderInfo.pShader->Flush()) {
                        SR_ERROR("DebugPass::Update() : failed to flush shader \"line\"!");
                    }
                }

                ++index;
            }
        }
    }
}
