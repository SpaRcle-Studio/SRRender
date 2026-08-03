//
// Created by Monika on 13.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_ATLAS_H
#define SR_ENGINE_GRAPHICS_ATLAS_H

#include <Graphics/stdInclude.h>

#include <Utils/Resources/Asset.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(AtlasType, uint8_t,
        SpriteSheet,
        Packed,
        Dynamic
    );

    struct AtlasEntry : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_MATH_NS::USRect rect; /// Пиксельные координаты в атласе
        /// @property
        SR_MATH_NS::USVector2 size; /// Размер в пикселях (с padding)
    };

    struct AtlasSpriteSheetSettings : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::StringAtom prefix;

        bool operator==(const AtlasSpriteSheetSettings& other) const noexcept {
            return prefix == other.prefix;
        }

        bool operator!=(const AtlasSpriteSheetSettings& other) const noexcept {
            return !(*this == other);
        }
    };

    /// @extension(sratlas)
    class Atlas : public SR_UTILS_NS::Asset {
        using Super = SR_UTILS_NS::Asset;
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Atlas>;

    public:
        SR_NODISCARD const AtlasEntry* GetTexture(SR_UTILS_NS::StringAtom path) const;

    private:
        /// @property
        AtlasType m_type = AtlasType::SpriteSheet;

        /// @property @condition(This.m_type == AtlasType::SpriteSheet)
        SR_UTILS_NS::Vector<AtlasSpriteSheetSettings> m_spriteSheets;

        /// @property @hidden
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, AtlasEntry> m_entries;

    };
}

#endif //SR_ENGINE_GRAPHICS_ATLAS_H
