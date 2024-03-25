//
// Created by qulop on 17.03.2024
//
#ifndef SR_ENGINE_ATLAS_BUILDER_H
#define SR_ENGINE_ATLAS_BUILDER_H

#include <Utils/stdInclude.h>
#include <Utils/FileSystem/Path.h>
#include <Utils/Math/Vector2.h>
#include <Graphics/Loaders/TextureLoader.h>

namespace SR_GRAPH_NS {
    struct AtlasBuilderData {
        SR_UTILS_NS::Path location;
        SR_MATH_NS::USVector2 quantityOnAxes;
        uint32_t xStep = 0;
        uint32_t yStep = 0;
    };

    class AtlasBuilder : public SR_UTILS_NS::NonCopyable {
        using Super = SR_UTILS_NS::NonCopyable;
    public:
        AtlasBuilder(SR_UTILS_NS::Path path, const SR_MATH_NS::USVector2& quantityOnAxes, uint32_t xStep, uint32_t yStep);
        explicit AtlasBuilder(const SR_UTILS_NS::Path& folder);

    private:
        void Copy(const AtlasBuilder& other);

    public:
        SR_NODISCARD SR_UTILS_NS::Path GetLocation() const { return m_location; }
        SR_NODISCARD bool IsCached() const { return m_isCached; }
        SR_NODISCARD SR_MATH_NS::USVector2 GetQuantity() const { return m_quantityOnAxes; }
        SR_NODISCARD uint32_t GetXStep() const { return m_xStep; }
        SR_NODISCARD uint32_t GetYStep() const { return m_yStep; }

        SR_NODISCARD bool Generate(const SR_UTILS_NS::Path& path) const;

    private:
        SR_UTILS_NS::Path m_location;
        SR_MATH_NS::USVector2 m_quantityOnAxes;
        uint32_t m_xStep = 0;
        uint32_t m_yStep = 0;
        bool m_isCached = false;
    };


    class AtlasCreationStrategy {
        using AtlasPtr = SR_GRAPH_NS::TextureData::Ptr;
    public:
        AtlasCreationStrategy(const std::list<AtlasPtr>& sprites, uint32_t totalWidth, uint32_t totalHeight);

    public:
        SR_NODISCARD std::optional<AtlasBuilder> ChooseAndCreate(const SR_UTILS_NS::Path& destFolder) const;

    private:
        SR_NODISCARD SR_UTILS_NS::Path CreateTargetFile(AtlasPtr atlas) const;

        // std::optional<AtlasBuilder> CreateSquare() const;
        SR_NODISCARD std::optional<AtlasBuilder> CreateTightSquare() const;
        // std::optional<AtlasBuilder> CreateLine() const;

    private:
        std::list<AtlasPtr> m_sprites;
        SR_UTILS_NS::Path m_destination;
        uint32_t m_totalWidth = 0;
        uint32_t m_totalHeight = 0;
    };
}
#endif // SR_ENGINE_ATLAS_BUILDER_H