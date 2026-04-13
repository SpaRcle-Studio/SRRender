//
// Created by Nikita on 17.11.2020.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_H
#define SR_ENGINE_GRAPHICS_MESH_H

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Types/IRenderComponent.h>

#include <Utils/Common/Enumerations.h>
#include <Utils/Common/Vertices.h>
#include <Utils/Types/Function.h>
#include <Utils/Types/SortedVector.h>

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class DescriptorManager;
    class BaseMaterial;
}

namespace SR_GTYPES_NS {
    class Shader;

    /// @abstract
    class Mesh : public SR_GTYPES_NS::IRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<Shader>;
        using Ptr = SR_HTYPES_NS::SharedPtr<Mesh>;

    public:
        Mesh();
        ~Mesh() override;

    public:
        static std::vector<Mesh::Ptr> Load(const SR_UTILS_NS::Path& path);
        static Mesh::Ptr TryLoad(SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t id);
        static Mesh::Ptr TryLoad(const SR_UTILS_NS::Path& path, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom name);

    public:
        SR_NODISCARD bool ExecuteInEditMode() const final { return true; }
        SR_NODISCARD bool IsActive() const noexcept override;
        SR_NODISCARD virtual uint32_t GetIndicesCount() const { return 0; }
        SR_NODISCARD const SR_MATH_NS::Matrix4x4& GetMatrix() const;
        SR_NODISCARD int32_t GetVirtualUBO() const final { return m_virtualUBO; }

        virtual void SetFrustumCullingType(FrustumCullingType type) { }
        void SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description) override;

        bool Bind() override;
        void Draw() override;

    protected:
        SR_NODISCARD bool IsCalculated() const noexcept { return m_isCalculated; }
        SR_NODISCARD virtual bool IsCalculatable() const;
        void FreeVideoMemory() override;
        virtual bool OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId);
        virtual bool Calculate();

    protected:
        bool m_isCalculated = false;

    private:
        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_H
