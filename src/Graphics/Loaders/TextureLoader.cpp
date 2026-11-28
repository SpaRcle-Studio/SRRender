//
// Created by Nikita on 03.12.2020.
//

#define STB_IMAGE_IMPLEMENTATION

#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Loaders/TextureContainer.h>

#include <Utils/Common/StringUtils.h>
#include <Utils/Debug.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/FileSystem/FileSystem.h>
#include <Utils/FileSystem/VFS.h>

#include <Enum/TextureCompression.hpp>
#include <Enum/ImageFormat.hpp>

#include <stbi/stb_image.c> /// NOLINT
#include <stbi/stb_image_write.c> /// NOLINT

#ifdef SR_RENDER_USE_LIBSPNG
    #include <spng.h>
#endif

namespace SR_GRAPH_NS {
    SR_UTILS_NS::String TextureLoader::EncodeRgbPng(const uint8_t* pPixels, uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String result;
        if (!pPixels || width == 0 || height == 0 || width > 8192 || height > 8192) {
            return result;
        }
        const bool encoded = stbi_write_png_to_func([](void* pContext, void* pData, int size) {
            static_cast<SR_UTILS_NS::String*>(pContext)->append(static_cast<const char*>(pData), size);
        }, &result, width, height, 3, pPixels, width * 3) != 0;
        if (!encoded) {
            result.clear();
        }
        return result;
    }

    SR_UTILS_NS::String TextureLoader::EncodeRgbaPng(const uint8_t* pPixels, uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String result;
        if (!pPixels || width == 0 || height == 0 || width > 8192 || height > 8192) {
            return result;
        }
        const bool encoded = stbi_write_png_to_func([](void* pContext, void* pData, int size) {
            static_cast<SR_UTILS_NS::String*>(pContext)->append(static_cast<const char*>(pData), size);
        }, &result, width, height, 4, pPixels, width * 4) != 0;
        if (!encoded) {
            result.clear();
        }
        return result;
    }

    TextureData::TextureData()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    TextureData::~TextureData() {
        if (m_deleter) {
            m_deleter(m_data);
        }
    }

    uint32_t TextureData::GetNumberOfBytes() const {
        if (!m_data) {
            SRHalt("TextureData::GetNumberOfBytes() : m_data is null!");
            return 0;
        }

        if (m_info.preparedMipChain) {
            return GetCompressedImageSize(m_width, m_height, m_info);
        }

        if (!TextureLoader::IsAllowedChannelsCount(m_info.channels)) {
            SRHalt("TextureData::GetNumberOfBytes() : wrong channels count! Channels: {}, path: {}", m_info.channels, m_path);
            return 0;
        }

        return m_width * m_height * m_info.channels * (m_info.format == ImageFormat::RGBA16_SFLOAT ? 2 : 1);
    }

    bool TextureLoader::IsAllowedChannelsCount(uint8_t channels) {
        return channels == 1 || channels == 2 || channels == 4;
    }

    TextureData::Ptr TextureData::Create(uint32_t width, uint32_t height, uint8_t* pData, DeleterFn&& deleter, TextureLoadInfo info) {
        SR_TRACY_ZONE;
        TextureData::Ptr pTextureData = new TextureData();
        pTextureData->m_width = width;
        pTextureData->m_height = height;
        pTextureData->m_data = pData;
        pTextureData->m_info = info;
        pTextureData->m_deleter = std::move(deleter);
        return pTextureData;
    }

    bool TextureData::Save(const SR_UTILS_NS::Path& path) const {
        //if (!path.Create()) {
        //    SR_ERROR("TextureData::Save() : failed to create path! \nPath: \"" + path.GetFolder().ToString() + "\".");
        //    return false;
        //}

        //if (path.GetExtensionView() == "png") {
        //    return stbi_write_png(path.CStr(), m_width, m_height, m_channels, m_data, m_width * m_channels);
        //}

        //SR_ERROR("TextureData::Save() : extension is not supported! \nPath: \"" + path.ToString() + "\".");
        SRHalt("TextureData::Save() : not implemented yet!");
        return false;
    }

    uint8_t TextureData::GetChannels() const {
        return m_info.channels;
    }

    TextureData::Ptr TextureData::CreateEmpty() {
        return new TextureData();
    }

    void TextureData::SetData(uint32_t width, uint32_t height, uint8_t* pData, TextureData::DeleterFn&& deleter, TextureLoadInfo info) {
        m_width = width;
        m_height = height;
        m_data = pData;
        m_info = info;
        m_deleter = std::move(deleter);
    }

    TextureData::Ptr TextureLoader::LoadHalf(SR_UTILS_NS::StringView buffer, TextureLoadInfo info) {
        TextureSourceInfo sourceInfo;
        SR_UTILS_NS::Vector<float_t> pixels;
        if (!DecodeSource(buffer, sourceInfo, pixels)) {
            return nullptr;
        }
        auto&& pBytes = static_cast<uint8_t*>(SRMalloc(pixels.size() * sizeof(uint16_t)));
        if (!pBytes) {
            return nullptr;
        }
        for (uint64_t i = 0; i < pixels.size(); ++i) {
            const uint16_t half = TextureContainer::FloatToHalf(info.disableAlphaChannel && i % 4 == 3 ? 1.f : pixels[i]);
            memcpy(pBytes + i * sizeof(half), &half, sizeof(half));
        }
        info.compression = TextureCompression::None;
        auto pTextureData = TextureData::Create(sourceInfo.width, sourceInfo.height, pBytes, [](uint8_t* pData) {
            SRFree(pData);
        }, std::move(info));
        return pTextureData;
    }

    TextureData::Ptr TextureLoader::LoadPrepared(const SR_UTILS_NS::Path& path, const TextureLoadInfo& info) {
        SR_TRACY_ZONE;
        const auto root = SR_UTILS_NS::ResourceManager::Instance().GetResPath();
        if (info.preparedVariants.size() != info.preparedHashes.size()) {
            SR_ERROR("TextureLoader::LoadPrepared() : incomplete variant manifest for {}", path);
            return nullptr;
        }
        for (uint32_t index = 0; index < info.preparedVariants.size(); ++index) {
            const auto variant = info.preparedVariants[index];
            PreparedTexture texture;
            SR_UTILS_NS::String bytes;
            if (!SR_UTILS_NS::FileSystem::ReadFile(root.Concat(variant), bytes)
                || TextureContainer::HashBytes(bytes) != info.preparedHashes[index]) {
                SR_WARN("TextureLoader::LoadPrepared() : invalid prepared texture: {}", variant);
                continue;
            }
            if (variant == path) {
                TextureLoadInfo sourceInfo = info;
                sourceInfo.preparedVariants.clear();
                sourceInfo.preparedHashes.clear();
                sourceInfo.channels = GetAlignedChannels(sourceInfo.format);
                return LoadSource(path, bytes, std::move(sourceInfo));
            }
            if (!TextureContainer::Read(SR_UTILS_NS::StringView(bytes), texture)) {
                SR_WARN("TextureLoader::LoadPrepared() : invalid KTX2 texture: {}", variant);
                continue;
            }
            const uint32_t nativeFormat = TextureContainer::GetNativeFormat(texture.compression, texture.format);
            const auto block = TextureContainer::GetBlockInfo(texture.compression, texture.format);
            if (info.requireAlignedBlocks && (texture.width % block.width != 0 || texture.height % block.height != 0)) {
                continue;
            }
            bool supported = false;
            for (const uint32_t candidate : info.supportedFormats) {
                supported |= candidate == nativeFormat;
            }
            if (!supported) {
                continue;
            }
            uint64_t size = 0;
            for (auto&& mip : texture.mips) {
                size += mip.bytes.size();
            }
            auto&& pBytes = static_cast<uint8_t*>(SRMalloc(size));
            if (!pBytes) {
                return nullptr;
            }
            uint64_t offset = 0;
            for (auto&& mip : texture.mips) {
                memcpy(pBytes + offset, mip.bytes.data(), mip.bytes.size());
                offset += mip.bytes.size();
            }
            TextureLoadInfo preparedInfo;
            preparedInfo.compression = texture.compression;
            preparedInfo.format = texture.format;
            preparedInfo.channels = GetChannelCount(texture.format);
            preparedInfo.mips = static_cast<uint32_t>(texture.mips.size());
            preparedInfo.preparedMipChain = true;
            auto pTextureData = TextureData::Create(texture.width, texture.height, pBytes, [](uint8_t* pData) {
                SRFree(pData);
            }, std::move(preparedInfo));
            pTextureData->SetPath(path);
            return pTextureData;
        }
        SR_ERROR("TextureLoader::LoadPrepared() : no valid supported variant for {}", path);
        return nullptr;
    }

    TextureData::Ptr TextureLoader::Load(const SR_UTILS_NS::Path& path, TextureLoadInfo info) {
        SR_TRACY_ZONE;
        if (!info.preparedVariants.empty()) {
            return LoadPrepared(path, info);
        }
        const auto fullPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
        SR_UTILS_NS::String buffer;
        if (!SR_UTILS_NS::FileSystem::ReadFile(fullPath, buffer) || !IsAllowedChannelsCount(info.channels)) {
            SR_ERROR("TextureLoader::Load() : failed to read source texture: {}", path);
            return nullptr;
        }
        return LoadSource(path, buffer, std::move(info));
    }

    TextureData::Ptr TextureLoader::LoadSource(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringView buffer, TextureLoadInfo info) {
        SR_TRACY_ZONE;
        if (buffer == TextureContainer::SourcePlaceholder) {
            if (!info.preparedVariants.empty()) {
                return LoadPrepared(path, info);
            }
            SR_ERROR("TextureLoader::LoadSource() : compressed texture placeholder has no prepared variant manifest: {}", path);
            return nullptr;
        }
        const auto fullPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
        if (!IsAllowedChannelsCount(info.channels)) {
            return nullptr;
        }
        if (info.format == ImageFormat::RGBA16_SFLOAT) {
            auto pTextureData = LoadHalf(buffer, info);
            if (pTextureData) {
                pTextureData->SetPath(path);
            }
            return pTextureData;
        }
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t channels = 0;
        auto&& pSource = LoadRaw(buffer, width, height, channels, fullPath, info.loadMode);
        if (!pSource) {
            return nullptr;
        }
        const uint64_t pixelCount = uint64_t(width) * height;
        auto&& pPixels = static_cast<uint8_t*>(SRMalloc(pixelCount * info.channels));
        if (!pPixels) {
            Free(pSource, info.loadMode);
            return nullptr;
        }
        for (uint64_t pixel = 0; pixel < pixelCount; ++pixel) {
            for (uint32_t channel = 0; channel < info.channels; ++channel) {
                pPixels[pixel * info.channels + channel] = pSource[pixel * 4 + channel];
            }
            if (info.disableAlphaChannel && info.channels == 4) {
                pPixels[pixel * 4 + 3] = 255;
            }
        }
        Free(pSource, info.loadMode);
        uint32_t maxMips = 1;
        for (uint32_t dimension = std::max(width, height); dimension > 1; dimension /= 2) {
            ++maxMips;
        }
        info.mips = info.mips == 0 ? maxMips : std::min(info.mips, maxMips);
        info.compression = TextureCompression::None;
        auto pTextureData = TextureData::Create(width, height, pPixels, [](uint8_t* pData) {
            SRFree(pData);
        }, std::move(info));
        pTextureData->SetPath(path);
        return pTextureData;
    }

    bool TextureLoader::ReadSourceInfo(SR_UTILS_NS::StringView bytes, TextureSourceInfo& info) {
        if (bytes.empty() || bytes.size() > SR_INT32_MAX) {
            return false;
        }
        const auto&& pBytes = reinterpret_cast<const stbi_uc*>(bytes.data());
        int32_t width = 0;
        int32_t height = 0;
        int32_t channels = 0;
        if (!stbi_info_from_memory(pBytes, static_cast<int32_t>(bytes.size()), &width, &height, &channels)
            || width <= 0 || height <= 0 || width > 32768 || height > 32768
            || uint64_t(width) * height * 16 > SR_UINT32_MAX) {
            return false;
        }
        info.width = width;
        info.height = height;
        info.channels = channels;
        info.hdr = stbi_is_hdr_from_memory(pBytes, static_cast<int32_t>(bytes.size())) != 0;
        info.sixteenBit = stbi_is_16_bit_from_memory(pBytes, static_cast<int32_t>(bytes.size())) != 0;
        return true;
    }

    bool TextureLoader::DecodeSource(SR_UTILS_NS::StringView bytes, TextureSourceInfo& info, SR_UTILS_NS::Vector<float_t>& pixels) {
        SR_TRACY_ZONE;
        if (!ReadSourceInfo(bytes, info)) {
            return false;
        }
        const auto&& pBytes = reinterpret_cast<const stbi_uc*>(bytes.data());
        int32_t width = 0;
        int32_t height = 0;
        int32_t channels = 0;
        pixels.resize(uint64_t(info.width) * info.height * 4);
        if (info.hdr) {
            auto&& pPixels = stbi_loadf_from_memory(pBytes, static_cast<int32_t>(bytes.size()), &width, &height, &channels, 4);
            if (!pPixels) {
                return false;
            }
            memcpy(pixels.data(), pPixels, pixels.size() * sizeof(float_t));
            stbi_image_free(pPixels);
        }
        else if (info.sixteenBit) {
            auto&& pPixels = stbi_load_16_from_memory(pBytes, static_cast<int32_t>(bytes.size()), &width, &height, &channels, 4);
            if (!pPixels) {
                return false;
            }
            for (uint64_t i = 0; i < pixels.size(); ++i) {
                pixels[i] = pPixels[i] / 65535.f;
            }
            stbi_image_free(pPixels);
        }
        else {
            auto&& pPixels = stbi_load_from_memory(pBytes, static_cast<int32_t>(bytes.size()), &width, &height, &channels, 4);
            if (!pPixels) {
                return false;
            }
            for (uint64_t i = 0; i < pixels.size(); ++i) {
                pixels[i] = pPixels[i] / 255.f;
            }
            stbi_image_free(pPixels);
        }
        for (const float_t value : pixels) {
            if (!std::isfinite(value)) {
                return false;
            }
        }
        return true;
    }

    bool TextureLoader::Free(uint8_t* data, TextureLoadMode mode) {
        SR_TRACY_ZONE;

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("TextureLoader::Free() : free source image data...");
        }

        if (data) {
            if (mode == TextureLoadMode::LibsPNG) {
                SRFree(data);
            }
            else {
                stbi_image_free(data);
            }
        }
        else {
            SR_ERROR("TextureLoader::Free() : data is nullptr!");
            return false;
        }

        return true;
    }

    TextureData::Ptr TextureLoader::LoadFromMemory(const SR_UTILS_NS::String& data, const ImageMetaInfo& config) {
        SR_TRACY_ZONE;

        if (config.format == ImageFormat::RGBA16_SFLOAT) {
            TextureLoadInfo info;
            info.format = config.format;
            info.channels = 4;
            info.mips = config.mipLevels;
            info.disableAlphaChannel = config.disableAlphaChannel;
            return LoadHalf(data, std::move(info));
        }
        int32_t width = 0, height = 0, channels = 0;

        const uint8_t requireChannels = GetAlignedChannels(config.format);
        if (requireChannels == 0) {
            SRHalt("TextureLoader::LoadFromMemory() : wrong load format! Format: ", config.format);
            return nullptr;
        }

        stbi_set_unpremultiply_on_load(0);
        stbi_convert_iphone_png_to_rgb(0);
        uint8_t* pImgData = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.c_str()),
            static_cast<int32_t>(data.size()), &width, &height, &channels, requireChannels
        );

        if (!pImgData) {
            SR_UTILS_NS::String reason;

            if (stbi_failure_reason()) {
                reason = stbi_failure_reason();
            }

            SR_ERROR("TextureLoader::LoadFromMemory() : can not load texture from memory!\n\tReason: " + reason);

            return nullptr;
        }

        TextureLoadInfo info;
        info.channels = requireChannels;
        info.format = config.format;
        info.mips = config.mipLevels;

        auto&& pTextureData = TextureData::Create(width, height, pImgData, [](uint8_t* pData) {
            TextureLoader::Free(pData, TextureLoadMode::STBImage);
        }, info);

        return pTextureData;
    }

    int TextureLoader::GetAlignedChannels(ImageFormat format) {
        const int channels = GetChannelCount(format);
        switch (channels) {
            case 1: return STBI_grey;
            case 2: return STBI_grey_alpha;
            case 4: return STBI_rgb_alpha;
            default:
                SRHalt("TextureLoader::GetAlignedChannels() : unsupported number of channels! Number of channels must be 1, 2 or 4! Format: {}", format);
                return 0;
        }
    }

    uint8_t* TextureLoader::LoadRaw(SR_UTILS_NS::StringView buffer, uint32_t& width, uint32_t& height, uint8_t& channels,  const SR_UTILS_NS::Path& path, TextureLoadMode mode) {
        SR_TRACY_ZONE;

        uint8_t* pImgData = nullptr;

        if (mode == TextureLoadMode::STBImage) {
            SR_TRACY_ZONE_N("STBImage load");
            stbi_set_unpremultiply_on_load(0);
            stbi_convert_iphone_png_to_rgb(0);

            int32_t w = 0, h = 0, c = 0;
            pImgData = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer.data()), static_cast<int32_t>(buffer.size()), &w, &h, &c, 4);
            width = static_cast<uint32_t>(w);
            height = static_cast<uint32_t>(h);
            channels = c;

            if (!pImgData) {
                auto&& reason = stbi_failure_reason() ? stbi_failure_reason() : nullptr;
                SR_ERROR("TextureLoader::Load() : can not load \"{}\" file!\n\tReason: {}", path, reason);
                return nullptr;
            }
        }
    #ifdef SR_RENDER_USE_LIBSPNG
        else if (mode == TextureLoadMode::LibsPNG) {
            SR_TRACY_ZONE_N("LibsPNG load");
            spng_ctx* ctx = spng_ctx_new(0);
            if (!ctx) {
                SR_ERROR("TextureLoader::Load() : failed to create spng context!");
                return nullptr;
            }

            if (spng_set_png_buffer(ctx, buffer.data(), buffer.size()) != 0) {
                SR_ERROR("TextureLoader::Load() : failed to set png buffer!");
                spng_ctx_free(ctx);
                return nullptr;
            }

            spng_ihdr ihdr;
            if (spng_get_ihdr(ctx, &ihdr) != 0) {
                SR_ERROR("TextureLoader::Load() : failed to get ihdr!");
                spng_ctx_free(ctx);
                return nullptr;
            }

            width = ihdr.width;
            height = ihdr.height;
            channels = ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA ? 4 : (ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA ? 2 : 1);

            size_t out_size = 0;
            if (spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size) != 0) {
                SR_ERROR("TextureLoader::Load() : failed to get decoded image size!");
                spng_ctx_free(ctx);
                return nullptr;
            }

            pImgData = (uint8_t*)SRMalloc(out_size);
            if (!pImgData) {
                SR_ERROR("TextureLoader::Load() : failed to allocate memory for image data!");
                spng_ctx_free(ctx);
                return nullptr;
            }

            {
                SR_TRACY_ZONE_N("Decode image");
                if (spng_decode_image(ctx, pImgData, out_size, SPNG_FMT_RGBA8, 0) != 0) {
                    SR_ERROR("TextureLoader::Load() : failed to decode image!");
                    SRFree(pImgData);
                    spng_ctx_free(ctx);
                    return nullptr;
                }
            }

            spng_ctx_free(ctx);
        }
    #endif
        else {
            SRHalt("TextureLoader::Load() : unsupported load mode! Mode: {}", static_cast<uint32_t>(mode));
            return nullptr;
        }

        return pImgData;
    }
}

