//
// Created by Nikita on 17.11.2020.
//

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Common/StringUtils.h>

#include <Graphics/Types/Texture.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Render/RenderContext.h>

namespace SR_GTYPES_NS {
    Texture::Texture()
        : IResource(SR_COMPILE_TIME_CRC32_TYPE_NAME(Texture))
    { }

    Texture::~Texture() {
        FreeTextureData();
    }

    Texture::Ptr Texture::LoadFont(Font* pFont) {
        auto&& pTexture = new Texture();

        pTexture->m_isFromMemory = true;

        pTexture->m_textureData = TextureData::Create(pFont->GetWidth(), pFont->GetHeight(), pFont->CopyData(), [](const uint8_t* pData) {
            delete[] pData;
        });

        pTexture->m_config.m_alpha = SR_UTILS_NS::BoolExt::True;
        pTexture->m_config.m_format = ImageFormat::RGBA8_UNORM;
        pTexture->m_config.m_filter = TextureFilter::NEAREST;
        pTexture->m_config.m_compression = TextureCompression::None;
        pTexture->m_config.m_mipLevels = 1;
        pTexture->m_config.m_cpuUsage = false;

        pTexture->SetId("FontTexture");

        return pTexture;
    }

    Texture::Ptr Texture::LoadRaw(const uint8_t* pData, uint64_t bytes, uint64_t h, uint64_t w, const Memory::TextureConfig& config) {
        auto&& pTexture = new Texture();

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
            SR_ERROR("Texture::Load() : path is empty!");
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
                    SR_WARN("Texture::Load() : copy values do not match load values.");
                }

                return;
            }

            pTexture = new Texture();

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
            SR_UTILS_NS::ResourceManager::Instance().RegisterResource(pTexture);
        });

        return pTexture;
    }

    bool Texture::Unload() {
        bool hasErrors = !IResource::Unload();

        FreeTextureData();

        m_isCalculated = false;

        m_context.Do([](RenderContext* ptr) {
            ptr->SetDirty();
        });

        return !hasErrors;
    }

    bool Texture::Load() {
        SR_TRACY_ZONE;

        bool hasErrors = !IResource::Load();

        if (!IsCalculated()) {
            SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(GetResourceId());
            if (!path.IsAbs()) {
                path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
            }

            m_textureData = TextureLoader::Load(path);
            if (!m_textureData) {
                SR_ERROR("Texture::Load() : failed to load texture!");
                hasErrors |= true;
            }
        }
        else {
            SRHalt("Texture already calculated!");
        }

        m_context.Do([](RenderContext* ptr) {
            ptr->SetDirty();
        });

        return !hasErrors;
    }

    bool Texture::Calculate() {
        SR_TRACY_ZONE;

        if (m_isCalculated) {
            SR_ERROR("Texture::Calculate() : texture is already calculated!");
            return false;
        }

        if (!m_textureData) {
            SR_ERROR("Texture::Calculate() : data is invalid!");
            return false;
        }

        if (!SRVerifyFalse2(!(m_context = SR_THIS_THREAD->GetContext()->GetValue<RenderContextPtr>()), "Is not render context!")) {
            m_hasErrors = true;
            return false;
        }

        m_context.Do([this](RenderContext* ptr) {
            ptr->Register(this);
        });

        if (IsDestroyed()) {
            SR_ERROR("Texture::Calculate() : the texture is destroyed!");
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("Texture::Calculate() : calculating \"" + std::string(GetResourceId()) + "\" texture...");
        }

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!m_pipeline->FreeTexture(&m_id));
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

        m_id = m_pipeline->AllocateTexture(createInfo);

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

        m_isCalculated = true;

        return true;
    }

    void Texture::FreeVideoMemory() {
        /// Просто игнорируем, текстура могла быть не использована
        if (!m_isCalculated) {
            return;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Low) {
            SR_LOG("Texture::FreeVideoMemory() : free \"" + std::string(GetResourceId()) + "\" texture's video memory...");
        }

        SRAssert(m_pipeline);

        if (m_pipeline && !m_pipeline->FreeTexture(&m_id)) {
            SR_ERROR("Texture::FreeVideoMemory() : failed to free texture!");
        }

        IGraphicsResource::FreeVideoMemory();
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

        if (!m_isCalculated && !Calculate()) {
            SR_ERROR("Texture::GetId() : failed to calculate the texture!");
            m_hasErrors = true;
            return SR_ID_INVALID;
        }

        return m_id;
    }

    Texture* Texture::LoadFromMemory(const std::string& data, const Memory::TextureConfig &config) {
        SR_TRACY_ZONE;

        auto&& pTexture = new Texture();

        pTexture->m_textureData = TextureLoader::LoadFromMemory(data, config);
        if (!pTexture->m_textureData) {
            SR_ERROR("Texture::LoadFromMemory() : failed to load texture from memory!");
            pTexture->DeleteResource();
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

        return m_pipeline->GetOverlayTextureDescriptorSet(textureId, OverlayType::ImGui);
    }

    SR_UTILS_NS::Path SR_GTYPES_NS::Texture::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    void Texture::FreeTextureData() {
        m_textureData.Reset();
    }

    SR_UTILS_NS::IResource::RemoveUPResult Texture::RemoveUsePoint() {
        SRAssert2(!(IsCalculated() && GetCountUses() == 1), "Possible multi threading error!");
        return IResource::RemoveUsePoint();
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