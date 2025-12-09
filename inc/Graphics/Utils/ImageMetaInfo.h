//
// Created by Monika on 08.12.2025.
//

#ifndef SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H
#define SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H

#include <Graphics/Pipeline/TextureHelper.h>

#include <Utils/Serialization/Serializable.h>

namespace SR_GRAPH_NS {
    struct ImageMetaInfo : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
    public:
        SR_NODISCARD ImageFormat GetFormat() const noexcept { return format; }
        SR_NODISCARD TextureFilter GetFilter() const noexcept { return filter; }
        SR_NODISCARD TextureCompression GetCompression() const noexcept { return compression; }
        SR_NODISCARD uint32_t GetMipLevels() const noexcept { return mipLevels; }
        SR_NODISCARD SR_UTILS_NS::BoolExt GetAlpha() const noexcept { return alpha; }
        SR_NODISCARD bool GetCpuUsage() const noexcept { return cpuUsage; }

        bool operator==(const ImageMetaInfo& lrs) const {
            return format == lrs.format
                   && filter == lrs.filter
                   && compression == lrs.compression
                   && mipLevels == lrs.mipLevels
                   && (alpha == lrs.alpha || alpha == SR_UTILS_NS::BoolExt::None || lrs.alpha == SR_UTILS_NS::BoolExt::None)
                   && cpuUsage == lrs.cpuUsage;
        }

        bool operator!=(const ImageMetaInfo& lrs) const {
            return !(*this == lrs);
        }

    public:
        /// @property
        ImageFormat format = ImageFormat::RGBA8_UNORM;
        /// @property
        TextureFilter filter = TextureFilter::LINEAR;
        /// @property
        TextureCompression compression = TextureCompression::None;
        /// @property
        SR_UTILS_NS::BoolExt alpha = SR_UTILS_NS::BoolExt::None;
        /// @property
        uint32_t mipLevels = 0;
        /// @property
        bool cpuUsage = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H
