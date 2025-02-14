//
// Created by Monika on 26.05.2024.
//

#include <Graphics/Material/UniqueMaterial.h>

#include <Codegen/UniqueMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    const MaterialData::Ptr& UniqueMaterial::GetMaterialData() const noexcept {
        static MaterialData::Ptr pEmptyData;
        return pEmptyData;
    }

    MaterialType UniqueMaterial::GetMaterialType() const noexcept {
        return MaterialType::Unique;
    }

    void UniqueMaterial::UnregisterMesh(uint32_t *pId) {
        Super::UnregisterMesh(pId);
    }
}
