//
// Created by Monika on 02.06.2024.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H
#define SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Utils/Frustum.h>

#include <Utils/Types/SharedPtr.h>
#include <Utils/Types/SortedVector.h>
#include <Utils/Types/FastMemoryArray.h>

namespace SR_GTYPES_NS {
    class Shader;
    class Mesh;
}

namespace SR_GRAPH_NS {
    namespace Memory {
        class UBOManager;
    }
    class MeshDrawerPass;
    class RenderStrategy;
    class RenderContext;
    class RenderScene;
    class Pipeline;

    struct RenderQueueInfo;

    class RenderQueue : public SR_HTYPES_NS::SharedPtr<RenderQueue> {
        using Super = SR_HTYPES_NS::SharedPtr<RenderQueue>;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
        using VBO = uint32_t;
        using Layer = SR_UTILS_NS::StringAtom;

        struct MeshShaderPair {
            SR_GTYPES_NS::Mesh* pMesh;
            RenderQueueInfo* pInfo;
        };
    public:
        enum QueueState : uint8_t {
            QUEUE_STATE_OK = 0,

            QUEUE_STATE_ERROR          = 1 << 1,
            QUEUE_STATE_VBO_ERROR      = QUEUE_STATE_ERROR | 1 << 2,
            QUEUE_STATE_SHADER_ERROR   = QUEUE_STATE_ERROR | 1 << 3,
            QUEUE_STATE_NOT_RENDERED   = QUEUE_STATE_ERROR | 1 << 4,
            QUEUE_STATE_WAIT_REGISTER  = QUEUE_STATE_ERROR | 1 << 5,
            QUEUE_STATE_MISSING_SHADER = QUEUE_STATE_ERROR | 1 << 6,
            QUEUE_STATE_INVISIBLE      = QUEUE_STATE_ERROR | 1 << 7,
        };
        typedef uint8_t QueueStateFlags;

        struct MeshInfo {
            RenderQueueInfo* pInfo = nullptr;
            VBO vbo = 0;
            SR_GTYPES_NS::Mesh* pMesh = nullptr;
            SR_GTYPES_NS::Shader* pShader = nullptr;
            int64_t priority = 0;
            QueueStateFlags state = QUEUE_STATE_NOT_RENDERED;

            bool operator==(const MeshInfo& other) const noexcept;
        };

        struct RenderQueueLessPredicate {
            SR_NODISCARD bool operator()(const MeshInfo& left, const MeshInfo& right) const noexcept;
        };

        using Queue = SR_HTYPES_NS::SortedVector<MeshInfo, RenderQueueLessPredicate>;

    public:
        RenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer);
        ~RenderQueue() override;

        void Register(const MeshRegistrationInfo& info);
        void UnRegister(const MeshRegistrationInfo& info);

        void Init();

        bool Render();
        void Update();

        bool UpdateFrustumCulling(const Frustum& frustum);
        void SetFrustumCullingAllowed(bool allowed) { m_isFrustumAllowed = allowed; }

        void OnMeshDirty(SR_GTYPES_NS::Mesh* pMesh, RenderQueueInfo* pInfo);

        SR_NODISCARD const std::vector<std::pair<Layer, Queue>>& GetQueues() const noexcept { return m_queues; }

    protected:
        virtual void CustomDrawMesh(const MeshInfo& info) { }

        SR_NODISCARD MeshDrawerPass* GetMeshDrawerPass() const noexcept { return m_meshDrawerPass; }

    private:
        void UpdateShaders();
        void UpdateMeshes();
        void UpdateAllMeshes();

        SR_NODISCARD bool IsSuitable(const MeshRegistrationInfo& info) const;

        void Render(const SR_UTILS_NS::StringAtom& layer, Queue& queue);

        SR_NODISCARD MeshInfo* SR_FASTCALL FindNextShader(Queue& queue, MeshInfo* pElement);
        SR_NODISCARD MeshInfo* SR_FASTCALL FindNextVBO(Queue& queue, MeshInfo* pElement);

        bool SR_FASTCALL UseShader(SR_GTYPES_NS::Shader* pShader);

        void PrepareLayers();

    protected:
        bool m_customMeshDraw = false;

    private:
        bool m_multiFrameMode = false;
        bool m_updateMeshesOnDemand = false;
        bool m_rendered = false;
        bool m_isInitialized = false;
        bool m_isFrustumAllowed = true;

        uint64_t m_layersStateHash = 0;

        Memory::UBOManager& m_uboManager;

        std::vector<std::pair<Layer, Queue>> m_queues;

        SR_HTYPES_NS::SortedVector<SR_GTYPES_NS::Shader*> m_shaders;
        SR_HTYPES_NS::FastMemoryArray<MeshShaderPair> m_meshes;
        SR_HTYPES_NS::FastMemoryArray<MeshShaderPair> m_tempMeshes;

        MeshDrawerPass* m_meshDrawerPass = nullptr;
        RenderContext* m_renderContext = nullptr;
        RenderStrategy* m_renderStrategy = nullptr;
        RenderScene* m_renderScene = nullptr;
        Pipeline* m_pipeline = nullptr;

    };

    struct RenderQueueInfo {
        RenderQueue* pRenderQueue;
        SR_GTYPES_NS::Shader* pShader;
        std::bitset<16> dirtyUniformsFrames;
        bool inUpdateQueue : 1 = false;
        bool isVisible : 1 = true;

        bool operator==(const RenderQueueInfo& other) const {
            return pRenderQueue == other.pRenderQueue;
        }
    };

    struct MeshRenderQueues {
        constexpr static size_t MaxQueues = 64;
        RenderQueueInfo queues[MaxQueues] = {};
        uint8_t count = 0;
        uint8_t maxCount = 0;

        void Clear() {
            count = 0;
            maxCount = 0;
            for (auto& q : queues) {
                q = {};
            }
        }

        SR_NODISCARD RenderQueueInfo* data() { return &queues[0]; }
        SR_NODISCARD size_t size() const { return maxCount; }

        SR_NODISCARD RenderQueueInfo* Find(RenderQueue* pQueue) {
            SR_TRACY_ZONE;
            for (size_t i = 0; i < maxCount; ++i) {
                if (queues[i].pRenderQueue == pQueue) {
                    return &queues[i];
                }
            }
            return nullptr;
        }

        SR_NODISCARD RenderQueueInfo* Add(RenderQueue* pQueue) {
            SR_TRACY_ZONE;
            if (count >= MaxQueues) {
                SRHalt("MeshRenderQueues::Add() : max queues limit reached!");
                return nullptr;
            }
            for (size_t i = 0; i < MaxQueues; ++i) {
                if (queues[i].pRenderQueue == nullptr) {
                    queues[i].pRenderQueue = pQueue;
                    count++;
                    maxCount = SR_MAX(maxCount, i + 1);
                    return &queues[i];
                }
            }
            SRHalt("MeshRenderQueues::Add() : queue not found, but should be added!");
            return nullptr;
        }

        RenderQueueInfo Remove(RenderQueue* pQueue) {
            SR_TRACY_ZONE;
            for (size_t i = 0; i < maxCount; ++i) {
                if (queues[i].pRenderQueue == pQueue) {
                    RenderQueueInfo tmp = queues[i];
                    queues[i] = {};
                    --count;
                    return tmp;
                }
            }
            SRHalt("MeshRenderQueues::Remove() : queue not found!");
            return {};
        }
    };

    static constexpr size_t SIZE_OF_MESH_RENDER_QUEUES_CLASS = sizeof(MeshRenderQueues);

    struct RenderQueuePredicate {
        using Element = RenderQueueInfo;
        SR_NODISCARD bool operator()(const Element& left, const Element& right) const noexcept {
            return left.pRenderQueue < right.pRenderQueue;
        }
    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_RENDER_QUEUE_H
