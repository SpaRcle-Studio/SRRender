//
// Created by Monika on 02.06.2024.
//

#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Render/MeshRenderQueue.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>

namespace SR_GRAPH_NS {
    MeshRenderQueue::MeshRenderQueue()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_uboManager(Memory::UBOManager::Instance())
    {
        m_queue.Reserve(1024);
    }

    void MeshRenderQueue::AddMesh(SR_GTYPES_NS::Mesh* pMesh, ShaderUseInfo shaderUseInfo) {
        MeshInfo info;
        info.pMesh = pMesh;
        info.shaderUseInfo = shaderUseInfo;
        info.vbo = pMesh->GetVBO();

        m_queue.Add(info);
    }

    void MeshRenderQueue::Render() {
        SR_TRACY_ZONE;

        ShaderPtr pCurrentShader = nullptr;
        VBO currentVBO = 0;

        MeshInfo* pStart = m_queue.data();
        const MeshInfo* pEnd = pStart + m_queue.size();

        for (MeshInfo* pElement = pStart; pElement < pEnd; ++pElement) {
            const MeshInfo info = *pElement;

            if (!info.shaderUseInfo.pShader || pElement->vbo == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                pElement->state = QUEUE_STATE_ERROR;
                continue;
            }

            if (!info.pMesh->IsMeshActive()) SR_UNLIKELY_ATTRIBUTE {
                pElement->state = QUEUE_STATE_ERROR;
                continue;
            }

            if (info.shaderUseInfo.pShader != pCurrentShader) SR_UNLIKELY_ATTRIBUTE {
                pCurrentShader = info.shaderUseInfo.pShader;
                if (!UseShader(pCurrentShader)) SR_UNLIKELY_ATTRIBUTE {
                    pElement->state = QUEUE_STATE_SHADER_ERROR;
                    pElement = FindNextShader(pElement);
                    continue;
                }
            }

            if (info.vbo != currentVBO) SR_UNLIKELY_ATTRIBUTE {
                if (!info.pMesh->BindMesh()) SR_UNLIKELY_ATTRIBUTE {
                    pElement->state = QUEUE_STATE_VBO_ERROR;
                    pElement = FindNextVBO(pElement);
                    continue;
                }
                currentVBO = info.vbo;
            }

            info.pMesh->Draw();
            pElement->state = QUEUE_STATE_OK;
        }

        if (pCurrentShader) SR_LIKELY_ATTRIBUTE {
            pCurrentShader->UnUse();
        }
    }

    void MeshRenderQueue::Update() {
        SR_TRACY_ZONE;

        ShaderPtr pCurrentShader = nullptr;

        MeshInfo* pStart = m_queue.data();
        const MeshInfo* pEnd = pStart + m_queue.size();

        for (MeshInfo* pElement = pStart; pElement < pEnd; ++pElement) {
            const MeshInfo info = *pElement;

            if (info.state & QUEUE_STATE_ERROR) SR_UNLIKELY_ATTRIBUTE {
                if (info.state & QUEUE_STATE_SHADER_ERROR) {
                    pElement = FindNextShader(pElement);
                }
                else if (info.state & QUEUE_STATE_VBO_ERROR) {
                    pElement = FindNextVBO(pElement);
                }
                continue;
            }

            if (info.state & MESH_STATE_SHADER_UPDATED) SR_LIKELY_ATTRIBUTE {
                pElement = FindNextShader(pElement);
                continue;
            }

            if (info.shaderUseInfo.pShader != pCurrentShader) SR_UNLIKELY_ATTRIBUTE {
                pCurrentShader = info.shaderUseInfo.pShader;
                if (pCurrentShader->BeginSharedUBO()) SR_LIKELY_ATTRIBUTE {
                    m_meshDrawerPass->UseSharedUniforms(ShaderUseInfo(pCurrentShader));
                    pCurrentShader->EndSharedUBO();
                }
            }

            if (info.state & MESH_STATE_VBO_UPDATED) SR_LIKELY_ATTRIBUTE {
                pElement = FindNextVBO(pElement);
                continue;
            }

            auto&& virtualUbo = info.pMesh->GetVirtualUBO();
            if (virtualUbo == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            m_meshDrawerPass->UseUniforms(info.shaderUseInfo, info.pMesh);

            if (m_uboManager.BindUBO(virtualUbo) == Memory::UBOManager::BindResult::Duplicated) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("VBORenderStage::Update() : memory has been duplicated!");
                continue;
            }

            SR_MAYBE_UNUSED_VAR info.shaderUseInfo.pShader->Flush();
        }

        m_renderContext->SetCurrentShader(nullptr);
        m_renderScene->SetCurrentSkeleton(nullptr);
    }

    MeshRenderQueue::MeshInfo* MeshRenderQueue::FindNextShader(MeshInfo* pElement) {
        const ShaderMismatchPredicate predicate(pElement->shaderUseInfo.pShader);
        return m_queue.UpperBound(pElement, m_queue.data() + m_queue.size(), *pElement, predicate);
    }

    MeshRenderQueue::MeshInfo* MeshRenderQueue::FindNextVBO(MeshInfo* pElement) {
        const ShaderVBOMismatchPredicate predicate(pElement->shaderUseInfo.pShader, pElement->vbo);
        return m_queue.UpperBound(pElement, m_queue.data() + m_queue.size(), *pElement, predicate);
    }

    bool MeshRenderQueue::UseShader(ShaderPtr pShader) {
        SR_TRACY_ZONE;

        if (pShader->Use() == ShaderBindResult::Failed) {
            return false;
        }

        if (!pShader->IsSamplersValid()) {
            std::string message = "Shader samplers is not valid!\n\tPath: " + pShader->GetResourcePath().ToStringRef();
            for (auto&& [name, sampler] : pShader->GetSamplers()) {
                if (m_pipeline->IsSamplerValid(sampler.samplerId)) {
                    continue;
                }

                message += "\n\tSampler is not set: " + name.ToStringRef();
            }
            ///TODO: SetError(message);
            pShader->UnUse();
            return false;
        }

        if (m_pipeline->IsShaderChanged()) {
            m_meshDrawerPass->UseConstants(ShaderUseInfo(pShader));
            m_meshDrawerPass->UseSamplers(ShaderUseInfo(pShader));
        }

        return true;
    }
}
