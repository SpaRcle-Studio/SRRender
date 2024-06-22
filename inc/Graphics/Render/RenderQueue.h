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
    class RenderStrategy;
    class RenderContext;
    class RenderScene;

    class RenderQueue : public SR_HTYPES_NS::SharedPtr<RenderQueue> {
        using Super = SR_HTYPES_NS::SharedPtr<RenderQueue>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using VBO = uint32_t;
        using Layer = SR_UTILS_NS::StringAtom;
        using MeshPtr = SR_GTYPES_NS::Mesh*;

    public:
        enum QueueState : uint8_t {
            QUEUE_STATE_OK = 0,

            QUEUE_STATE_ERROR         = 1 << 1,
            QUEUE_STATE_VBO_ERROR     = QUEUE_STATE_ERROR | 1 << 2,
            QUEUE_STATE_SHADER_ERROR  = QUEUE_STATE_ERROR | 1 << 3,
        };
        typedef uint8_t QueueStateFlags;

        struct MeshInfo {
            ShaderUseInfo shaderUseInfo = {};
            VBO vbo = 0;
            MeshPtr pMesh = nullptr;
            int64_t priority = 0;
            QueueStateFlags state = QUEUE_STATE_ERROR;
            bool hasVBO = false;

            bool operator==(const MeshInfo& other) const noexcept {
                return
                    shaderUseInfo.pShader == other.shaderUseInfo.pShader &&
                    vbo == other.vbo &&
                    pMesh == other.pMesh &&
                    priority == other.priority;
            }
        };

        struct RenderQueueLessPredicate {
            SR_NODISCARD constexpr bool operator()(const MeshInfo& left, const MeshInfo& right) const noexcept {
                /// Сравниваем приоритеты
                if (left.priority != right.priority) SR_UNLIKELY_ATTRIBUTE {
                    return left.priority < right.priority;
                }

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

        struct ShaderInfo {
            ShaderInfo() = default;
            ShaderInfo(ShaderPtr pShader) : info(pShader) { }
            ShaderUseInfo info = {};
            uint32_t count = 0;
        };

        struct ShaderQueueLessPredicate {
            SR_NODISCARD constexpr bool operator()(const ShaderUseInfo& left, const ShaderUseInfo& right) const noexcept {
                return left.pShader < right.pShader;
            }
        };

        using Queue = SR_HTYPES_NS::SortedVector<MeshInfo, RenderQueueLessPredicate>;

    public:
        RenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer);
        virtual ~RenderQueue();

        void Register(const MeshRegistrationInfo& info);
        void UnRegister(const MeshRegistrationInfo& info);

        void Init();

        bool Render();
        void Update();

        void OnMeshDirty(MeshPtr pMesh, ShaderUseInfo info);

        SR_NODISCARD const std::vector<std::pair<Layer, Queue>>& GetQueues() const noexcept { return m_queues; }

    protected:
        virtual void CustomDrawMesh(const MeshInfo& info) { }

        SR_NODISCARD MeshDrawerPass* GetMeshDrawerPass() const noexcept { return m_meshDrawerPass; }

    private:
        void UpdateShaders();
        void UpdateMeshes();

        SR_NODISCARD bool IsSuitable(const MeshRegistrationInfo& info) const;

        void Render(const SR_UTILS_NS::StringAtom& layer, Queue& queue);

        SR_NODISCARD MeshInfo* SR_FASTCALL FindNextShader(Queue& queue, MeshInfo* pElement);
        SR_NODISCARD MeshInfo* SR_FASTCALL FindNextVBO(Queue& queue, MeshInfo* pElement);

        bool SR_FASTCALL UseShader(ShaderUseInfo info);

        void PrepareLayers();

        SR_NODISCARD SR_GRAPH_NS::ShaderUseInfo GetShaderUseInfo(const MeshRegistrationInfo& info) const;

    protected:
        bool m_customMeshDraw = false;

    private:
        bool m_rendered = false;
        bool m_isInitialized = false;

        uint64_t m_layersStateHash = 0;

        Memory::UBOManager& m_uboManager;

        std::vector<std::pair<Layer, Queue>> m_queues;

        SR_HTYPES_NS::SortedVector<ShaderUseInfo, ShaderQueueLessPredicate> m_shaders;
        std::vector<std::pair<MeshPtr, ShaderUseInfo>> m_meshes;

        MeshDrawerPass* m_meshDrawerPass = nullptr;
        RenderContext* m_renderContext = nullptr;
        RenderStrategy* m_renderStrategy = nullptr;
        RenderScene* m_renderScene = nullptr;
        Pipeline* m_pipeline = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H
