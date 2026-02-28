//
// Created by Nikita on 17.11.2020.
//

#include <Graphics/Types/Texture.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Serialization/SRASerialization.h>
#include <Utils/Types/WeakPtr.h>
#include <Utils/TaskManager/TaskManager.h>

#ifdef SR_USE_VULKAN
    #include <EvoVulkan/Tools/VulkanDebug.h>
#endif

#include <Enum/BoolExt.hpp>
#include <Enum/TextureCompression.hpp>
#include <Enum/TextureFilter.hpp>
#include <Enum/ImageFormat.hpp>
#include <Enum/AddressMode.hpp>
#include <Enum/ImageType.hpp>

#include <Codegen/Texture.generated.hpp>

namespace SR_GTYPES_NS {
    Texture::Texture() = default;

    Texture::~Texture() {
        FreeTextureData();
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

        auto&& fullPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(GetResourcePath());

        ImageMetaInfo metaInfo = ImageMetaInfo();
        if (auto&& metaPath = fullPath.ConcatExt(".meta"); metaPath.Exists(SR_UTILS_NS::Path::Type::File)) {
            SR_UTILS_NS::SRADeserializer deserializer;
            if (deserializer.LoadFromFile(metaPath)) {
                if (!metaInfo.Load(deserializer)) {
                    SR_WARN("Texture::Load() : failed to load meta info from file: {}", metaPath);
                    metaInfo = ImageMetaInfo();
                }
            }
            else {
                SR_WARN("Texture::Load() : failed to load meta info from file: {}", metaPath);
            }
        }

        SetImageMetaInfoInternal(metaInfo);

        if (metaInfo.loadMode == SR_UTILS_NS::ResourceLoadMode::Async) {
            RegisterGraphicsResource();
            m_asyncLoading = true;
            m_syncLoadTaskId = SR_UTILS_NS::TaskManager::Instance().ExecuteAsync([pWeak = GetWeakThis<Texture>(), fullPath]() {
                SR_TRACY_ZONE_N("Texture::LoadAsync");
                SR_TRACY_ZONE_TEXT(fullPath);
                if (auto&& pStrong = pWeak.Lock()) {
                    pStrong->OnAsyncLoaded(TextureLoader::Load(fullPath));
                }
            });
        }
        else {
            m_asyncLoading = false;
            m_textureData = TextureLoader::Load(fullPath);
            if (!m_textureData) {
                SR_ERROR("Texture::Load() : failed to load texture!");
                hasErrors |= true;
            }
        }

        if (auto&& pRenderContext = GetRenderContext()) {
            pRenderContext->SetDirty();
        }

        return !hasErrors;
    }

    bool Texture::Calculate() {
        SR_TRACY_ZONE;

        if (!m_isDirty) {
            return true;
        }

        SR_TRACY_ZONE_TEXT(GetResourceId());

        if (m_asyncLoading) {
            SRHalt("Texture::Calculate() : the texture is still loading asynchronously!");
            return false;
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

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Low) {
            SR_LOG("Texture::Calculate() : calculating \"" + std::string(GetResourceId()) + "\" texture...");
        }

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!GetPipeline()->FreeTexture(&m_id));
        }

    #ifdef SR_USE_VULKAN
        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);
    #endif

        SRTextureCreateInfo createInfo;
        createInfo.pData = m_textureData->GetData();
        createInfo.width = m_textureData->GetWidth();
        createInfo.height = m_textureData->GetHeight();
        createInfo.compression = m_activeImageMetaInfo.compression;
        createInfo.cpuUsage = m_activeImageMetaInfo.cpuUsage;
        createInfo.alpha = m_activeImageMetaInfo.alpha == SR_UTILS_NS::BoolExt::None;
        createInfo.format = m_activeImageMetaInfo.format;
        createInfo.mipLevels = m_activeImageMetaInfo.mipLevels;
        createInfo.filter = m_activeImageMetaInfo.filter;
        createInfo.addressMode = m_activeImageMetaInfo.addressMode;

        if (m_activeImageMetaInfo.format == ImageFormat::Auto) {
            //const bool hasAlpha = (m_activeImageMetaInfo.alpha == SR_UTILS_NS::BoolExt::None || m_activeImageMetaInfo.alpha == SR_UTILS_NS::BoolExt::True) && m_textureData->GetChannels() == 4;
            constexpr bool hasAlpha = true;
            const bool srbgEnabled = GetRenderContext()->IsSrgbEnabled();
            switch (m_activeImageMetaInfo.imageType) {
                case ImageType::Albedo:
                    if (srbgEnabled) {
                        createInfo.format = hasAlpha ? ImageFormat::RGBA8_SRGB : ImageFormat::RGB8_SRGB;
                    }
                    else {
                        createInfo.format = hasAlpha ? ImageFormat::RGBA8_UNORM : ImageFormat::RGB8_UNORM;
                    }
                    break;
                case ImageType::Normal: createInfo.format = hasAlpha ? ImageFormat::RGBA8_UNORM : ImageFormat::RGB8_UNORM; break;
                case ImageType::Roughness: createInfo.format = hasAlpha ? ImageFormat::RGBA8_UNORM : ImageFormat::RGB8_UNORM; break;
                case ImageType::Metallic: createInfo.format = ImageFormat::R8_UNORM; break;
                case ImageType::AmbientOcclusion: createInfo.format = ImageFormat::R8_UNORM; break;
                case ImageType::Emissive: createInfo.format = hasAlpha ? ImageFormat::RGBA8_UNORM : ImageFormat::RGB8_UNORM; break;
                case ImageType::Height: createInfo.format = ImageFormat::R16_UNORM; break;
                default:
                    SRHalt("Texture::Calculate() : unsupported image type for auto format! Image type: {}", m_activeImageMetaInfo.imageType);
                    break;
            }
        }

        if (!IsTextureSupportsFormat(createInfo.format) && createInfo.format != ImageFormat::Auto && createInfo.format != ImageFormat::Unknown) {
            SR_WARN("Texture::Calculate() : the texture format {} is not supported! Falling back to Auto format.", createInfo.format);
            createInfo.format = ImageFormat::Auto;
        }

        if (createInfo.format == ImageFormat::Auto || createInfo.format == ImageFormat::Unknown) {
            if (m_textureData->GetChannels() == 4) {
                createInfo.format = ImageFormat::RGBA8_UNORM;
            }
            else {
                createInfo.format = ImageFormat::RGB8_UNORM;
            }
        }

        m_id = GetPipeline()->AllocateTexture(createInfo);

    #ifdef SR_USE_VULKAN
        EVK_POP_LOG_LEVEL();
    #endif

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

        if (m_id != SR_ID_INVALID && !GetPipeline()->FreeTexture(&m_id)) {
            SR_ERROR("Texture::FreeVMemory() : failed to free texture!");
        }

        m_isDirty = true;
        m_hasErrors = false;

        IGraphicsResource::FreeVMemory();
    }

    void Texture::SetImageMetaInfoInternal(const ImageMetaInfo& meta) {
        m_imageMetaInfo = m_activeImageMetaInfo = meta;
    }

    void Texture::OnAsyncLoaded(SR_HTYPES_NS::SharedPtr<TextureData>&& pTextureData) {
        SR_TRACY_ZONE;

        m_textureData = std::move(pTextureData);
        if (!m_textureData) {
            SR_ERROR("Texture::OnAsyncLoaded() : failed to load texture asynchronously!");
            m_hasErrors = true;
        }
    }

    void Texture::SetImageMetaInfo(const ImageMetaInfo& meta) {
        if (m_imageMetaInfo == meta) {
            return;
        }

        auto alpha = m_imageMetaInfo.alpha;
        m_imageMetaInfo = meta;

        // TODO: to refactoring
        if (alpha != SR_UTILS_NS::BoolExt::None) {
            m_imageMetaInfo.alpha = alpha;
        }
    }

    int32_t Texture::GetId() noexcept {
        SR_TRACY_ZONE;

        if (m_hasErrors) {
            return SR_ID_INVALID;
        }

        if (IsDestroyed()) {
            SRHalt("Texture::GetId() : the texture \"" + std::string(GetResourceId()) + "\" is destroyed!");
            return SR_ID_INVALID;
        }

        if (!Calculate()) {
            SR_ERROR("Texture::GetId() : failed to calculate the texture!");
            m_hasErrors = true;
            return SR_ID_INVALID;
        }

        SRAssert2(m_id != SR_ID_INVALID, "Texture::GetId() : the texture \"{}\" has invalid id!", GetResourceId());

        return m_id;
    }

    Texture::Ptr Texture::LoadFromMemory(const std::string& data, const ImageMetaInfo& meta) {
        SR_TRACY_ZONE;

        Texture::Ptr pTexture = new Texture();

        pTexture->m_textureData = TextureLoader::LoadFromMemory(data, meta);
        if (!pTexture->m_textureData) {
            SR_ERROR("Texture::LoadFromMemory() : failed to load texture from memory!");
            pTexture->DeleteResource();
            return nullptr;
        }

        pTexture->m_isFromMemory = true;

        pTexture->SetImageMetaInfo(meta);
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
        SR_TRACY_ZONE;
        if (m_asyncLoading) {
            SR_INFO("Texture::FreeTextureData() : the texture is still loading asynchronously! Waiting for the loading to finish...");
            while (SR_UTILS_NS::TaskManager::Instance().IsActive(m_syncLoadTaskId)) {
                SR_PLATFORM_NS::Sleep(5);
            }
        }
        m_textureData.Reset();
    }

    uint32_t Texture::GetWidth() const noexcept {
        if (m_asyncLoading) {
            SRHalt("Texture::GetWidth() : the texture is still loading asynchronously!");
            return 0;
        }
        return m_textureData ? m_textureData->GetWidth() : 0;
    }

    uint32_t Texture::GetHeight() const noexcept {
        if (m_asyncLoading) {
            //const_cast<Texture&>(*this).RegisterGraphicsResource();
            //GetRenderContext()->GetDefaultTexture()->
            SRHalt("Texture::GetHeight() : the texture is still loading asynchronously!");
            return 0;
        }
        return m_textureData ? m_textureData->GetHeight() : 0;
    }

    uint32_t Texture::GetChannels() const noexcept {
        if (m_asyncLoading) {
            SRHalt("Texture::GetChannels() : the texture is still loading asynchronously!");
            return 0;
        }
        return m_textureData ? m_textureData->GetChannels() : 0;
    }

    void Texture::PrepareFrame() {
        SR_TRACY_ZONE;

        if (m_asyncLoading) {
            if (SR_UTILS_NS::TaskManager::Instance().IsActive(m_syncLoadTaskId)) {
                return;
            }
            m_asyncLoading = false;
        }
        else if (m_imageMetaInfo == m_activeImageMetaInfo) {
            return;
        }

        if (auto&& pRenderContext = GetRenderContext()) {
            pRenderContext->SetDirty();
        }

        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->SetDirty(true);
        }

        SetImageMetaInfoInternal(m_imageMetaInfo);

        Broadcast(SR_UTILS_NS::IResource::RELOAD_BEGIN_EVENT);
        Broadcast(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT);

        m_isDirty = true;
        m_hasErrors = false;
    }

    float_t Texture::GetPPU() const noexcept {
        return m_activeImageMetaInfo.GetPixelsPerUnit();
    }

    SR_MATH_NS::FRect Texture::GetBorder() const noexcept {
        return m_activeImageMetaInfo.GetBorder();
    }

    bool Texture::CanBeUsed() const {
        if (m_asyncLoading) {
            return false;
        }

        return true;
    }
}
