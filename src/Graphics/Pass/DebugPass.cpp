//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Pass/DebugPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Render/DebugRenderer.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>

#include <Utils/Common/Features.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/DebugPass.generated.hpp>

namespace SR_GRAPH_NS {
    DebugPassShaderInfo::~DebugPassShaderInfo() {
        if (pShader) {
            pShader->RemoveUsePoint();
        }
    }

    void DebugPassShaderInfo::LoadShader(const DebugPass* pPass) {
        SR_TRACY_ZONE;

        if (!shaderPath.empty()) {
            if (pShader) {
                pShader->RemoveUsePoint();
            }

            SR_SRSL_NS::ShaderMacrosParams shaderMacros;

            auto&& macros = pPass->GetRenderContext()->GetShaderMacros();
            for (auto&& [key, value] : macros) {
                shaderMacros.SetParam(key, value);
            }

            const uint32_t layers = pPass->GetColorLayersCount();
            for (uint32_t i = 0; i < layers; ++i) {
                shaderMacros.AddDefine(SR_SRSL_NS::SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[i]);
            }

            pShader = SR_GTYPES_NS::Shader::Load(shaderPath, shaderMacros);
            if (pShader) {
                pShader->AddUsePoint();
            }
            else {
                SR_ERROR("DebugPassShaderInfo::LoadShader() : failed to load shader from path \"{}\"!", shaderPath);
            }
        }
    }

    void DebugPassShaderInfo::SetShader(const SR_UTILS_NS::Path& path) {
        shaderPath = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
    }

    bool DebugPass::Prepare() {
        SR_TRACY_ZONE;

        auto&& pDebugRenderer = GetDebugRenderer();
        if (!pDebugRenderer) {
            SR_ERROR("DebugPass::Prepare() : debug renderer is null!");
            return false;
        }

        m_isNeedUpdate = pDebugRenderer->IsRenderSceneChanged();
        pDebugRenderer->ResetChangedFlags();

        if (m_isNeedUpdate) {
            BuildQueues();
        }

        return Super::Prepare();
    }

    bool DebugPass::Render() {
        SR_TRACY_ZONE;

        if (!m_isValid) {
            return false;
        }

        auto&& pDebugRenderer = GetDebugRenderer();
        auto&& pPipeline = GetPipeline();
        if (!pDebugRenderer || !pPipeline) {
            return false;
        }

        m_hasRendered = false;
        m_isNeedUpdate = true;

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
                    DrawQueue(const_cast<Pipeline&>(pPipeline.GetUncheckedRef()), queue, shaderInfo, 2);
                    continue;
                }

                if (meshId < 0) {
                    SR_ERROR("DebugPass::Render() : invalid meshId {} for shader \"{}\"!", meshId, id);
                    continue;
                }

                if (!pDebugRenderer->IsMeshValid(meshId)) {
                    continue;
                }

                const Memory::BakedMesh& mesh = pDebugRenderer->GetMeshUnchecked(meshId);

                pPipeline->BindVBO(mesh.GetVBO());
                pPipeline->BindIBO(mesh.GetIBO());

                DrawQueue(const_cast<Pipeline&>(pPipeline.GetUncheckedRef()), queue, shaderInfo, mesh.GetCountIndices());
            }

            pShader->UnUse();
        }

        return m_hasRendered;
    }

    void DebugPass::Update() {
        SR_TRACY_ZONE;

        if (!m_hasRendered || !GetCamera()) {
            return;
        }

        UpdateUBO(m_shaders["line"_atom], DebugRenderer::DrawType::Line);
        UpdateUBO(m_shaders["mesh"_atom], DebugRenderer::DrawType::Mesh);

        Super::Update();
    }

    bool DebugPass::Init() {
        SR_TRACY_ZONE;

        m_isValid = true;
        m_updateMeshesOnDemand = SR_UTILS_NS::Features::Instance().Enabled("UpdateDebugMeshesOnDemand", false);

        for (auto& [id, shaderInfo] : m_shaders) {
            shaderInfo.LoadShader(this);

            if (!shaderInfo.pShader) {
                SR_ERROR("DebugPass::Load() : failed to load shader \"{}\"!", id);
                continue;
            }
        }

        static std::vector<SR_UTILS_NS::StringAtom> requiredShaders = { "line", "mesh" };

        for (auto&& shader : requiredShaders) {
            if (m_shaders.find(shader) == m_shaders.end()) {
                SR_ERROR("DebugPass::Init() : shader \"{}\" not set, but required!", shader);
                m_isValid = false;
            }
        }

        if (m_shaders["line"].drawQueues.empty()) {
            m_shaders["line"].drawQueues.emplace_back();
        }

        return Super::Init();
    }

    void DebugPass::DeInit() {
        SR_TRACY_ZONE;

        for (auto& [id, shaderInfo] : m_shaders) {
            for (DebugPassShaderInfo::MemInfo& UBO : shaderInfo.UBOs) {
                m_uboManager.FreeUBO(&UBO.virtualUBO);
                m_descriptorManager.FreeDescriptorSet(&UBO.virtualDescriptor);
            }
        }
        m_shaders.clear();

        Super::DeInit();
    }

    void DebugPass::OnResize(const SR_MATH_NS::UVector2& size) {
        SR_TRACY_ZONE;
        Super::OnResize(size);
    }

    void DebugPass::BuildQueues() {
        SR_TRACY_ZONE;

        if (!m_isValid) {
            return;
        }

        auto&& pDebugRenderer = GetDebugRenderer();
        auto&& pPipeline = GetPipeline();
        if (!pDebugRenderer || !pPipeline) {
            return;
        }

        DebugPassShaderInfo& lineShader = m_shaders["line"_atom];
        DebugPassShaderInfo& meshShader = m_shaders["mesh"_atom];

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
            GetPipeline()->SetDirty(true);
            m_hasRendered = false;
        }
    }

    void DebugPass::DrawQueue(Pipeline& pipeline, const std::vector<DebugRenderer::DrawInfo>& queue, DebugPassShaderInfo& shaderInfo, uint32_t indicesCount) {
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

            const auto bindResult = m_descriptorManager.Bind(shaderInfo.UBOs[shaderInfo.uboUsed].virtualDescriptor);
            if (bindResult == DescriptorManager::BindResult::Failed) {
                SR_ERROR("DebugPass::DrawQueue() : failed to bind descriptor set!");
                continue;
            }

            if (shaderInfo.UBOs[shaderInfo.uboUsed].isDirty || bindResult == DescriptorManager::BindResult::Duplicated) {
                m_descriptorManager.Flush();
                shaderInfo.UBOs[shaderInfo.uboUsed].isDirty = false;
            }

            if (drawInfo.type == DebugRenderer::DrawType::Line) {
                pipeline.Draw(indicesCount);
            }
            else {
                pipeline.DrawIndices(indicesCount);
            }

            ++shaderInfo.uboUsed;

            m_hasRendered = true;
        }
    }

    void DebugPass::UpdateUBO(DebugPassShaderInfo& shaderInfo, DebugRenderer::DrawType type) {
        SR_TRACY_ZONE;

        if (!m_isValid) {
            return;
        }

        if (!shaderInfo.pShader || !shaderInfo.pShader->Ready()) {
            return;
        }

        GetPipeline()->SetCurrentShader(shaderInfo.pShader.Get());

        if (shaderInfo.pShader->BeginSharedUBO()) {
            shaderInfo.pShader->SetMat4(SHADER_VIEW_MATRIX, GetCamera()->GetViewTranslate());
            shaderInfo.pShader->SetMat4(SHADER_PROJECTION_MATRIX, GetCamera()->GetProjection());
            shaderInfo.pShader->EndSharedUBO();
        }

        if (!m_isNeedUpdate && m_updateMeshesOnDemand) {
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
                        shaderInfo.pShader->SetColor(SHADER_LINE_COLOR, drawInfo.color / 255.0f);
                    }
                    else {
                         shaderInfo.pShader->SetColor(SHADER_RGBA_VALUE, drawInfo.color / 255.0f);
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

    SR_HTYPES_NS::SharedPtr<DebugRenderer> DebugPass::GetDebugRenderer() const {
        if (m_isOverlay) {
            return GetRenderScene()->GetRenderer<DebugOverlayRenderer>().StaticCast<DebugRenderer>();
        }
        else {
            return GetRenderScene()->GetRenderer<DebugRenderer>();
        }
    }
}
