//
// Created by Monika on 02.06.2024.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H
#define SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H

#include <Utils/Types/SharedPtr.h>
#include <Utils/Types/SortedVector.h>
#include <Graphics/Memory/UBOManager.h>

namespace SR_GTYPES_NS {
    class Shader;
    class Mesh;
}

namespace SR_GRAPH_NS {
    class MeshDrawerPass;
    class RenderContext;
    class RenderScene;

    class MeshRenderQueue : public SR_HTYPES_NS::SharedPtr<MeshRenderQueue> {
        using Super = SR_HTYPES_NS::SharedPtr<MeshRenderQueue>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using VBO = uint32_t;
        using MeshPtr = SR_GTYPES_NS::Mesh*;

        enum QueueState : uint8_t {
            QUEUE_STATE_OK = 0,

            QUEUE_STATE_ERROR         = 1 << 0,
            QUEUE_STATE_VBO_ERROR     = QUEUE_STATE_ERROR | 1 << 1,
            QUEUE_STATE_SHADER_ERROR  = QUEUE_STATE_ERROR | 1 << 2,

            MESH_STATE_VBO_UPDATED    = 1 << 3,
            MESH_STATE_SHADER_UPDATED = 1 << 4,
        };
        typedef uint8_t QueueStateFlags;

        struct MeshInfo {
            ShaderUseInfo shaderUseInfo = {};
            VBO vbo = 0;
            MeshPtr pMesh = nullptr;
            QueueStateFlags state = QUEUE_STATE_ERROR;
        };

        struct MeshRenderQueueLessPredicate {
            SR_NODISCARD constexpr bool operator()(const MeshInfo& left, const MeshInfo& right) const noexcept {
                /// Сравниваем указатели на шейдеры
                if (left.shaderUseInfo.pShader != right.shaderUseInfo.pShader) SR_LIKELY_ATTRIBUTE {
                    return left.shaderUseInfo.pShader < right.shaderUseInfo.pShader;
                }

                /// Если шейдеры одинаковые, сравниваем VBO
                if (left.vbo != right.vbo) SR_UNLIKELY_ATTRIBUTE {
                    return left.vbo < right.vbo;
                }

                /// Если и VBO одинаковые, сравниваем указатели на меши
                return left.pMesh < right.pMesh;
            }
        };

        struct ShaderMismatchPredicate {
            ShaderPtr pShader;
            explicit ShaderMismatchPredicate(ShaderPtr pShader)
                : pShader(pShader)
            { }

            constexpr bool operator()(const MeshInfo& lhs, const MeshInfo& rhs) const noexcept {
                return lhs.shaderUseInfo.pShader == pShader && lhs.shaderUseInfo.pShader < rhs.shaderUseInfo.pShader;
            }
        };

        struct ShaderVBOMismatchPredicate {
            ShaderPtr pShader;
            VBO vbo;
            ShaderVBOMismatchPredicate(ShaderPtr pShader, const VBO vbo)
                : pShader(pShader)
                , vbo(vbo)
            { }

            constexpr bool operator()(const MeshInfo& lhs, const MeshInfo& rhs) const noexcept {
                return lhs.shaderUseInfo.pShader == pShader && lhs.vbo == vbo && lhs.shaderUseInfo.pShader < rhs.shaderUseInfo.pShader;
            }
        };

    public:
        MeshRenderQueue();

        void AddMesh(SR_GTYPES_NS::Mesh* pMesh, ShaderUseInfo shaderUseInfo);
        void RemoveMesh(SR_GTYPES_NS::Mesh* pMesh, ShaderUseInfo shaderUseInfo);

        void Render();
        void Update();

    private:
        SR_NODISCARD MeshInfo* SR_FASTCALL FindNextShader(MeshInfo* pElement);
        SR_NODISCARD MeshInfo* SR_FASTCALL FindNextVBO(MeshInfo* pElement);

        bool SR_FASTCALL UseShader(ShaderPtr pShader);

    private:
        SR_HTYPES_NS::SortedVector<MeshInfo, MeshRenderQueueLessPredicate> m_queue;
        MeshDrawerPass* m_meshDrawerPass = nullptr;
        RenderContext* m_renderContext = nullptr;
        RenderScene* m_renderScene = nullptr;
        Pipeline* m_pipeline = nullptr;
        Memory::UBOManager& m_uboManager;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H
