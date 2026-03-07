//
// Created by Nikita on 28.06.2021.
//

#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Loaders/TextureLoader.h>

#include <Utils/Debug.h>
#include <Utils/Profile/TracyContext.h>

#include <Enum/TextureCompression.hpp>
#include <Enum/ImageFormat.hpp>

#ifdef SR_USE_CMP_CORE
    #include <cmp_core.h>
#endif

namespace SR_GRAPH_NS {
    void DownscaleImage2x(const uint8_t* src, uint32_t srcW, uint32_t srcH, uint8_t* dst, uint8_t channels) {
        const uint32_t dstW = std::max(1u, srcW / 2);
        const uint32_t dstH = std::max(1u, srcH / 2);

        for (uint32_t y = 0; y < dstH; ++y) {
            for (uint32_t x = 0; x < dstW; ++x) {
                uint32_t sum[4] = { 0, 0, 0, 0 };

                for (uint32_t ky = 0; ky < 2; ++ky) {
                    for (uint32_t kx = 0; kx < 2; ++kx) {
                        uint32_t sx = std::min(srcW - 1, x * 2 + kx);
                        uint32_t sy = std::min(srcH - 1, y * 2 + ky);

                        const uint8_t* p = src + (sy * srcW + sx) * static_cast<uint32_t>(channels);
                        for (uint8_t c = 0; c < channels; ++c) {
                            sum[c] += p[c];
                        }
                    }
                }

                uint8_t* d = dst + (y * dstW + x) * channels;
                for (uint32_t c = 0; c < channels; ++c) {
                    d[c] = static_cast<uint8_t>(sum[c] / 4);
                }
            }
        }
    }

    uint64_t GetCompressedImageSize(uint32_t w, uint32_t h, TextureLoadInfo info) {
        SR_TRACY_ZONE;

        const uint8_t autoMips = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;
        if (info.mips == 0 || info.mips > autoMips) {
            SRHalt("GetCompressedImageSize() : invalid mip levels! Mips: {}, auto mips: {}. Please specify the number of mip levels to generate.", info.mips, autoMips);
            return 0;
        }

        uint64_t totalSize = 0;
        uint32_t currentWidth  = w;
        uint32_t currentHeight = h;

        const size_t bytesPerBlock = (info.compression == TextureCompression::BC1 || info.compression == TextureCompression::BC4) ? 8 : 16;

        for (uint32_t i = 0; i < info.mips; ++i) {
            uint32_t blockCols = std::max(1u, (currentWidth  + 3) / 4);
            uint32_t blockRows = std::max(1u, (currentHeight + 3) / 4);

            totalSize += uint64_t(blockCols) * blockRows * bytesPerBlock;

            currentWidth  = std::max(1u, currentWidth  / 2);
            currentHeight = std::max(1u, currentHeight / 2);
        }

        return totalSize;
    }

    uint8_t* CompressImageMultithread(uint32_t w, uint32_t h, const uint8_t* pixels, TextureLoadInfo info, uint32_t maxThreads) {
        SR_TRACY_ZONE;

        if (info.compression == TextureCompression::None) {
            SRHalt("Texture compression method is None! Cannot compress image!");
            return nullptr;
        }

        if (info.mips == 0) {
            SRHalt("TextureLoadInfo.mips is zero! Cannot compress image! Please specify the number of mip levels to generate.");
            return nullptr;
        }

        SR_LOG("CompressImageMultithread() : compressing image {}x{}x{} with method {} and {} mip levels using up to {} threads...",  w, h, info.channels, info.compression, info.mips, maxThreads);

        const size_t bytesPerBlock = (info.compression == TextureCompression::BC1 || info.compression == TextureCompression::BC4) ? 8 : 16;
        const uint32_t threadCount = std::min(maxThreads, std::max(1u, std::thread::hardware_concurrency() / 2));

        uint8_t* pCmpBuffer = (uint8_t*)SRMalloc(GetCompressedImageSize(w, h, info));
        uint64_t dstOffset = 0;

        uint32_t currentWidth  = w;
        uint32_t currentHeight = h;

        std::vector<uint8_t> currentPixels(pixels, pixels + w * h * info.channels);
        std::vector<uint8_t> nextPixels;

        for (uint32_t mip = 0; mip < info.mips; ++mip) {
            SR_LOG("CompressImageMultithread() : compressing mip level {} ({}x{})...", mip, currentWidth, currentHeight);
            SR_TRACY_ZONE_N("Compress Mip Level");

            const uint32_t blockCols = std::max(1u, (currentWidth  + 3) / 4);
            const uint32_t blockRows = std::max(1u, (currentHeight + 3) / 4);

            const uint32_t rowsPerThread = blockRows / threadCount;
            const uint32_t remainder     = blockRows % threadCount;

            std::vector<std::thread> threads;
            uint32_t startRow = 0;

            for (uint32_t t = 0; t < threadCount; ++t) {
                uint32_t count  = rowsPerThread + (t < remainder ? 1 : 0);
                uint32_t endRow = startRow + count;

                threads.emplace_back([=, &currentPixels]() {
                    SR_TRACY_ZONE;
                    for (uint32_t row = startRow; row < endRow; ++row) {
                        for (uint32_t col = 0; col < blockCols; ++col) {
                            //const uint8_t* blockPtr = currentPixels.data() + (row * 4 * currentWidth + col * 4) * alignedChannels;
                            const uint8_t* blockPtr = currentPixels.data() + (row * 4) * currentWidth * 4 + (col * 4) * 4;

                            uint64_t blockIndex = row * blockCols + col;
                            uint8_t* dst = pCmpBuffer + dstOffset + blockIndex * bytesPerBlock;

                        #ifdef SR_USE_CMP_CORE
                            switch (info.compression) {
                                case TextureCompression::BC1:
                                case TextureCompression::BC4:
                                    CompressBlockBC1(blockPtr, 4 * currentWidth, dst);
                                    break;
                                default:
                                    CompressBlockBC7(blockPtr, 4 * currentWidth, dst);
                                    break;
                            }
                        #else
                            SRHalt("Texture compression is not supported! Please enable SR_USE_CMP_CORE and link cmp_core library.");
                        #endif
                        }
                    }
                });

                startRow = endRow;
            }

            for (auto& th : threads)
                th.join();

            dstOffset += uint64_t(blockCols) * blockRows * bytesPerBlock;

            // генерим следующий mip
            if (mip + 1 < info.mips) {
                uint32_t nextW = std::max(1u, currentWidth / 2);
                uint32_t nextH = std::max(1u, currentHeight / 2);

                nextPixels.resize(nextW * nextH * info.channels);
                DownscaleImage2x(currentPixels.data(), currentWidth, currentHeight, nextPixels.data(), info.channels);
                currentPixels.swap(nextPixels);

                currentWidth  = nextW;
                currentHeight = nextH;
            }
        }

        return pCmpBuffer;
    }

    uint8_t* CompressImage(uint32_t w, uint32_t h, const uint8_t *pixels, TextureLoadInfo info) {
        SR_TRACY_ZONE;
    #if defined(SR_WIN32) || defined(SR_LINUX)
        return CompressImageMultithread(w, h, pixels, info, 16);
    #else
        return CompressImageMultithread(w, h, pixels, info, 1);
    #endif
    }

    uint32_t Find4(uint32_t i) {
        SR_TRACY_ZONE;
        if (i % 4 == 0)
            return i;
        else
            return Find4(i - 1);
    }

    std::pair<uint32_t, uint32_t> MakeGoodSizes(uint32_t w, uint32_t h) {
        SR_TRACY_ZONE;
        return std::pair(Find4(w), Find4(h));
    }

    uint8_t* ResizeToLess(uint32_t ow, uint32_t oh, uint32_t nw, uint32_t nh, const uint8_t* pixels) {
        SR_TRACY_ZONE;
        auto* image = (uint8_t*)SRMalloc(nw * nh * 4);
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
                SRHalt("GetChannelCount : unsupported color format!\n\tImageFormat: " + SR_UTILS_NS::EnumReflector::ToStringAtom(format).ToStringRef());
                return 0;
            case ImageFormat::Unknown:
            default:
                SRHalt("GetChannelCount : unknown color format!\n\tImageFormat: " + SR_UTILS_NS::EnumReflector::ToStringAtom(format).ToStringRef());
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
