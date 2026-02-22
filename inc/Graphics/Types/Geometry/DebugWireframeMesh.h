//
// Created by Monika on 05.10.2021.
//

#ifndef SR_ENGINE_DEBUGWIREFRAMEMESH_H
#define SR_ENGINE_DEBUGWIREFRAMEMESH_H

#include <Utils/Types/IRawMeshHolder.h>

#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Types/Vertices.h>
#include <Graphics/Types/Uniforms.h>

namespace SR_GTYPES_NS {
    /// @category(Render)
    class DebugWireframeMesh final : public IndexedMesh, public SR_HTYPES_NS::IRawMeshHolder {
        SR_CLASS()
        using Super = IndexedMesh;
    public:
        typedef Vertices::SimpleVertex VertexType;

    public:
        DebugWireframeMesh() = default;
        ~DebugWireframeMesh() override = default;

    public:
        void SetColor(const SR_MATH_NS::FVector4& color);

        bool OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) override;

        void OnRawMeshChanged() override;

        SR_NODISCARD MeshType GetMeshTypeImpl() const noexcept override { return MeshType::Wireframe; }

        SR_NODISCARD const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const override;
        SR_NODISCARD std::string GetMeshIdentifier() const override;

        SR_NODISCARD SR_UTILS_NS::StringAtom GetMeshLayer() const override {
            const static SR_UTILS_NS::StringAtom debugLayer = "Debug";
            return debugLayer;
        }

        void SetMatrix(const SR_MATH_NS::Matrix4x4& matrix) override;
        SR_NODISCARD const SR_MATH_NS::Matrix4x4& GetMatrix() const override { return m_modelMatrix; }

        bool Calculate() override;

        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;

    private:
        SR_MATH_NS::FVector4 m_color;
        SR_MATH_NS::Matrix4x4 m_modelMatrix;

    };
}

#endif //SR_ENGINE_DEBUGWIREFRAMEMESH_H
