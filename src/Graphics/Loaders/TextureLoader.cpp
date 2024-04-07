//
// Created by Nikita on 03.12.2020.
//

#define STB_IMAGE_IMPLEMENTATION

#include <Utils/Common/StringUtils.h>
#include <Utils/Common/ToString.h>
#include <Utils/Debug.h>

#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Types/Texture.h>

#include <stbi/stb_image.c> /// NOLINT
#include <stbi/stbi_image_write.c> /// NOLINT

namespace SR_GRAPH_NS {
    TextureData::TextureData(uint32_t width, uint32_t height, uint8_t channels, uint8_t* data, ImageLoadFormat format)
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_width(width)
        , m_height(height)
        , m_channels(channels)
        , m_data(data)
        , m_format(format)
    { }

    TextureData::~TextureData() {
        m_deleter(m_data);
    }

    TextureData::Ptr TextureData::Load(const SR_UTILS_NS::Path& path, ImageLoadFormat format) {
        int32_t stbiFormat = 0;
        switch(format) {
            case ImageLoadFormat::Grey: stbiFormat = STBI_grey; break;
            case ImageLoadFormat::GreyAlpha: stbiFormat = STBI_grey_alpha; break;
            case ImageLoadFormat::RGB: stbiFormat = STBI_rgb; break;
            case ImageLoadFormat::RGBA: stbiFormat = STBI_rgb_alpha; break;
            default: SRHalt("TextureData::Load() : wrong load format."); return nullptr;
        }

        int width = 0, height = 0, channels = 0;
        uint8_t* imageData = stbi_load(path.CStr(), &width, &height, &channels, stbiFormat);

        if (!imageData) {
            auto&& reason = stbi_failure_reason() ? stbi_failure_reason() : std::string();
            SR_ERROR("TextureData::Load() : cannot load texture by path \"" + path.ToString() + "\".\n\tReason: " + reason);
            return nullptr;
        }

        auto&& pTextureData = new TextureData(width, height, channels, imageData, format);
        pTextureData->m_path = path;
        pTextureData->m_deleter = [](uint8_t* pData) {
            if (!TextureLoader::Free(pData)) {
                SR_ERROR("TextureData::Load() : failed to free pData.");
            }
        };

        return pTextureData;
    }

    TextureData::Ptr TextureData::Create(uint32_t width, uint32_t height, uint8_t* pData, DeleterFn&& deleter, ImageLoadFormat format) {
        uint8_t channels = 0;
        switch(format) {
            case ImageLoadFormat::Grey: channels = 1; break;
            case ImageLoadFormat::GreyAlpha: channels = 2; break;
            case ImageLoadFormat::RGB: channels = 3; break;
            case ImageLoadFormat::RGBA: channels = 4; break;
            default: SRHalt("TextureData::Create() : wrong load format."); return nullptr;
        }

        auto&& pTextureData = new TextureData(width, height, channels, pData, format);
        pTextureData->m_deleter = std::move(deleter);

        return pTextureData;
    }

    bool TextureData::Save(const SR_UTILS_NS::Path& path) const {
        auto result = stbi_write_png(path.CStr(), m_width, m_height, m_channels, m_data, m_width * m_channels);
        if (!result) {
            return false;
        }

        return true;
    }


    TextureData::Ptr TextureLoader::Load(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        const bool cacheEnabled = SR_UTILS_NS::Features::Instance().Enabled("TextureCaching", true);
        auto&& cache = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Textures");

        const uint64_t hashName = cacheEnabled ? SR_HASH(path.ConvertToFileName()) : 0;
        uint64_t fileHash = cacheEnabled ? path.GetFileHash() : 0;

        if (cacheEnabled) {
            auto&& stringHash = SR_UTILS_NS::ToString(hashName);
            auto&& cachePath = cache.Concat(path.GetBaseNameAndExt() + "." + stringHash);
            auto&& cacheHashPath = cachePath.ConcatExt(".cache.hash");

            if (cacheHashPath.Exists(SR_UTILS_NS::Path::Type::File)) {
                if (SR_UTILS_NS::FileSystem::ReadHashFromFile(cacheHashPath) == fileHash) {
                    auto&& pTextureData = LoadFromCache(cachePath.ConcatExt(".cache"));
                    if (pTextureData) {
                        return pTextureData;
                    }
                }
            }
        }

        int32_t width = 0, height = 0, numComponents = 0;
        uint8_t* pImgData = stbi_load(path.c_str(), &width, &height, &numComponents, STBI_rgb_alpha);

        if (!pImgData) {
            std::string reason = stbi_failure_reason() ? stbi_failure_reason() : std::string();
            SR_ERROR("TextureLoader::Load() : can not load \"" + path.ToStringRef() + "\" file!\n\tReason: " + reason);
            return nullptr;
        }

        const ImageLoadFormat format = numComponents == 4 ? ImageLoadFormat::RGBA : ImageLoadFormat::RGB;

        if (cacheEnabled) {
            SR_LOG("TextureLoader::Load() : save texture to cache...");

            auto&& stringHash = SR_UTILS_NS::ToString(hashName);
            auto&& cachePath = cache.Concat(path.GetBaseNameAndExt() + "." + stringHash);
            auto&& cacheHashPath = cachePath.ConcatExt(".cache.hash");
            auto&& cacheFilePath = cachePath.ConcatExt(".cache");

            if (!cacheHashPath.Exists(SR_UTILS_NS::Path::Type::File)) {
                if (!SR_UTILS_NS::FileSystem::WriteHashToFile(cacheHashPath, fileHash)) {
                    SR_ERROR("TextureLoader::Load() : failed to write hash to file \"" + cacheHashPath.ToStringRef() + "\"!");
                }
            }

            auto&& marshal = SR_HTYPES_NS::Marshal();
            marshal.Write<std::string>(path.ToStringRef());
            marshal.Write<uint32_t>(width);
            marshal.Write<uint32_t>(height);
            marshal.Write(static_cast<uint8_t>(format));
            marshal.WriteBlock(pImgData, width * height * 4 * sizeof(uint8_t));

            if (!marshal.Save(cacheFilePath)) {
                SR_ERROR("TextureLoader::Load() : failed to save marshal to file \"" + cacheFilePath.ToStringRef() + "\"!");
            }
        }

        auto&& pTextureData = TextureData::Create(width, height, pImgData, [](uint8_t* pData) {
            TextureLoader::Free(pData);
        }, format);

        pTextureData->SetPath(path);

        SRAssert2(pTextureData, "TextureLoader::Load() : failed to create TextureData!");

        return pTextureData;
    }

    bool TextureLoader::Free(unsigned char *data) {
        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("TextureLoader::Free() : free source image data...");
        }

        if (data) {
            stbi_image_free(data);
        }
        else {
            SR_ERROR("TextureLoader::Free() : data is nullptr!");
            return false;
        }

        return true;
    }

    TextureData::Ptr TextureLoader::LoadFromMemory(const std::string& data, const Memory::TextureConfig &config) {
        int32_t width = 0, height = 0, numComponents = 0;

        const uint8_t requireComponents = GetChannelCount(config.m_format);

        uint8_t* pImgData = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.c_str()),
            static_cast<int32_t>(data.size()), &width, &height, &numComponents, requireComponents
        );

        if (!pImgData) {
            std::string reason;

            if (stbi_failure_reason()) {
                reason = stbi_failure_reason();
            }

            SR_ERROR("TextureLoader::LoadFromMemory() : can not load texture from memory!\n\tReason: " + reason);

            return nullptr;
        }

        const ImageLoadFormat format = numComponents == 4 ? ImageLoadFormat::RGBA : ImageLoadFormat::RGB;

        auto&& pTextureData = TextureData::Create(width, height, pImgData, [](uint8_t* pData) {
            TextureLoader::Free(pData);
        }, format);

        return pTextureData;
    }

    TextureData::Ptr TextureLoader::LoadFromCache(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        auto&& marshal = SR_HTYPES_NS::Marshal::Load(path);
        if (!marshal) {
            SR_ERROR("TextureLoader::LoadFromCache() : failed to load marshal from path \"" + path.ToString() + "\"!");
            return nullptr;
        }

        auto&& sourcePath = marshal.Read<std::string>();
        auto&& width = marshal.Read<uint32_t>();
        auto&& height = marshal.Read<uint32_t>();
        auto&& format = static_cast<ImageLoadFormat>(marshal.Read<uint8_t>());

        auto&& size = marshal.Read<uint64_t>();

        uint8_t* pData = nullptr;

        if (size > 0) {
            pData = (uint8_t*)malloc(size);
            marshal.Stream::Read(pData, size);
        }

        auto&& pTextureData = TextureData::Create(width, height, pData, [](uint8_t* pData) {
            free(pData);
        }, format);

        pTextureData->SetPath(sourcePath);

        return pTextureData;
    }
}

