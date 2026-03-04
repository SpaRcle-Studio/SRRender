//
// Created by Monika on 09.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_ACTIVE_GRAPHICS_SETTINGS_H
#define SR_ENGINE_GRAPHICS_ACTIVE_GRAPHICS_SETTINGS_H

#include <Graphics/Settings/Quality.h>

#include <Utils/Serialization/Serializable.h>
#include <Utils/FileSystem/Path.h>

namespace SR_GRAPH_NS {
    struct ActiveGraphicsSettings : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        ActiveGraphicsSettings() = default;

        SR_INLINE_STATIC SR_UTILS_NS::Path SETTINGS_PATH = "User/GraphicsSettings.sra";

        bool operator==(const ActiveGraphicsSettings& lrs) const {
            return instancing == lrs.instancing
                   && shadowsInstancing == lrs.shadowsInstancing
                   && postProcess == lrs.postProcess
                   && hdr == lrs.hdr
                   && autoExposure == lrs.autoExposure
                   && sRGB == lrs.sRGB
                   && shadowsQuality == lrs.shadowsQuality
                   && colorBufferQuality == lrs.colorBufferQuality;
        }

        bool operator!=(const ActiveGraphicsSettings& lrs) const {
            return !(*this == lrs);
        }

        /// @property
        bool instancing = true;
        /// @property
        bool shadowsInstancing = true;
        /// @property
        bool postProcess = true;
        /// @property
        bool hdr = true;
        /// @property
        bool autoExposure = true;
        /// @property
        bool sRGB = true;
        /// @property
        bool textureCompression = true;
        /// @property
        Quality shadowsQuality = Quality::High;
        /// @property
        Quality colorBufferQuality = Quality::High;

    };
}

#endif //SR_ENGINE_GRAPHICS_ACTIVE_GRAPHICS_SETTINGS_H
