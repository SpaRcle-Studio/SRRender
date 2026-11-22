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
    struct SR_GRAPHICS_DLL_API ImageMetaInfo : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
    public:
        SR_NODISCARD ImageFormat GetFormat() const noexcept { return format; }
        SR_NODISCARD TextureFilter GetFilter() const noexcept { return filter; }
        SR_NODISCARD TextureCompression GetCompression() const noexcept { return compression; }
        SR_NODISCARD uint32_t GetMipLevels() const noexcept { return mipLevels; }
        SR_NODISCARD bool GetCpuUsage() const noexcept { return cpuUsage; }
        SR_NODISCARD float_t GetPixelsPerUnit() const noexcept { return m_pixelsPerUnit; }
        SR_NODISCARD const SR_MATH_NS::FRect& GetBorder() const noexcept { return m_border; }
        SR_NODISCARD AddressMode GetAddressMode() const noexcept { return addressMode; }
        SR_NODISCARD SR_UTILS_NS::ResourceLoadMode GetLoadMode() const noexcept { return loadMode; }
        SR_NODISCARD bool GetDisableAlphaChannel() const noexcept { return disableAlphaChannel; }

        bool operator==(const ImageMetaInfo& lrs) const;
        bool operator!=(const ImageMetaInfo& lrs) const {
            return !(*this == lrs);
        }

    public:
        /// @property
        SR_UTILS_NS::ResourceLoadMode loadMode = SR_UTILS_NS::ResourceLoadMode::Async;
        /// @property
        ImageType imageType = ImageType::Albedo;
        /// @property
        ImageFormat format = ImageFormat::Auto;
        /// @property
        TextureFilter filter = TextureFilter::LINEAR;
        /// @property
        AddressMode addressMode = AddressMode::Repeat;
        /// @property
        TextureCompression compression = TextureCompression::None;
        /// @property
        TextureCompressionPolicy compressionPolicy = TextureCompressionPolicy::Auto;
        /// @property
        TextureContent content = TextureContent::Auto;
        /// @property
        TextureQualityValidation qualityValidation = TextureQualityValidation::Warn;
        /// @property
        float_t compressionQuality = 0.5f;
        /// @property
        bool preserveAlphaCoverage = false;
        /// @property
        float_t alphaCutoff = 0.5f;
        /// @property @hidden
        bool compressed = false;
        /// @property @hidden
        SR_UTILS_NS::Vector<SR_UTILS_NS::Path> preparedVariants;
        /// @property @hidden
        SR_UTILS_NS::Vector<uint64_t> preparedHashes;
        /// @property
        float_t m_pixelsPerUnit = 100.f;
        /// @property
        uint32_t mipLevels = 0;
        /// @property
        bool cpuUsage = false;
        /// @property
        bool disableAlphaChannel = false;
        /// @property
        SR_MATH_NS::FRect m_border;

    };
}

#endif //SR_ENGINE_GRAPHICS_IMAGE_META_INFO_H
