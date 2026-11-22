//
// Created by Nikita on 28.06.2021.
//

#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Loaders/TextureContainer.h>

#include <Utils/Debug.h>
#include <Utils/Profile/TracyContext.h>

#include <Enum/TextureCompression.hpp>
#include <Enum/ImageFormat.hpp>

namespace SR_GRAPH_NS {
    uint64_t GetCompressedImageSize(uint32_t width, uint32_t height, const TextureLoadInfo& info) {
        uint64_t size = 0;
        for (uint32_t mip = 0; mip < info.mips; ++mip) {
            size += TextureContainer::GetLevelSize(width, height, info.compression, info.format);
            width = std::max(1u, width / 2);
            height = std::max(1u, height / 2);
        }
        return size;
    }

    uint32_t Find4(uint32_t i) {
        SR_TRACY_ZONE;
        if (i % 4 == 0)
            return i;
        else
            return Find4(i - 1);
    }

    SR_UTILS_NS::Pair<uint32_t, uint32_t> MakeGoodSizes(uint32_t w, uint32_t h) {
        SR_TRACY_ZONE;
        return SR_UTILS_NS::Pair<uint32_t, uint32_t>(Find4(w), Find4(h));
    }

    uint8_t* ResizeToLess(uint32_t ow, uint32_t oh, uint32_t nw, uint32_t nh, const uint8_t* pixels) {
        SR_TRACY_ZONE;
        auto&& image = (uint8_t*)SRMalloc(nw * nh * 4);
        uint32_t dw = ow - nw;

        for (uint32_t row = 0; row < nh; ++row) {
            memcpy(image + (nw * 4 * row), pixels + (dw * 4 * row) + (nw * 4 * row), nw * 4);
        }

        return image;
    }

    uint8_t GetChannelCount(ImageFormat format) {
        switch (format) {
            case ImageFormat::RGBA8_UNORM:
            case ImageFormat::RGBA16_SFLOAT:
            case ImageFormat::BGRA8_UNORM:
            case ImageFormat::RGBA8_SRGB:
                return 4;
            case ImageFormat::R8_UNORM:
            case ImageFormat::R8_UINT:
                return 1;
            case ImageFormat::RG8_UNORM:
                return 2;
            case ImageFormat::RGB8_UNORM:
                return 3;
            case ImageFormat::RGBA16_UNORM:
            case ImageFormat::RGB16_UNORM:
            case ImageFormat::R16_UNORM:
            case ImageFormat::R32_SFLOAT:
            case ImageFormat::R64_SFLOAT:
            case ImageFormat::R16_UINT:
            case ImageFormat::R32_UINT:
            case ImageFormat::R64_UINT:
                SRHalt("GetChannelCount : unsupported color format!\n\tImageFormat: {}", SR_UTILS_NS::EnumReflector::ToStringAtom(format));
                return 0;
            case ImageFormat::Unknown:
            default:
                SRHalt("GetChannelCount : unknown color format!\n\tImageFormat: {}", SR_UTILS_NS::EnumReflector::ToStringAtom(format));
                return 0;
        }
    }

    uint32_t GetPixelSize(ImageFormat format) {
        SR_TRACY_ZONE;
        switch (format) {
            case ImageFormat::RGBA8_UNORM:
            case ImageFormat::BGRA8_UNORM:
            case ImageFormat::RGBA8_SRGB:
                return 4 * 1;
            case ImageFormat::RGBA16_UNORM:
            case ImageFormat::RGBA16_SFLOAT:
                return 4 * 2;
            case ImageFormat::RGB8_UNORM:
            case ImageFormat::RGB8_SRGB:
                return 3 * 1;
            case ImageFormat::RGB16_UNORM:
                return 3 * 2;
            case ImageFormat::R32_SFLOAT:
                return 4;
            case ImageFormat::R64_SFLOAT:
                return 8;
            case ImageFormat::R8_UNORM:
            case ImageFormat::R8_UINT:
                return 1;
            case ImageFormat::R16_UNORM:
            case ImageFormat::R16_UINT:
                return 2;
            case ImageFormat::R32_UINT:
            case ImageFormat::D32_SFLOAT:
                return 4;
            default:
                break;
        }

        SRHalt("Unknown format!");

        return 0;
    }

    bool IsTextureSupportsFormat(ImageFormat format) {
        switch (format) {
            case ImageFormat::RGB8_SRGB:
            case ImageFormat::RGBA8_UNORM:
            case ImageFormat::BGRA8_UNORM:
            case ImageFormat::RGBA16_UNORM:
            case ImageFormat::RGBA16_SFLOAT:
            case ImageFormat::RGB8_UNORM:
            case ImageFormat::RGB16_UNORM:
            case ImageFormat::RGBA8_SRGB:
            case ImageFormat::R8_UNORM:
            case ImageFormat::R16_UNORM:
            case ImageFormat::R32_SFLOAT:
            case ImageFormat::R64_SFLOAT:
            case ImageFormat::R8_UINT:
            case ImageFormat::R16_UINT:
            case ImageFormat::R32_UINT:
            case ImageFormat::R64_UINT:
            case ImageFormat::RG8_UNORM:
            case ImageFormat::D16_UNORM:
            case ImageFormat::D24_UNORM_S8_UINT:
            case ImageFormat::D32_SFLOAT_S8_UINT:
            case ImageFormat::D32_SFLOAT:
                return true;
            default:
                break;
        }
        return false;
    }
}
