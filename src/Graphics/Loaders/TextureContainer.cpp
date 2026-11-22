//
// Created by OpenAI Codex on 22.11.2026.
//

#include <Graphics/Loaders/TextureContainer.h>

#include <Utils/FileSystem/FileSystem.h>
#include <Utils/Profile/TracyContext.h>

#ifdef SR_COMMON_ZLIB
    #include <zlib/zlib.h>
#endif

namespace SR_GRAPH_NS::TextureContainer {
    struct NativeFormatEntry {
        TextureCompression compression;
        uint32_t linear;
        uint32_t srgb;
        ImageFormat format;
    };

    static constexpr NativeFormatEntry Formats[] = {
        { TextureCompression::None, 37, 43, ImageFormat::RGBA8_UNORM },
        { TextureCompression::None, 97, 97, ImageFormat::RGBA16_SFLOAT },
        { TextureCompression::BC1, 133, 134, ImageFormat::RGBA8_UNORM },
        { TextureCompression::BC2, 135, 136, ImageFormat::RGBA8_UNORM },
        { TextureCompression::BC3, 137, 138, ImageFormat::RGBA8_UNORM },
        { TextureCompression::BC4, 139, 139, ImageFormat::R8_UNORM },
        { TextureCompression::BC5, 141, 141, ImageFormat::RG8_UNORM },
        { TextureCompression::BC6, 144, 144, ImageFormat::RGBA16_SFLOAT },
        { TextureCompression::BC7, 145, 146, ImageFormat::RGBA8_UNORM },
        { TextureCompression::ASTC4x4, 157, 158, ImageFormat::RGBA8_UNORM },
        { TextureCompression::ASTC6x6, 165, 166, ImageFormat::RGBA8_UNORM },
        { TextureCompression::ASTC8x8, 171, 172, ImageFormat::RGBA8_UNORM },
        { TextureCompression::ETC2RGB, 147, 148, ImageFormat::RGBA8_UNORM },
        { TextureCompression::ETC2RGBA, 151, 152, ImageFormat::RGBA8_UNORM },
        { TextureCompression::EACR11, 153, 153, ImageFormat::R8_UNORM },
        { TextureCompression::EACRG11, 155, 155, ImageFormat::RG8_UNORM }
    };

    TextureBlockInfo GetBlockInfo(TextureCompression compression, ImageFormat format) {
        if (compression == TextureCompression::None) {
            return { 1, 1, format == ImageFormat::RGBA16_SFLOAT ? 8u : 4u };
        }
        if (compression == TextureCompression::ASTC6x6) {
            return { 6, 6, 16 };
        }
        if (compression == TextureCompression::ASTC8x8) {
            return { 8, 8, 16 };
        }
        const bool smallBlock = compression == TextureCompression::BC1 || compression == TextureCompression::BC4
            || compression == TextureCompression::ETC2RGB || compression == TextureCompression::EACR11;
        return { 4, 4, smallBlock ? 8u : 16u };
    }

    uint64_t GetLevelSize(uint32_t width, uint32_t height, TextureCompression compression, ImageFormat format) {
        const auto block = GetBlockInfo(compression, format);
        return ((uint64_t(width) + block.width - 1) / block.width)
            * ((uint64_t(height) + block.height - 1) / block.height) * block.bytes;
    }

    uint32_t GetNativeFormat(TextureCompression compression, ImageFormat format) {
        for (const NativeFormatEntry& entry : Formats) {
            if (entry.compression != compression || (compression == TextureCompression::None
                && entry.format != format && !(entry.format == ImageFormat::RGBA8_UNORM && format == ImageFormat::RGBA8_SRGB))) {
                continue;
            }
            return format == ImageFormat::RGBA8_SRGB ? entry.srgb : entry.linear;
        }
        return 0;
    }

    bool FromNativeFormat(uint32_t nativeFormat, TextureCompression& compression, ImageFormat& format) {
        for (const NativeFormatEntry& entry : Formats) {
            if (entry.linear == nativeFormat || entry.srgb == nativeFormat) {
                compression = entry.compression;
                format = nativeFormat == entry.srgb && entry.srgb != entry.linear ? ImageFormat::RGBA8_SRGB : entry.format;
                return true;
            }
        }
        return false;
    }

    SR_UTILS_NS::StringView GetExtension(TextureCompression compression, ImageFormat format) {
        switch (compression) {
            case TextureCompression::BC1: return ".bc1";
            case TextureCompression::BC2: return ".bc2";
            case TextureCompression::BC3: return ".bc3";
            case TextureCompression::BC4: return ".bc4";
            case TextureCompression::BC5: return ".bc5";
            case TextureCompression::BC6: return ".bc6h";
            case TextureCompression::BC7: return ".bc7";
            case TextureCompression::ASTC4x4:
            case TextureCompression::ASTC6x6:
            case TextureCompression::ASTC8x8: return ".astc";
            case TextureCompression::ETC2RGB:
            case TextureCompression::ETC2RGBA: return ".etc2";
            case TextureCompression::EACR11:
            case TextureCompression::EACRG11: return ".eac";
            default: return format == ImageFormat::RGBA16_SFLOAT ? ".rgba16f" : ".rgba8";
        }
    }

    uint64_t HashBytes(SR_UTILS_NS::StringView bytes, uint64_t hash) {
        for (const char value : bytes) {
            hash = (hash ^ static_cast<uint8_t>(value)) * 1099511628211ull;
        }
        return hash;
    }

    uint16_t FloatToHalf(float_t value) {
        uint32_t bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        const uint32_t sign = (bits >> 16) & 0x8000;
        const int32_t exponent = int32_t((bits >> 23) & 255) - 127 + 15;
        uint32_t mantissa = bits & 0x7fffff;
        if (exponent <= 0) {
            if (exponent < -10) {
                return static_cast<uint16_t>(sign);
            }
            mantissa = (mantissa | 0x800000) >> (1 - exponent);
            return static_cast<uint16_t>(sign | ((mantissa + 0xfff + ((mantissa >> 13) & 1)) >> 13));
        }
        if (exponent >= 31) {
            return static_cast<uint16_t>(sign | 0x7bff);
        }
        const uint32_t rounded = mantissa + 0xfff + ((mantissa >> 13) & 1);
        return static_cast<uint16_t>(sign | std::min(0x7bffu, (uint32_t(exponent) << 10) + (rounded >> 13)));
    }

    static uint64_t ReadInteger(SR_UTILS_NS::StringView bytes, uint64_t offset, uint32_t count) {
        uint64_t value = 0;
        for (uint32_t i = 0; i < count; ++i) {
            value |= uint64_t(static_cast<uint8_t>(bytes[offset + i])) << (i * 8);
        }
        return value;
    }

    bool Read(SR_UTILS_NS::StringView bytes, PreparedTexture& texture) {
        SR_TRACY_ZONE;
        texture = PreparedTexture();
        static constexpr uint8_t identifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
        if (bytes.size() < 104 || memcmp(bytes.data(), identifier, sizeof(identifier)) != 0) {
            return false;
        }
        const uint32_t nativeFormat = ReadInteger(bytes, 12, 4);
        const uint32_t width = ReadInteger(bytes, 20, 4);
        const uint32_t height = ReadInteger(bytes, 24, 4);
        const uint32_t levels = ReadInteger(bytes, 40, 4);
        const uint32_t supercompression = ReadInteger(bytes, 44, 4);
        if (width == 0 || height == 0 || width > 65536 || height > 65536 || levels == 0 || levels > 17
            || ReadInteger(bytes, 28, 4) != 0 || ReadInteger(bytes, 32, 4) != 0
            || ReadInteger(bytes, 36, 4) != 1 || (supercompression != 0 && supercompression != 3)
            || !FromNativeFormat(nativeFormat, texture.compression, texture.format)
            || bytes.size() < 80ull + levels * 24ull) {
            return false;
        }
    #ifndef SR_COMMON_ZLIB
        if (supercompression == 3) {
            return false;
        }
    #endif
        uint32_t maxLevels = 1;
        for (uint32_t size = std::max(width, height); size > 1; size /= 2) {
            ++maxLevels;
        }
        const uint64_t dfdOffset = ReadInteger(bytes, 48, 4);
        const uint64_t dfdSize = ReadInteger(bytes, 52, 4);
        const uint64_t kvdOffset = ReadInteger(bytes, 56, 4);
        const uint64_t kvdSize = ReadInteger(bytes, 60, 4);
        const uint64_t indexEnd = 80ull + levels * 24ull;
        if (levels > maxLevels || dfdOffset < indexEnd || dfdOffset > bytes.size()
            || dfdSize < 28 || dfdSize > bytes.size() - dfdOffset
            || ReadInteger(bytes, dfdOffset, 4) != dfdSize
            || (kvdSize && (kvdOffset < dfdOffset + dfdSize || kvdOffset > bytes.size() || kvdSize > bytes.size() - kvdOffset))
            || ReadInteger(bytes, 64, 8) != 0 || ReadInteger(bytes, 72, 8) != 0) {
            return false;
        }
        const uint64_t dataStart = std::max(dfdOffset + dfdSize, kvdOffset + kvdSize);
        const auto block = GetBlockInfo(texture.compression, texture.format);
        const uint32_t typeSize = texture.compression == TextureCompression::None && texture.format == ImageFormat::RGBA16_SFLOAT ? 2 : 1;
        if (ReadInteger(bytes, 16, 4) != typeSize
            || ReadInteger(bytes, dfdOffset + 8, 2) != 2
            || ReadInteger(bytes, dfdOffset + 10, 2) != dfdSize - 4
            || ReadInteger(bytes, dfdOffset + 16, 1) + 1 != block.width
            || ReadInteger(bytes, dfdOffset + 17, 1) + 1 != block.height
            || ReadInteger(bytes, dfdOffset + 20, 1) != block.bytes) {
            return false;
        }
        uint64_t previousOffset = bytes.size();
        uint64_t totalSize = 0;
        uint32_t mipWidth = width;
        uint32_t mipHeight = height;
        for (uint32_t level = 0; level < levels; ++level) {
            const uint64_t index = 80ull + level * 24ull;
            const uint64_t offset = ReadInteger(bytes, index, 8);
            const uint64_t size = ReadInteger(bytes, index + 8, 8);
            const uint64_t expected = GetLevelSize(mipWidth, mipHeight, texture.compression, texture.format);
            if (offset < dataStart || offset > bytes.size()
                || (supercompression == 0 && (offset % std::max(4u, block.bytes) != 0 || size != expected))
                || size == 0 || size > bytes.size() - offset
                || ReadInteger(bytes, index + 16, 8) != expected || offset + size > previousOffset
                || size > SR_UINT32_MAX || expected > SR_UINT32_MAX || totalSize > SR_UINT32_MAX - expected) {
                texture = PreparedTexture();
                return false;
            }
            previousOffset = offset;
            totalSize += expected;
            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }
        texture.width = width;
        texture.height = height;
        mipWidth = width;
        mipHeight = height;
        for (uint32_t level = 0; level < levels; ++level) {
            auto&& mip = texture.mips.emplace_back();
            mip.width = mipWidth;
            mip.height = mipHeight;
            const uint64_t offset = ReadInteger(bytes, 80ull + level * 24ull, 8);
            const uint64_t size = ReadInteger(bytes, 88ull + level * 24ull, 8);
            const uint64_t unpackedSize = ReadInteger(bytes, 96ull + level * 24ull, 8);
            mip.bytes.resize(unpackedSize);
            if (supercompression == 0) {
                memcpy(mip.bytes.data(), bytes.data() + offset, size);
            }
        #ifdef SR_COMMON_ZLIB
            else {
                z_stream stream{};
                stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(bytes.data() + offset));
                stream.avail_in = static_cast<uInt>(size);
                stream.next_out = mip.bytes.data();
                stream.avail_out = static_cast<uInt>(unpackedSize);
                if (inflateInit(&stream) != Z_OK) {
                    texture = PreparedTexture();
                    return false;
                }
                const int32_t result = inflate(&stream, Z_FINISH);
                const bool valid = result == Z_STREAM_END && stream.total_out == unpackedSize && stream.total_in == size;
                inflateEnd(&stream);
                if (!valid) {
                    texture = PreparedTexture();
                    return false;
                }
            }
        #endif
            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }
        return true;
    }

    bool Read(const SR_UTILS_NS::Path& path, PreparedTexture& texture) {
        SR_UTILS_NS::String bytes;
        return SR_UTILS_NS::FileSystem::ReadFile(path, bytes) && Read(SR_UTILS_NS::StringView(bytes), texture);
    }
}
