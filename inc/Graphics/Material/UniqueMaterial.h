//
// Created by Monika on 26.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_UNIQUE_MATERIAL_H
#define SR_ENGINE_GRAPHICS_UNIQUE_MATERIAL_H

#include <Graphics/Material/BaseMaterial.h>

namespace SR_GRAPH_NS {
    class UniqueMaterial : public BaseMaterial {
        using Super = BaseMaterial;
    public:
        using Ptr = UniqueMaterial*;

    public:
        SR_NODISCARD MaterialType GetMaterialType() const noexcept override;

        void UnregisterMesh(uint32_t *pId) override;

    };
}

#endif //SR_ENGINE_GRAPHICS_UNIQUE_MATERIAL_H
