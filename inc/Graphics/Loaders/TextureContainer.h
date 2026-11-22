//
// Created by OpenAI Codex on 22.11.2026.
//

#ifndef SR_ENGINE_GRAPHICS_TEXTURE_CONTAINER_H
#define SR_ENGINE_GRAPHICS_TEXTURE_CONTAINER_H

#include <Graphics/Pipeline/TextureHelper.h>

#include <Utils/FileSystem/Path.h>
#include <Utils/Types/Vector.h>

namespace SR_GRAPH_NS {
    struct TextureBlockInfo {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t bytes = 0;
    };

    struct TextureMipData {
        uint32_t width = 0;
        uint32_t height = 0;
        SR_UTILS_NS::Vector<uint8_t> bytes;
    };

    struct PreparedTexture {
        uint32_t width = 0;
        uint32_t height = 0;
        ImageFormat format = ImageFormat::Unknown;
        TextureCompression compression = TextureCompression::None;
        SR_UTILS_NS::Vector<TextureMipData> mips;
    };

    namespace TextureContainer {
        SR_GRAPHICS_DLL_API TextureBlockInfo GetBlockInfo(TextureCompression compression, ImageFormat format);
        SR_GRAPHICS_DLL_API uint64_t GetLevelSize(uint32_t width, uint32_t height, TextureCompression compression, ImageFormat format);
        SR_GRAPHICS_DLL_API uint32_t GetNativeFormat(TextureCompression compression, ImageFormat format);
        SR_GRAPHICS_DLL_API bool FromNativeFormat(uint32_t nativeFormat, TextureCompression& compression, ImageFormat& format);
        SR_GRAPHICS_DLL_API SR_UTILS_NS::StringView GetExtension(TextureCompression compression, ImageFormat format);
        SR_GRAPHICS_DLL_API uint64_t HashBytes(SR_UTILS_NS::StringView bytes, uint64_t hash = 14695981039346656037ull);
        SR_GRAPHICS_DLL_API uint16_t FloatToHalf(float_t value);
        SR_GRAPHICS_DLL_API bool Read(SR_UTILS_NS::StringView bytes, PreparedTexture& texture);
        SR_GRAPHICS_DLL_API bool Read(const SR_UTILS_NS::Path& path, PreparedTexture& texture);
    }
}

#endif //SR_ENGINE_GRAPHICS_TEXTURE_CONTAINER_H
