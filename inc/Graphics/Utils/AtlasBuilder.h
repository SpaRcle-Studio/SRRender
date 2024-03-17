//
// Created by qulop on 17.03.2024
//
#ifndef SR_ENGINE_ATLAS_BUILDER_H
#define SR_ENGINE_ATLAS_BUILDER_H

#include <Utils/stdInclude.h>
#include <Utils/FileSystem/Path.h>
#include <Utils/Math/Vector2.h>

namespace SR_GRAPH_NS {
/*    struct AtlasConfigAttributes {
        SR_UTILS_NS::Path location;

        bool isCached = false;

        SR_MATH_NS::USVector2 quantityOnAxes;
        uint32_t xStep = 0;
        uint32_t yStep = 0;
    };*/


    class AtlasBuilder : public SR_UTILS_NS::NonCopyable {
    public:
        using Super = SR_UTILS_NS::NonCopyable;

        AtlasBuilder(const SR_UTILS_NS::Path& path, const SR_MATH_NS::USVector2& quantityOnAxes, uint32_t xStep, uint32_t yStep);

        AtlasBuilder(const SR_UTILS_NS::Path& folder);  // NOLINT

        SR_NODISCARD SR_UTILS_NS::Path GetLocation() const;
        SR_NODISCARD bool IsCached() const;
        SR_NODISCARD SR_MATH_NS::USVector2 GetQuantity() const;
        SR_NODISCARD uint32_t GetXStep() const;
        SR_NODISCARD uint32_t GetYStep() const;

        SR_NODISCARD bool Generate(const SR_UTILS_NS::Path& path) const;

    private:
        SR_NODISCARD bool CreateAtlas(const SR_UTILS_NS::Path& folder) const;


        SR_UTILS_NS::Path m_location;
        bool m_isCached = false;
        SR_MATH_NS::USVector2 m_quantityOnAxes;
        uint32_t m_xStep = 0;
        uint32_t m_yStep = 0;
    };
}
#endif // SR_ENGINE_ATLAS_BUILDER_H