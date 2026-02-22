//
// Created by Monika on 08.12.2025.
//

#ifndef SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H
#define SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H

#include <Graphics/Pipeline/TextureHelper.h>

#include <Utils/Serialization/Serializable.h>
#include <Utils/Math/Rect.h>
#include <Utils/Resources/ResourceLoadMode.h>

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
        SR_NODISCARD float_t GetPixelsPerUnit() const noexcept { return m_pixelsPerUnit; }
        SR_NODISCARD const SR_MATH_NS::FRect& GetBorder() const noexcept { return m_border; }
        SR_NODISCARD AddressMode GetAddressMode() const noexcept { return addressMode; }
        SR_NODISCARD SR_UTILS_NS::ResourceLoadMode GetLoadMode() const noexcept { return loadMode; }

        bool operator==(const ImageMetaInfo& lrs) const {
            return format == lrs.format
                   && loadMode == lrs.loadMode
                   && filter == lrs.filter
                   && addressMode == lrs.addressMode
                   && compression == lrs.compression
                   && mipLevels == lrs.mipLevels
                   && m_border == lrs.m_border
                   && SR_EQUALS_T(m_pixelsPerUnit, lrs.m_pixelsPerUnit, SR_SCALAR_EPSILON)
                   && (alpha == lrs.alpha || alpha == SR_UTILS_NS::BoolExt::None || lrs.alpha == SR_UTILS_NS::BoolExt::None)
                   && cpuUsage == lrs.cpuUsage;
        }

        bool operator!=(const ImageMetaInfo& lrs) const {
            return !(*this == lrs);
        }

    public:
        /// @property
        SR_UTILS_NS::ResourceLoadMode loadMode = SR_UTILS_NS::ResourceLoadMode::Async;
        /// @property
        ImageFormat format = ImageFormat::RGBA8_UNORM;
        /// @property
        TextureFilter filter = TextureFilter::LINEAR;
        /// @property
        AddressMode addressMode = AddressMode::Repeat;
        /// @property
        TextureCompression compression = TextureCompression::None;
        /// @property
        SR_UTILS_NS::BoolExt alpha = SR_UTILS_NS::BoolExt::None;
        /// @property
        float_t m_pixelsPerUnit = 100.f;
        /// @property
        uint32_t mipLevels = 0;
        /// @property
        bool cpuUsage = false;
        /// @property
        SR_MATH_NS::FRect m_border;

    };
}

#endif //SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H
