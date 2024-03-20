//
// Created by Nikita on 03.12.2020.
//

#define STB_IMAGE_IMPLEMENTATION

#include <Utils/Common/StringUtils.h>
#include <Utils/Debug.h>

#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Types/Texture.h>

#include <stbi/stb_image.c>

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
        int stbiFormat = 0;
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
            TextureLoader::Free(pData);
        };

        return pTextureData;
    }

    TextureData::Ptr TextureData::Create(uint32_t width, uint32_t height, ImageLoadFormat format, uint8_t* pData, DeleterFn deleter) {
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

    bool TextureLoader::Load(Types::Texture* texture, std::string path) {
        if (!SRVerifyFalse(!texture)) {
            return false;
        }

        if (!SRVerifyFalse2(texture->m_data, "Texture already loaded!")) {
            return false;
        }

        int width, height, numComponents;

        unsigned char* imgData = stbi_load(path.c_str(), &width, &height, &numComponents, STBI_rgb_alpha);

        if (!imgData) {
            std::string reason = stbi_failure_reason() ? stbi_failure_reason() : std::string();
            SR_ERROR("TextureLoader::Load() : can not load \"" + path + "\" file!\n\tReason: " + reason);
            return false;
        }

        texture->m_height     = height;
        texture->m_width      = width;
        texture->m_channels   = numComponents;
        texture->m_data       = imgData;
        texture->m_config.m_alpha = (numComponents == 4) ? SR_UTILS_NS::BoolExt::True : SR_UTILS_NS::BoolExt::False;

        return true;
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

    SR_GTYPES_NS::Texture* TextureLoader::GetDefaultTexture() noexcept {
        return nullptr;
    }

    bool TextureLoader::LoadFromMemory(Types::Texture* texture, const std::string& data, const Memory::TextureConfig &config) {
        if (!SRVerifyFalse(!texture)) {
            return false;
        }

        int width, height, numComponents;

        int requireComponents = 0;

        switch (config.m_format) {
            case ImageFormat::RGBA8_UNORM:
            case ImageFormat::BGRA8_UNORM:
            case ImageFormat::RGBA8_SRGB:
                requireComponents = 4;
                break;
            case ImageFormat::R8_UNORM:
            case ImageFormat::R8_UINT:
                requireComponents = 1;
                break;
            case ImageFormat::RG8_UNORM:
                requireComponents = 2;
                break;
            case ImageFormat::RGB8_UNORM:
                requireComponents = 3;
                break;
            case ImageFormat::RGBA16_UNORM:
            case ImageFormat::RGB16_UNORM:
            case ImageFormat::R16_UNORM:
            case ImageFormat::R32_SFLOAT:
            case ImageFormat::R64_SFLOAT:
            case ImageFormat::R16_UINT:
            case ImageFormat::R32_UINT:
            case ImageFormat::R64_UINT:
            case ImageFormat::Unknown:
            default:
                SR_ERROR("TextureLoader::LoadFromMemory() : unknown color format!\n\tImageFormat: " + SR_UTILS_NS::EnumReflector::ToStringAtom(config.m_format).ToStringRef());
                return false;
        }

        unsigned char* imgData = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc*>(data.c_str()),
                data.size(), &width, &height, &numComponents, requireComponents
        );

        if (!imgData) {
            std::string reason;

            if (stbi_failure_reason()) {
                reason = stbi_failure_reason();
            }

            SR_ERROR("TextureLoader::LoadFromMemory() : can not load texture from memory!\n\tReason: " + reason);

            return false;
        }

        texture->m_height = height;
        texture->m_width  = width;
        texture->m_data   = imgData;
        texture->m_config.m_alpha = (numComponents == 4) ? SR_UTILS_NS::BoolExt::True : SR_UTILS_NS::BoolExt::False;

        return true;
    }
}

