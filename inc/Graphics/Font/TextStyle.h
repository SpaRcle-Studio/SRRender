//
// Created by Codex on 15.11.2026.
//

#ifndef SR_ENGINE_GRAPHICS_TEXT_STYLE_H
#define SR_ENGINE_GRAPHICS_TEXT_STYLE_H

#include <Graphics/macros.h>
#include <Utils/Serialization/Serializable.h>
#include <Utils/Math/Vector4.h>

namespace SR_GTYPES_NS {
    struct SR_GRAPHICS_DLL_API TextStyle : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
    public:
        /// @property
        uint32_t begin = 0;
        /// @property
        uint32_t end = 0;
        /// @property
        bool bold = false;
        /// @property
        bool italic = false;
        /// @property
        bool underline = false;
        /// @property
        bool strike = false;
        /// @property
        float_t relativeSize = 1.f;
        /// @property
        SR_MATH_NS::FVector4 color = SR_MATH_NS::FVector4(1.f);
        /// Per-range shader payloads mapped to CUSTOM2/CUSTOM3. Zero inherits the preceding range's channel.
        /// @property
        SR_MATH_NS::FVector4 customData0 = SR_MATH_NS::FVector4(0.f);
        /// @property
        SR_MATH_NS::FVector4 customData1 = SR_MATH_NS::FVector4(0.f);

        bool operator==(const TextStyle& other) const noexcept {
            return begin == other.begin && end == other.end && bold == other.bold && italic == other.italic
                && underline == other.underline && strike == other.strike && relativeSize == other.relativeSize && color == other.color
                && customData0 == other.customData0 && customData1 == other.customData1;
        }

        bool operator!=(const TextStyle& other) const noexcept {
            return !(*this == other);
        }
    };
}

#endif // SR_ENGINE_GRAPHICS_TEXT_STYLE_H
