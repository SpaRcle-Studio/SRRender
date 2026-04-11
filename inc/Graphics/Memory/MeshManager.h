//
// Created by Monika on 05.10.2021.
//

#ifndef SR_ENGINE_GRAPHICS_MEMORY_MESH_MANAGER_H
#define SR_ENGINE_GRAPHICS_MEMORY_MESH_MANAGER_H

#include <Graphics/Pipeline/PipelineType.h>

#include <Utils/Common/Singleton.h>
#include <Utils/Common/Enumerations.h>
#include <Utils/Types/Thread.h>
#include <Utils/Types/SharedPtr.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/Types/SortedVector.h>

namespace SR_GTYPES_NS {
    class Mesh3D;
}

namespace SR_GRAPH_NS {
    class Pipeline;

    class MeshVideoMemoryInfo {
    public:
        struct RegistrationInfo {
            SR_UTILS_NS::StringAtom resourceId;
            uint64_t meshIndex = SR_ID_INVALID;
            uint64_t reloadCount = 0;
            bool isVBO = false;

            SR_NODISCARD uint64_t GetHash() const noexcept {
                uint64_t hash = resourceId.GetHash();
                hash = SR_UTILS_NS::HashCombine(hash, meshIndex);
                hash = SR_UTILS_NS::HashCombine(hash, reloadCount);
                return hash;
            }

            SR_NODISCARD bool operator<(const RegistrationInfo& other) const {
                return GetHash() < other.GetHash();
            }
            SR_NODISCARD bool operator==(const RegistrationInfo& other) const {
                return GetHash() == other.GetHash();
            }
        };

    public:
        MeshVideoMemoryInfo() = default;
        explicit MeshVideoMemoryInfo(RegistrationInfo info)
            : m_info(info)
        { }
        MeshVideoMemoryInfo(RegistrationInfo info, uint32_t size, uint32_t memoryId)
            : m_info(info)
            , m_size(size)
            , m_memoryId(memoryId)
        { }

    public:
        void Use();
        void UnUse();

        SR_NODISCARD uint32_t Copy();
        SR_NODISCARD uint32_t GetUsages() const noexcept { return m_usages; }
        SR_NODISCARD uint32_t Size() const noexcept { return m_size; }

        SR_NODISCARD bool operator<(const MeshVideoMemoryInfo& other) const { return m_info < other.m_info; }
        SR_NODISCARD bool operator==(const MeshVideoMemoryInfo& other) const { return m_info == other.m_info; }

    private:
        uint32_t m_memoryId = SR_ID_INVALID;
        uint32_t m_usages = 0;
        uint32_t m_size = 0;
        RegistrationInfo m_info;

    };

    class MeshManager;

    class BakedMesh : public SR_HTYPES_NS::SharedPtr<BakedMesh> {
        using Super = SR_HTYPES_NS::SharedPtr<BakedMesh>;
        friend MeshManager;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<BakedMesh>;

    public:
        BakedMesh()
            : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        ~BakedMesh() override;

    public:
        void Destroy();

        SR_NODISCARD int32_t GetVBO() const noexcept { return m_VBO; }
        SR_NODISCARD int32_t GetIBO() const noexcept { return m_IBO; }
        SR_NODISCARD uint32_t GetCountIndices() const noexcept { return m_countIndices; }
        SR_NODISCARD uint32_t GetCountVertices() const noexcept { return m_countVertices; }
        SR_NODISCARD uint32_t GetUsages() const noexcept { return m_usages; }
        SR_NODISCARD uint64_t GetMeshIndex() const noexcept { return m_index; }
        SR_NODISCARD Pipeline* GetPipeline() const noexcept { return m_pipeline; }
        SR_NODISCARD SR_HTYPES_NS::RawMesh* GetRawMesh() const noexcept { return m_pRawMesh; }

        void AddUsePoint() {
            m_usages++;
        }

        void RemoveUsePoint() {
            if (m_usages > 0) {
                m_usages--;
                return;
            }
            SRHalt("BakedMesh::RemoveUsePoint() : usages is less than 0!");
        }

        static Ptr Bake(Pipeline* pPipeline, std::string_view path, uint32_t index, SR_UTILS_NS::VertexLayoutDescription layout);
        static Ptr Bake(Pipeline* pPipeline, SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t index, SR_UTILS_NS::VertexLayoutDescription layout);

    private:
        uint32_t m_usages = 0;
        SR_HTYPES_NS::RawMesh* m_pRawMesh = nullptr;
        uint64_t m_index = 0;
        int32_t m_VBO = SR_ID_INVALID;
        int32_t m_IBO = SR_ID_INVALID;
        uint32_t m_countIndices = 0;
        uint32_t m_countVertices = 0;
        Pipeline* m_pipeline = nullptr;
    };

    class MeshManager : public SR_UTILS_NS::Singleton<MeshManager> {
        SR_REGISTER_SINGLETON(MeshManager)
        using RegistrationInfo = MeshVideoMemoryInfo::RegistrationInfo;
    public:
        enum class FreeResult {
            Unknown, Freed, EndUse, NotFound, UnknownMemory, Invalid
        };

    private:
        MeshManager();
        ~MeshManager() override = default;

    public:
        SR_NODISCARD BakedMesh::Ptr BakeMesh(Pipeline* pPipeline, SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t index, SR_UTILS_NS::VertexLayoutDescription layout);

        bool Register(RegistrationInfo info, uint32_t size, uint32_t memoryId, SR_UTILS_NS::VertexLayoutDescription layout);
        bool Register(RegistrationInfo info, uint32_t size, uint32_t memoryId);
        FreeResult Free(bool isVBO, int32_t memoryId);
        int32_t CopyIfExists(RegistrationInfo info, SR_UTILS_NS::VertexLayoutDescription layout);
        int32_t CopyIfExists(RegistrationInfo info);
        uint32_t Size(RegistrationInfo info);
        uint32_t Size(RegistrationInfo info, SR_UTILS_NS::VertexLayoutDescription layout);
        MeshVideoMemoryInfo* Find(RegistrationInfo info, SR_UTILS_NS::VertexLayoutDescription layout);

    private:
        void OnSingletonDestroy() override;

    private:
        struct VertexBufferInfo {
            SR_HTYPES_NS::SortedVector<MeshVideoMemoryInfo> buffers;
            SR_UTILS_NS::VertexLayoutDescription layout;
        };
        std::vector<VertexBufferInfo> m_vertexBuffers;
        SR_HTYPES_NS::SortedVector<MeshVideoMemoryInfo> m_indexBuffers;

        struct Registration {
            SR_UTILS_NS::VertexLayoutDescription vertexLayout;
            std::optional<MeshVideoMemoryInfo::RegistrationInfo> vertexBuffer;
            std::optional<MeshVideoMemoryInfo::RegistrationInfo> indexBuffer;
        };
        SR_HTYPES_NS::FastMemoryArray<Registration> m_registration;

    };
}


#endif //SR_ENGINE_GRAPHICS_MEMORY_MESH_MANAGER_H
