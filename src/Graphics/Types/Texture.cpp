//
// Created by Nikita on 17.11.2020.
//

#include <Graphics/Types/Texture.h>
#include <Graphics/Types/TextureImpl.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/RenderTarget.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Loaders/TextureContainer.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Serialization/SRASerialization.h>
#include <Utils/Types/WeakPtr.h>
#include <Utils/TaskManager/TaskManager.h>
#include <Utils/Common/Features.h>

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

        m_impl = TextureImpl::TryCreate(*this);
        if (m_impl) {
            m_isFromMemory = !m_impl->IsHaveFile();
            RegisterGraphicsResource();
            GetRenderContext()->SetDirty();
            return !hasErrors;
        }

        auto&& fullPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(GetResourcePath());
        ImageMetaInfo metaInfo = ImageMetaInfo();
        if (auto&& metaPath = fullPath.ConcatExt(".meta"); metaPath.IsFile()) {
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

        RegisterGraphicsResource();


        m_format = metaInfo.format;

        if (m_format == ImageFormat::Auto) {
            const bool srbgEnabled = GetRenderContext()->IsSrgbEnabled();
            switch (m_activeImageMetaInfo.imageType) {
                case ImageType::Albedo:
                case ImageType::Emissive:
                    if (srbgEnabled) {
                        m_format = ImageFormat::RGBA8_SRGB;
                    }
                    else {
                        m_format = ImageFormat::RGBA8_UNORM;
                    }
                    break;
                case ImageType::Direction:
                case ImageType::UI:
                case ImageType::ORM:
                case ImageType::MSDF:
                case ImageType::PixelArt:
                    m_format = ImageFormat::RGBA8_UNORM;
                    break;
                case ImageType::Normal:
                    m_format = ImageFormat::RG8_UNORM;
                    break;
                case ImageType::Roughness:
                case ImageType::Metallic:
                case ImageType::AmbientOcclusion:
                case ImageType::SSS:
                case ImageType::Mask:
                    m_format = ImageFormat::R8_UNORM;
                    break;
                case ImageType::HDR:
                    m_format = ImageFormat::RGBA16_SFLOAT;
                    break;
                case ImageType::Height:
                    m_format = ImageFormat::R16_UNORM;
                    break;
                default:
                    SRHalt("Texture::Load() : unsupported image type for auto format! Image type: {}", m_activeImageMetaInfo.imageType);
                    break;
            }
        }

        if (!IsTextureSupportsFormat(m_format) && m_format != ImageFormat::Auto && m_format != ImageFormat::Unknown) {
            SR_WARN("Texture::Load() : the texture format {} is not supported! Falling back to Auto format.", m_format);
            m_format = ImageFormat::Auto;
        }

        if (m_format == ImageFormat::Auto || m_format == ImageFormat::Unknown) {
            m_format = ImageFormat::RGBA8_UNORM;
        }

        const bool asyncLoadSupport = SR_UTILS_NS::Features::Instance().Enabled("TextureAsyncLoad", true);

        TextureLoadInfo loadInfo;
        loadInfo.format = m_format;
        loadInfo.preparedVariants = metaInfo.preparedVariants;
        loadInfo.preparedHashes = metaInfo.preparedHashes;
        loadInfo.requireAlignedBlocks = GetPipeline()->RequiresBlockAlignedTextures();
        if (!loadInfo.preparedVariants.empty()) {
            bool hasLosslessFallback = false;
            for (auto&& variant : loadInfo.preparedVariants) {
                const auto extension = variant.GetExtensionView();
                hasLosslessFallback |= variant == GetResourcePath() || extension == "rgba8" || extension == "rgba16f";
            }
            // A compact export requires its native formats. A runtime compression toggle
            // cannot select a lossless copy that was deliberately omitted from the package.
            const bool compressedAllowed = !hasLosslessFallback || GetRenderContext()->IsTextureCompressionEnabled();
            for (uint32_t compression = 0; compression < static_cast<uint32_t>(TextureCompression::TextureCompressionMAX); ++compression) {
                if (!compressedAllowed && compression != 0) {
                    continue;
                }
                for (const ImageFormat format : { ImageFormat::RGBA8_UNORM, ImageFormat::RGBA8_SRGB, ImageFormat::RGBA16_SFLOAT }) {
                    const uint32_t nativeFormat = TextureContainer::GetNativeFormat(static_cast<TextureCompression>(compression), format);
                    if (nativeFormat && GetPipeline()->SupportsTextureFormat(nativeFormat)) {
                        loadInfo.supportedFormats.emplace_back(nativeFormat);
                    }
                }
            }
        }
        loadInfo.caching = SR_UTILS_NS::Features::Instance().Enabled("TextureCaching", true) && GetRenderContext()->IsTextureCachingEnabled();
        loadInfo.mips = metaInfo.mipLevels;
        loadInfo.channels = loadInfo.preparedVariants.empty() ? TextureLoader::GetAlignedChannels(m_format) : 4;
        loadInfo.disableAlphaChannel = metaInfo.disableAlphaChannel;

        if (metaInfo.loadMode == SR_UTILS_NS::ResourceLoadMode::Async && asyncLoadSupport) {
            m_syncLoadTaskId = SR_UTILS_NS::TaskManager::Instance().ExecuteAsync([loadInfo, pWeak = GetWeakThis<Texture>(), path = GetResourcePath()](auto&&) {
                SR_TRACY_ZONE_N("Texture::LoadAsync");
                SR_TRACY_ZONE_TEXT(path);
                if (auto&& pStrong = pWeak.Lock()) {
                    pStrong->OnAsyncLoaded(TextureLoader::Load(path, loadInfo));
                }
            }, SR_UTILS_NS::TaskPriority::Normal);
            SR_UTILS_NS::TaskManager::Instance().SetDescription(*m_syncLoadTaskId, GetResourceId());
        }
        else {
            m_textureData = TextureLoader::Load(GetResourcePath(), loadInfo);
            if (!m_textureData) {
                SR_ERROR("Texture::Load() : failed to load texture!");
                hasErrors |= true;
            }
        }

        GetRenderContext()->SetDirty();

        return !hasErrors;
    }

    bool Texture::Calculate() {
        SR_TRACY_ZONE;

        if (!m_isDirty) {
            return true;
        }

        SR_TRACY_ZONE_TEXT(GetResourceId());

        if (m_syncLoadTaskId) {
            SR_LOG("Texture::Calculate() : waiting for async load task to finish for \"{}\" texture...", GetResourceId());
            SR_UTILS_NS::TaskManager::Instance().Wait(*m_syncLoadTaskId);
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
            SR_LOG("Texture::Calculate() : calculating \"{}\" texture...", GetResourceId());
        }

        if (m_impl && m_impl->IsReference()) {
            m_isDirty = false;
            return true;
        }

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!GetPipeline()->FreeTexture(&m_id));
        }

        SRTextureCreateInfo createInfo;
        createInfo.imageSize = m_textureData->GetNumberOfBytes();
        createInfo.pData = m_textureData->GetData();
        createInfo.width = m_textureData->GetWidth();
        createInfo.height = m_textureData->GetHeight();
        createInfo.compression = m_textureData->GetInfo().compression;
        createInfo.preparedMipChain = m_textureData->GetInfo().preparedMipChain;
        createInfo.format = createInfo.preparedMipChain ? m_textureData->GetInfo().format : m_format;
        createInfo.mipLevels = m_textureData->GetInfo().mips;
        createInfo.filter = m_activeImageMetaInfo.filter;
        createInfo.addressMode = m_activeImageMetaInfo.addressMode;
        createInfo.cpuUsage = m_activeImageMetaInfo.cpuUsage;
        createInfo.async = m_activeImageMetaInfo.loadMode == SR_UTILS_NS::ResourceLoadMode::Async;
        createInfo.pTexture = this;

    #ifdef SR_USE_VULKAN
        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);
    #endif

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
                SR_LOG("Texture::Calculate() : texture {} has id {}", GetResourceId(), m_id);
            }
        }

        m_isDirty = false;

        return true;
    }

    void Texture::FreeVMemory() {
        SR_TRACY_ZONE;

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Low) {
            SR_LOG("Texture::FreeVMemory() : freeing video memory for {}", GetResourceId());
        }

        if (m_syncLoadTaskId) {
            SR_LOG("Texture::FreeVMemory() : the texture is still loading asynchronously! Waiting for the loading to finish...\n\tPath: {}", GetResourcePath());
            SR_UTILS_NS::TaskManager::Instance().Wait(*m_syncLoadTaskId);
            m_syncLoadTaskId.Reset();
        }

        const bool isReference = m_impl && m_impl->IsReference();
        if (!isReference && m_id != SR_ID_INVALID && !GetPipeline()->FreeTexture(&m_id)) {
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

        m_imageMetaInfo = meta;
    }

    int32_t Texture::GetId() noexcept {
        SR_TRACY_ZONE;

        if (m_hasErrors) {
            return SR_ID_INVALID;
        }

        if (IsDestroyed()) {
            SRHalt("Texture::GetId() : the texture \"{}\" is destroyed!", GetResourceId());
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

    Texture::Ptr Texture::LoadFromMemory(const SR_UTILS_NS::String& data, const ImageMetaInfo& meta) {
        SR_TRACY_ZONE;

        Texture::Ptr pTexture = new Texture();

        pTexture->m_textureData = TextureLoader::LoadFromMemory(data, meta);
        if (!pTexture->m_textureData) {
            SR_ERROR("Texture::LoadFromMemory() : failed to load texture from memory!");
            pTexture->DeleteResource();
            return nullptr;
        }

        pTexture->m_isFromMemory = true;
        pTexture->m_format = meta.format;

        pTexture->SetImageMetaInfoInternal(meta);
        static std::atomic<uint64_t> sequence = 0;
        pTexture->SetId("TextureFromMemory_{}"_format(sequence.fetch_add(1)));
        pTexture->IResource::Load();

        return pTexture;
    }

    void* Texture::GetDescriptor() {
        if (!CanBeUsed()) {
            return nullptr;
        }

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

        m_impl.Reset();

        if (m_syncLoadTaskId) {
            SR_INFO("Texture::FreeTextureData() : the texture is still loading asynchronously! Waiting for the loading to finish...\n\tPath: {}", GetResourcePath());
            SR_UTILS_NS::TaskManager::Instance().Wait(*m_syncLoadTaskId);
        }
        m_textureData.Reset();
    }

    uint32_t Texture::GetWidth() const noexcept {
        if (m_syncLoadTaskId) {
            SRHalt("Texture::GetWidth() : the texture is still loading asynchronously!");
            return 0;
        }
        return m_textureData ? m_textureData->GetWidth() : 0;
    }

    uint32_t Texture::GetHeight() const noexcept {
        if (m_syncLoadTaskId) {
            SRHalt("Texture::GetHeight() : the texture is still loading asynchronously!");
            return 0;
        }
        return m_textureData ? m_textureData->GetHeight() : 0;
    }

    uint32_t Texture::GetChannels() const noexcept {
        if (m_syncLoadTaskId) {
            SRHalt("Texture::GetChannels() : the texture is still loading asynchronously!");
            return 0;
        }
        return m_textureData ? m_textureData->GetChannels() : 0;
    }

    void Texture::OnAsyncCalculated() {
        auto&& pRenderContext = GetRenderContext();
        if (!pRenderContext) {
            return;
        }
        pRenderContext->SetDirty();
        Broadcast(SR_UTILS_NS::IResource::RELOAD_BEGIN_EVENT);
        Broadcast(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT);
    }

    void Texture::PrepareFrame() {
        SR_TRACY_ZONE;

        auto&& pRenderContext = GetRenderContext();
        if (!pRenderContext) {
            return;
        }

        if (m_impl) {
            m_impl->PrepareFrame();
        }

        if (m_syncLoadTaskId) {
            if (SR_UTILS_NS::TaskManager::Instance().IsActive(*m_syncLoadTaskId)) {
                return;
            }
            m_syncLoadTaskId.Reset();
        }
        else if (m_imageMetaInfo == m_activeImageMetaInfo) {
            return;
        }

        pRenderContext->SetDirty();
        SetImageMetaInfoInternal(m_imageMetaInfo);

        Broadcast(SR_UTILS_NS::IResource::RELOAD_BEGIN_EVENT);
        Broadcast(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT);

        m_isDirty = true;
        if (m_textureData) {
            m_hasErrors = false;
        }
    }

    float_t Texture::GetPPU() const noexcept {
        return m_activeImageMetaInfo.GetPixelsPerUnit();
    }

    SR_MATH_NS::FRect Texture::GetBorder() const noexcept {
        return m_activeImageMetaInfo.GetBorder();
    }

    bool Texture::CanBeUsed() const {
        if (m_syncLoadTaskId) {
            return false;
        }

        if (m_impl && !m_impl->CanBeUsed()) {
            return false;
        }

        return true;
    }
}
