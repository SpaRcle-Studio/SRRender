//
// Created by Monika on 26.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_MATERIAL_PROPERTY_H
#define SR_ENGINE_GRAPHICS_MESH_MATERIAL_PROPERTY_H

#include <Utils/TypeTraits/Properties.h>
#include <Graphics/Material/MaterialType.h>

namespace SR_GTYPES_NS {
    class Mesh;
}

namespace SR_GRAPH_NS {
    class BaseMaterial;

    class MeshMaterialProperty final : public SR_UTILS_NS::Property {
        SR_REGISTER_TYPE_TRAITS_PROPERTY(MeshMaterialProperty, 1000)
        using Super = SR_UTILS_NS::Property;
    public:
        MeshMaterialProperty();
        ~MeshMaterialProperty() override;

    public:
        void SaveProperty(MarshalRef marshal) const noexcept override;
        void LoadProperty(MarshalRef marshal) noexcept override;

    public:
        SR_NODISCARD MaterialType GetMaterialType() const noexcept;
        SR_NODISCARD BaseMaterial* GetMaterial() const noexcept { return m_pMaterial; }
        void SetMaterial(BaseMaterial* pMaterial) noexcept;
        void SetMaterial(const SR_UTILS_NS::Path& path) noexcept;
        void SetMesh(SR_GTYPES_NS::Mesh* pMesh) noexcept { m_pMesh = pMesh; }

    private:
        SR_GTYPES_NS::Mesh* m_pMesh = nullptr;
        BaseMaterial* m_pMaterial = nullptr;
        uint32_t m_materialRegisterId = SR_ID_INVALID;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_MATERIAL_PROPERTY_H
