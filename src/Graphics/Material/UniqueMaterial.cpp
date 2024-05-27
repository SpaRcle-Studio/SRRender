//
// Created by Monika on 26.05.2024.
//

#include <Graphics/Material/UniqueMaterial.h>

namespace SR_GRAPH_NS {
    MaterialType UniqueMaterial::GetMaterialType() const noexcept {
        return MaterialType::Unique;
    }

    void UniqueMaterial::UnregisterMesh(uint32_t *pId) {
        Super::UnregisterMesh(pId);
        FinalizeMaterial();
        delete this;
    }
}
