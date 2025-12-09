//
// Created by Nikita on 28.06.2021.
//

#include <Graphics/Pipeline/TextureHelper.h>
#include <Utils/Debug.h>

#include <cmp_core.h>

namespace SR_GRAPH_NS {
    uint8_t* Compress(uint32_t w, uint32_t h, const uint8_t *pixels, TextureCompression method) {
        uint32_t blockCount = (w / 4) * (h / 4);
        auto* cmpBuffer = (uint8_t*)malloc(16 * blockCount * 4);
        for (uint32_t col = 0; col < w / 4; col++) {
            for (uint32_t row = 0; row < h / 4; row++) {
                uint32_t colOffs = col * 16;
                uint32_t rowOffs = row * w;

                switch (method) {
                    case TextureCompression::None:
                        return nullptr;
                    case TextureCompression::BC1:
                    case TextureCompression::BC4:
                        //! BC1, BC4 - has 8-byte cmp buffer
                        CompressBlockBC1(
                                pixels + colOffs + rowOffs * 16,            // source
                                4 * w,                                      // count bytes
                                cmpBuffer + colOffs / 2 + (rowOffs * 4) / 2 // dst
                        );
                        break;
                    case TextureCompression::BC2:
                    case TextureCompression::BC3:
                    case TextureCompression::BC5:
                    case TextureCompression::BC6:
                    case TextureCompression::BC7:
                        //! other BC has 16-byte cmp buffer
                        CompressBlockBC7(
                                pixels + colOffs + rowOffs * 16,    // source
                                4 * w,                              // count bytes
                                cmpBuffer + colOffs+ (rowOffs * 4)  // dst
                        );
                        break;
                    default:
                        break;
                }
            }
        }

        return cmpBuffer;
    }

    uint32_t Find4(uint32_t i) {
        if (i % 4 == 0)
            return i;
        else
            return Find4(i - 1);
    }

    std::pair<uint32_t, uint32_t> MakeGoodSizes(uint32_t w, uint32_t h) {
        return std::pair(Find4(w), Find4(h));
    }

    uint8_t* ResizeToLess(uint32_t ow, uint32_t oh, uint32_t nw, uint32_t nh, const uint8_t* pixels) {
        auto* image = (uint8_t*)malloc(nw * nh * 4);
        uint32_t dw = ow - nw;

        for (uint32_t row = 0; row < nh; ++row) {
            memcpy(image + (nw * 4 * row), pixels + (dw * 4 * row) + (nw * 4 * row), nw * 4);
        }

        return image;
    }

    uint8_t GetChannelCount(ImageFormat format) {
        switch (format) {
            case ImageFormat::RGBA8_UNORM:
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
                SR_ERROR("GetChannelCount : unsupported color format!\n\tImageFormat: " + SR_UTILS_NS::EnumReflector::ToStringAtom(format).ToStringRef());
                return 0;
            case ImageFormat::Unknown:
            default:
                SR_ERROR("GetChannelCount : unknown color format!\n\tImageFormat: " + SR_UTILS_NS::EnumReflector::ToStringAtom(format).ToStringRef());
                return 0;
        }
    }

    uint32_t GetPixelSize(ImageFormat format) {
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
            default:
                break;
        }

        SRHalt("Unknown format!");

        return 0;
    }

    bool IsTextureSupportsFormat(ImageFormat format) {
        switch (format) {
            case ImageFormat::RGBA8_UNORM:
            case ImageFormat::BGRA8_UNORM:
            case ImageFormat::RGBA8_SRGB:
            case ImageFormat::RGBA16_UNORM:
            case ImageFormat::RGBA16_SFLOAT:
            case ImageFormat::RGB8_UNORM:
            case ImageFormat::RGB8_SRGB:
            case ImageFormat::RGB16_UNORM:
            case ImageFormat::R32_SFLOAT:
                return true;
            default:
                break;
        }
        return false;
    }
}
