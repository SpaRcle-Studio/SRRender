//
// Created by Nikita on 17.11.2020.
//

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Common/StringUtils.h>

#include <Graphics/Types/Texture.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Render/RenderContext.h>

#include <Enum/BoolExt.hpp>
#include <Enum/TextureCompression.hpp>
#include <Enum/TextureFilter.hpp>
#include <Enum/ImageFormat.hpp>

#include <Codegen/Texture.generated.hpp>

namespace SR_GTYPES_NS {
    Texture::Texture() = default;

    Texture::~Texture() {
        FreeTextureData();
    }

    Texture::Ptr Texture::LoadRaw(const uint8_t* pData, uint64_t bytes, uint64_t h, uint64_t w, const Memory::TextureConfig& config) {
        auto&& pTexture = Texture::MakeShared<Texture>();

        auto&& pCopyData = new uint8_t[bytes];
        memcpy(pCopyData, pData, bytes);

        pTexture->m_textureData = TextureData::Create(w, h, pCopyData, [](const uint8_t* pData) {
            delete[] pData;
        });

        pTexture->m_isFromMemory = true;
        pTexture->m_config = config;
        pTexture->SetId("RawTexture");

        return pTexture;
    }

    Texture::Ptr Texture::Load(const SR_UTILS_NS::Path& rawPath, const std::optional<Memory::TextureConfig>& config) {
        SR_TRACY_ZONE;

        if (rawPath.IsEmpty()) {
            SRHalt("Texture::Load() : path is empty!");
            return nullptr;
        }

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();

        auto&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(resourceManager.GetResPath());
        if (!resourceManager.GetResPath().Concat(path).Exists(SR_UTILS_NS::Path::Type::File)) {
            SR_ERROR("Texture::Load() : texture \"{}\" does not exist!", path.ToStringRef());
            return nullptr;
        }

        Texture::Ptr pTexture = nullptr;

        resourceManager.Execute([&]() {
            if ((pTexture = SR_UTILS_NS::ResourceManager::Instance().Find<Texture>(path))) {
                if (config && pTexture->m_config != config.value()) {
                    const std::string debugInfo =
                        "\n\tPath: {}"
                        "\n\tOld alpha: {}, New alpha: {}"
                        "\n\tOld mip levels: {}, New mip levels: {}"
                        "\n\tOld format: {}, New format: {}"
                        "\n\tOld filter: {}, New filter: {}"
                        "\n\tOld compression: {}, New compression: {}"
                        "\n\tOld cpu usage: {}, New cpu usage: {}"
                        ""_format(
                            path,
                            pTexture->m_config.m_alpha, config.value().m_alpha,
                            pTexture->m_config.m_mipLevels, config.value().m_mipLevels,
                            pTexture->m_config.m_format, config.value().m_format,
                            pTexture->m_config.m_filter, config.value().m_filter,
                            pTexture->m_config.m_compression, config.value().m_compression,
                            pTexture->m_config.m_cpuUsage, config.value().m_cpuUsage
                    );
                    SR_WARN("Texture::Load() : copy values do not match load values!" + debugInfo);
                }

                return;
            }

            pTexture = Texture::MakeShared<Texture>();

            if (config) {
                pTexture->SetConfig(config.value());
            }
            else {
                pTexture->SetConfig(Memory::TextureConfig());
            }

            pTexture->SetId(path.ToStringRef(), false /** auto register */);

            if (!pTexture->Load()) {
                SR_ERROR("Texture::Load() : failed to load texture! \n\tPath: " + path.ToString());
                pTexture->DeleteResource();
                pTexture = nullptr;
                return;
            }

            /// отложенная ручная регистрация
            SR_UTILS_NS::ResourceManager::Instance().RegisterResource(pTexture.StaticCast<SR_UTILS_NS::IResource>());
        });

        return pTexture;
    }

    bool Texture::Unload() {
        bool hasErrors = !IResource::Unload();

        m_isDirty = true;

        FreeTextureData();

        if (auto&& pRenderContext = GetRenderContext()) {
            pRenderContext->SetDirty();
        }

        return !hasErrors;
    }

    bool Texture::Load() {
        SR_TRACY_ZONE;

        m_isDirty = true;

        bool hasErrors = !IResource::Load();

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(GetResourceId());
        if (!path.IsAbs()) {
            path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
        }

        m_textureData = TextureLoader::Load(path);
        if (!m_textureData) {
            SR_ERROR("Texture::Load() : failed to load texture!");
            hasErrors |= true;
        }

        if (auto&& pRenderContext = GetRenderContext()) {
            pRenderContext->SetDirty();
        }

        return !hasErrors;
    }

    bool Texture::Calculate() {
        SR_TRACY_ZONE;

        if (!m_isDirty) {
            SRHalt("Texture::Calculate() : the texture \"{}\" is not dirty!", GetResourceId());
            return true;
        }

        if (!m_textureData) {
            SR_ERROR("Texture::Calculate() : data is invalid!");
            return false;
        }

        RegisterGraphicsResource();

        if (IsDestroyed()) {
            SR_ERROR("Texture::Calculate() : the texture is destroyed!");
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("Texture::Calculate() : calculating \"" + std::string(GetResourceId()) + "\" texture...");
        }

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!GetPipeline()->FreeTexture(&m_id));
        }

        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

        SRTextureCreateInfo createInfo;
        createInfo.pData = m_textureData->GetData();
        createInfo.width = m_textureData->GetWidth();
        createInfo.height = m_textureData->GetHeight();
        createInfo.compression = m_config.m_compression;
        createInfo.cpuUsage = m_config.m_cpuUsage;
        createInfo.alpha = m_config.m_alpha == SR_UTILS_NS::BoolExt::None;
        createInfo.format = m_config.m_format;
        createInfo.mipLevels = m_config.m_mipLevels;
        createInfo.filter = m_config.m_filter;

        m_id = GetPipeline()->AllocateTexture(createInfo);

        EVK_POP_LOG_LEVEL();

        if (m_id == SR_ID_INVALID) {
            SR_ERROR("Texture::Calculate() : failed to calculate the texture!");
            return false;
        }
        else {
            if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
                SR_LOG("Texture::Calculate() : texture \"" + std::string(GetResourceId()) + "\" has " + std::to_string(m_id) + " id.");
            }
        }

        m_isDirty = false;

        return true;
    }

    void Texture::FreeVMemory() {
        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Low) {
            SR_LOG("Texture::FreeVMemory() : free \"" + std::string(GetResourceId()) + "\" texture's video memory...");
        }

        if (!GetPipeline()->FreeTexture(&m_id)) {
            SR_ERROR("Texture::FreeVMemory() : failed to free texture!");
        }

        IGraphicsResource::FreeVMemory();
    }

    void Texture::SetConfig(const Memory::TextureConfig &config) {
        auto alpha = m_config.m_alpha;
        m_config = config;

        // TODO: to refactoring
        if (alpha != SR_UTILS_NS::BoolExt::None)
            m_config.m_alpha = alpha;
    }

    int32_t Texture::GetId() noexcept {
        if (m_hasErrors) {
            return SR_ID_INVALID;
        }

        if (IsDestroyed()) {
            SRHalt("Texture::GetId() : the texture \"" + std::string(GetResourceId()) + "\" is destroyed!");
            return SR_ID_INVALID;
        }

        if (m_isDirty && !Calculate()) {
            SR_ERROR("Texture::GetId() : failed to calculate the texture!");
            m_hasErrors = true;
            return SR_ID_INVALID;
        }

        SRAssert2(m_id != SR_ID_INVALID, "Texture::GetId() : the texture \"{}\" has invalid id!", GetResourceId());

        return m_id;
    }

    Texture::Ptr Texture::LoadFromMemory(const std::string& data, const Memory::TextureConfig &config) {
        SR_TRACY_ZONE;

        auto&& pTexture = Texture::MakeShared<Texture>();

        pTexture->m_textureData = TextureLoader::LoadFromMemory(data, config);
        if (!pTexture->m_textureData) {
            SR_ERROR("Texture::LoadFromMemory() : failed to load texture from memory!");
            pTexture->DeleteResource();
            pTexture = nullptr;
            return nullptr;
        }

        pTexture->m_isFromMemory = true;

        pTexture->SetConfig(config);
        pTexture->SetId("TextureFromMemory");

        return pTexture;
    }

    void* Texture::GetDescriptor() {
        auto&& textureId = GetId();

        if (textureId == SR_ID_INVALID) {
            return nullptr;
        }

        return GetPipeline()->GetOverlayTextureDescriptorSet(textureId, OverlayType::ImGui);
    }

    SR_UTILS_NS::Path SR_GTYPES_NS::Texture::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    void Texture::FreeTextureData() {
        m_textureData.Reset();
    }

    uint32_t Texture::GetWidth() const noexcept {
        return m_textureData ? m_textureData->GetWidth() : 0;
    }

    uint32_t Texture::GetHeight() const noexcept {
        return m_textureData ? m_textureData->GetHeight() : 0;
    }

    uint32_t Texture::GetChannels() const noexcept {
        return m_textureData ? m_textureData->GetChannels() : 0;
    }
}