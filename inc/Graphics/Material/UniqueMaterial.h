//
// Created by Monika on 26.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_UNIQUE_MATERIAL_H
#define SR_ENGINE_GRAPHICS_UNIQUE_MATERIAL_H

#include <Graphics/Material/BaseMaterial.h>

namespace SR_GRAPH_NS {
    class UniqueMaterial : public BaseMaterial {
        SR_CLASS()
        using Super = BaseMaterial;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<UniqueMaterial>;

    public:
        UniqueMaterial();
        ~UniqueMaterial() override;

    public:
        void SetMaterialData(const MaterialData::Ptr& pData) noexcept;
        void SaveAs(const SR_UTILS_NS::Path& path);
        SR_NODISCARD const MaterialData::Ptr& GetMaterialData() const noexcept override;
        SR_NODISCARD MaterialType GetMaterialType() const noexcept override;

    private:
        /// @property @noHeader @getter(GetMaterialData) @setter(SetMaterialData)
        mutable SR_GRAPH_NS::MaterialData::Ptr m_data;

    };
}

#endif //SR_ENGINE_GRAPHICS_UNIQUE_MATERIAL_H
