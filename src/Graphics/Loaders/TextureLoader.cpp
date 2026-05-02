//
// Created by Nikita on 03.12.2020.
//

#define STB_IMAGE_IMPLEMENTATION

#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Types/Texture.h>

#include <Utils/Common/StringUtils.h>
#include <Utils/Common/ToString.h>
#include <Utils/TaskManager/TaskManager.h>
#include <Utils/Debug.h>
#include <Utils/Common/Features.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Types/Marshal.h>
#include <Utils/FileSystem/FileSystem.h>
#include <Utils/FileSystem/MappedFile.h>
#include <Utils/Common/CLIManager.h>

#include <Enum/TextureCompression.hpp>
#include <Enum/ImageFormat.hpp>

#include <stbi/stb_image.c> /// NOLINT
#include <stbi/stbi_image_write.c> /// NOLINT

namespace SR_GRAPH_NS {
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
            SRHalt("TextureData::GetNumberOfBytes() : m_data is not null!");
            return 0;
        }

        if (m_info.compression != TextureCompression::None) {
            return GetCompressedImageSize(m_width, m_height, m_info);
        }

        if (!TextureLoader::IsAllowedChannelsCount(m_info.channels)) {
            SRHalt("TextureData::GetNumberOfBytes() : wrong channels count! Channels: {}, path: {}", m_info.channels, m_path);
            return 0;
        }

        return m_width * m_height * m_info.channels;
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

    TextureData::Ptr TextureLoader::Load(const SR_UTILS_NS::Path& path, TextureLoadInfo info) {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_TEXT(path);

        const bool cacheEnabled = SR_UTILS_NS::Features::Instance().Enabled("TextureCaching", true);
        const bool compressionEnabled = SR_UTILS_NS::Features::Instance().Enabled("TextureCompression", true);

        auto&& resPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath();
        SR_UTILS_NS::Path fullPath = resPath.Concat(path);
        SR_UTILS_NS::Path compressedTexturePath = resPath.Concat(SR_UTILS_NS::Path("Packed").Concat(path).ConcatExt(SR_UTILS_NS::EnumReflector::ToStringAtom(info.compression)));

        const bool isUnitTests = SR_UTILS_NS::CLIManager::Instance().IsFlagPresent(SR_UTILS_NS::CLIFlagsEnumWrappper::UnitTests);
        const bool canCompress = info.compression != TextureCompression::None && cacheEnabled && !isUnitTests && compressionEnabled;
        const bool compressedTextureExists = info.compression != TextureCompression::None && compressedTexturePath.Exists(SR_UTILS_NS::Path::Type::File);

        auto&& cache = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Textures");
        auto&& cacheHashPath = cache.Concat("Hashes").Concat(path).ConcatExt(".cache.hash");
        auto&& cacheFilePath = cache.Concat("Dump").Concat(path).ConcatExt(".cache");

        uint64_t fileHash = 0;

        if (cacheEnabled) {
            fileHash = fullPath.GetFileHash();
            fileHash = SR_UTILS_NS::HashCombine(fileHash, static_cast<uint64_t>(info.channels));
            fileHash = SR_UTILS_NS::HashCombine(fileHash, static_cast<uint64_t>(info.mips));

            if (cacheHashPath.Exists(SR_UTILS_NS::Path::Type::File) && SR_UTILS_NS::FileSystem::ReadHashFromFile(cacheHashPath) == fileHash) {
                if (compressedTextureExists) {
                    if (auto&& pTextureData = LoadFromCache(compressedTexturePath)) {
                        return pTextureData;
                    }
                }

                if (auto&& pTextureData = LoadFromCache(cacheFilePath)) {
                    if (canCompress) {
                        AsyncCompressTexture(pTextureData, info.compression);
                    }
                    return pTextureData;
                }
            }
        }
        else if (compressedTextureExists) {
            if (auto&& pTextureData = LoadFromCache(compressedTexturePath)) {
                return pTextureData;
            }
        }

        SR_UTILS_NS::String buffer;
        if (!SR_UTILS_NS::FileSystem::ReadFile(fullPath, buffer)) {
            SR_ERROR("TextureLoader::Load() : can not read \"{}\" file!", path);
            return nullptr;
        }

        if (!TextureLoader::IsAllowedChannelsCount(info.channels)) {
            SRHalt("TextureLoader::Load() : wrong channels count! Channels: {}, path: {}", info.channels, path);
            return nullptr;
        }

        int32_t width = 0, height = 0, channels = 0;

        stbi_set_unpremultiply_on_load(0);
        stbi_convert_iphone_png_to_rgb(0);
        uint8_t* pImgDataOriginal = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(buffer.data()), static_cast<int32_t>(buffer.size()), &width, &height, &channels, 4);

        if (!pImgDataOriginal) {
            std::string reason = stbi_failure_reason() ? stbi_failure_reason() : std::string();
            SR_ERROR("TextureLoader::Load() : can not load \"{}\" file!\n\tReason: {}", path, reason);
            return nullptr;
        }

        uint8_t* pImgData = pImgDataOriginal;

        if (info.channels != 4) {
            pImgData = (uint8_t*)SRMalloc(width * height * info.channels * sizeof(uint8_t));
        }

        uint8_t* src = pImgDataOriginal;
        uint8_t* dst = pImgData;
        const int pixelCount = width * height;

        switch (info.channels) {
            case 1:
                for (int i = 0; i < pixelCount; ++i) {
                    dst[i] = src[i * 4];
                }
                break;
            case 2:
                for (int i = 0; i < pixelCount; ++i) {
                    dst[i * 2 + 0] = src[i * 4 + 0];
                    dst[i * 2 + 1] = src[i * 4 + 1];
                }
                break;
            default:
                break;
        }

        if (info.channels != 4) {
            TextureLoader::Free(pImgDataOriginal);
        }

        const uint32_t autoMips = std::floor(std::log2(std::max(width, height))) + 1;
        if (info.mips > autoMips) {
            SR_WARN("TextureLoader::Load() : requested mip levels count is greater than possible! Requested: {}, possible: {}. Path: {}", info.mips, autoMips, path);
            info.mips = autoMips;
        }
        else {
            info.mips = info.mips > 0 ? info.mips : autoMips;
        }

        if (cacheEnabled) {
            SRAssert2(!path.empty(), "TextureLoader::Load() : path is empty!");
            SR_LOG("TextureLoader::Load() : save texture to cache...\n\tPath: {}", path);

            if (!SR_UTILS_NS::FileSystem::WriteHashToFile(cacheHashPath, fileHash)) {
                SR_ERROR("TextureLoader::Load() : failed to write hash to file \"{}\"!", cacheHashPath);
            }

            auto&& marshal = SR_HTYPES_NS::Marshal();

            marshal.Reserve(
                1024 + // overhead
                path.ToStringRef().size() + // path
                width * height * info.channels * sizeof(uint8_t) // data
            );

            marshal.Write<uint64_t>(TextureLoader::VERSION);
            marshal.Write<std::string>(path.ToStringRef());
            marshal.Write<uint32_t>(width);
            marshal.Write<uint32_t>(height);
            marshal.Write<uint32_t>(info.mips);
            marshal.Write<uint8_t>(info.channels);
            marshal.Write(static_cast<uint8_t>(TextureCompression::None));
            marshal.WriteBlock(pImgData, width * height * info.channels * sizeof(uint8_t));

            if (!marshal.Save(cacheFilePath)) {
                SR_ERROR("TextureLoader::Load() : failed to save marshal to file \"{}\"!", cacheFilePath);
            }
        }

        TextureLoadInfo infoCopy = info;
        if (!compressedTextureExists) {
            infoCopy.compression = TextureCompression::None;
        }

        auto&& pTextureData = TextureData::Create(width, height, pImgData, [channels = info.channels](uint8_t* pData) {
            if (channels != 4) {
                SRFree(pData);
            }
            else {
                TextureLoader::Free(pData);
            }
        }, infoCopy);

        if (!pTextureData) {
            SR_ERROR("TextureLoader::Load() : failed to create TextureData for path \"{}\"!", path);
            return nullptr;
        }

        pTextureData->SetPath(path);

        if (canCompress) {
            AsyncCompressTexture(pTextureData, info.compression);
        }

        return pTextureData;
    }

    bool TextureLoader::Free(unsigned char *data) {
        SR_TRACY_ZONE;

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

    TextureData::Ptr TextureLoader::LoadFromMemory(const std::string& data, const ImageMetaInfo& config) {
        SR_TRACY_ZONE;

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
            std::string reason;

            if (stbi_failure_reason()) {
                reason = stbi_failure_reason();
            }

            SR_ERROR("TextureLoader::LoadFromMemory() : can not load texture from memory!\n\tReason: " + reason);

            return nullptr;
        }

        TextureLoadInfo info;
        info.channels = requireChannels;

        auto&& pTextureData = TextureData::Create(width, height, pImgData, [](uint8_t* pData) {
            TextureLoader::Free(pData);
        }, info);

        return pTextureData;
    }

    TextureData::Ptr TextureLoader::LoadFromCache(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_TEXT(path);

        SR_UTILS_NS::MappedFile mappedFile = SR_UTILS_NS::MappedFile::Open(path);
        if (!mappedFile) {
            SR_ERROR("TextureLoader::LoadFromCache() : failed to load marshal from path \"{}\"!", path);
            return nullptr;
        }

        SR_HTYPES_NS::Marshal marshal(mappedFile);
        auto&& version = marshal.Read<uint64_t>();
        if (version != TextureLoader::VERSION) {
            return nullptr;
        }

        auto&& sourcePath = marshal.Read<std::string>();
        auto&& width = marshal.Read<uint32_t>();
        auto&& height = marshal.Read<uint32_t>();

        TextureLoadInfo info;
        info.mips = marshal.Read<uint32_t>();
        info.channels = marshal.Read<uint8_t>();
        info.compression = static_cast<TextureCompression>(marshal.Read<uint8_t>());

        auto&& size = marshal.Read<uint64_t>();
        if (size == 0) {
            SR_ERROR("TextureLoader::LoadFromCache() : size is zero!");
            return nullptr;
        }

        const uint64_t offset = marshal.GetPosition();

        uint8_t* pOriginData = (uint8_t*)mappedFile.GetData();
        uint8_t* pData = pOriginData + offset;

        SR_UTILS_NS::MappedFile* pMappedFile = new SR_UTILS_NS::MappedFile(std::move(mappedFile));
        auto&& pTextureData = TextureData::Create(width, height, pData, [pMappedFile](uint8_t*) {
            delete pMappedFile;
        }, info);

        pTextureData->SetPath(sourcePath);

        return pTextureData;
    }

    void TextureLoader::AsyncCompressTexture(const TextureData::Ptr& pData, TextureCompression compression) {
        SR_TRACY_ZONE;

        if (compression == TextureCompression::None) {
            SRHalt("TextureLoader::AsyncCompressTexture() : compression is None!");
            return;
        }

        SR_UTILS_NS::TaskManager::Instance().ExecuteAsync([pCopyData = pData, compression](auto&& state) {
            CompressTexture(pCopyData, compression, state);
        }, SR_UTILS_NS::TaskPriority::Discardable);
    }

    void TextureLoader::CompressTexture(const TextureData::Ptr& pData, TextureCompression compression, std::atomic<SR_UTILS_NS::TaskState>& state) {
        SR_TRACY_ZONE;

        SR_LOG("TextureLoader::CompressTexture() : compress texture...\n\tPath: {}", pData->GetPath());

        uint64_t compressedSize = 0;
        uint8* pCompressedData = nullptr;

        TextureLoadInfo info = pData->GetInfo();
        info.compression = compression;

    #ifdef SR_USE_CMP_CORE
        compressedSize = GetCompressedImageSize(pData->GetWidth(), pData->GetHeight(), info);
        pCompressedData = CompressImage(pData->GetWidth(), pData->GetHeight(), pData->GetData(), info, state);
    #else
        return;
    #endif

        if (state.load() == SR_UTILS_NS::TaskState::Stopped) {
            SR_LOG("TextureLoader::CompressTexture() : compression task was stopped! Path: {}", pData->GetPath());
            return;
        }

        if (!pCompressedData || compressedSize == 0) {
            SR_ERROR("TextureLoader::AsyncCompressTexture() : failed to compress texture!");
            return;
        }

        const SR_UTILS_NS::Path& resFolder = SR_UTILS_NS::ResourceManager::Instance().GetResPathRef();
        const SR_UTILS_NS::Path relativePath = pData->GetPath().RemoveSubPath(resFolder);

        auto&& compressedTexturePath = resFolder.Concat("Packed").Concat(relativePath).ConcatExt(SR_UTILS_NS::EnumReflector::ToStringAtom(info.compression));

        auto&& marshal = SR_HTYPES_NS::Marshal();

        marshal.Reserve(
            1024 + // overhead
            pData->GetPath().ToStringRef().size() + // path
            compressedSize // data
        );

        marshal.Write<uint64_t>(TextureLoader::VERSION);
        marshal.Write<std::string>(pData->GetPath().ToStringRef());
        marshal.Write<uint32_t>(pData->GetWidth());
        marshal.Write<uint32_t>(pData->GetHeight());
        marshal.Write<uint32_t>(info.mips);
        marshal.Write<uint8_t>(info.channels);
        marshal.Write(static_cast<uint8_t>(info.compression));
        marshal.WriteBlock(pCompressedData, compressedSize);

        SRFree(pCompressedData);

        if (!marshal.Save(compressedTexturePath)) {
            SR_ERROR("TextureLoader::AsyncCompressTexture() : failed to save marshal to file \"{}\"!", compressedTexturePath);
            return;
        }

        SR_LOG("TextureLoader::CompressTexture() : compressed texture saved to cache.\n\tPath: {}", compressedTexturePath);

        SR_UTILS_NS::StringAtom resourceId = relativePath.View();
        SR_UTILS_NS::ResourceManager::Instance().ReloadResource(resourceId, SR_GTYPES_NS::Texture::GetClassStaticName());
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
}

