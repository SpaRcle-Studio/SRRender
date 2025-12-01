//
// Created by Nikita on 17.11.2020.
//

#include <Graphics/Types/Texture.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Serialization/SRASerialization.h>

#include <EvoVulkan/Tools/VulkanDebug.h>

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

    Texture::Ptr Texture::LoadRaw(const uint8_t* pData, uint64_t bytes, uint64_t h, uint64_t w, const ImageMetaInfo& metaInfo) {
        auto&& pTexture = Texture::MakeShared<Texture>();

        auto&& pCopyData = new uint8_t[bytes];
        memcpy(pCopyData, pData, bytes);

        pTexture->m_textureData = TextureData::Create(w, h, pCopyData, [](const uint8_t* pData) {
            delete[] pData;
        });

        pTexture->m_isFromMemory = true;
        pTexture->m_activeImageMetaInfo = metaInfo;
        pTexture->SetId("RawTexture");

        return pTexture;
    }

    Texture::Ptr Texture::Load(const SR_UTILS_NS::Path& rawPath, std::optional<ImageMetaInfo> metaInfo) {
        SR_TRACY_ZONE;

        if (rawPath.IsEmpty()) {
            SRHalt("Texture::Load() : path is empty!");
            return nullptr;
        }

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();

        auto&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(resourceManager.GetResPath());
        if (!resourceManager.GetResPath().Concat(path).Exists(SR_UTILS_NS::Path::Type::File)) {
            SR_ERROR("Texture::Load() : texture \"{}\" does not exist!", path);
            return nullptr;
        }

        if (!metaInfo) {
            auto&& metaPath = resourceManager.GetResPath().Concat(path).ConcatExt(".meta");
            if (metaPath.Exists(SR_UTILS_NS::Path::Type::File)) {
                SR_UTILS_NS::SRADeserializer deserializer;
                if (deserializer.LoadFromFile(metaPath)) {
                    metaInfo = ImageMetaInfo();
                    if (!metaInfo->Load(deserializer)) {
                        SR_WARN("Texture::Load() : failed to load meta info from file: {}", metaPath);
                        metaInfo = std::nullopt;
                    }
                }
                else {
                    SR_WARN("Texture::Load() : failed to load meta info from file: {}", metaPath);
                }
            }
        }

        Texture::Ptr pTexture = nullptr;

        resourceManager.Execute([&]() {
            if ((pTexture = SR_UTILS_NS::ResourceManager::Instance().Find<Texture>(path))) {
                if (metaInfo && pTexture->m_activeImageMetaInfo != metaInfo.value()) {
                    const std::string debugInfo =
                        "\n\tPath: {}"
                        "\n\tOld alpha: {}, New alpha: {}"
                        "\n\tOld mip levels: {}, New mip levels: {}"
                        "\n\tOld format: {}, New format: {}"
                        "\n\tOld filter: {}, New filter: {}"
                        "\n\tOld compression: {}, New compression: {}"
                        "\n\tOld cpu usage: {}, New cpu usage: {}"
                        "\n\tOld pixels per unit: {}, New pixels per unit: {}"
                        "\n\tOld border: {}, New border: {}"
                        ""_format(
                            path,
                            pTexture->m_activeImageMetaInfo.alpha, metaInfo.value().alpha,
                            pTexture->m_activeImageMetaInfo.mipLevels, metaInfo.value().mipLevels,
                            pTexture->m_activeImageMetaInfo.format, metaInfo.value().format,
                            pTexture->m_activeImageMetaInfo.filter, metaInfo.value().filter,
                            pTexture->m_activeImageMetaInfo.compression, metaInfo.value().compression,
                            pTexture->m_activeImageMetaInfo.cpuUsage, metaInfo.value().cpuUsage,
                            pTexture->m_activeImageMetaInfo.m_pixelsPerUnit, metaInfo.value().m_pixelsPerUnit,
                            pTexture->m_activeImageMetaInfo.m_border, metaInfo.value().m_border
                    );
                    SR_WARN("Texture::Load() : copy values do not match load values!" + debugInfo);
                }

                return;
            }

            pTexture = Texture::MakeShared<Texture>();

            if (metaInfo) {
                pTexture->SetImageMetaInfoInternal(metaInfo.value());
            }
            else {
                pTexture->SetImageMetaInfoInternal(ImageMetaInfo());
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
        createInfo.compression = m_activeImageMetaInfo.compression;
        createInfo.cpuUsage = m_activeImageMetaInfo.cpuUsage;
        createInfo.alpha = m_activeImageMetaInfo.alpha == SR_UTILS_NS::BoolExt::None;
        createInfo.format = m_activeImageMetaInfo.format;
        createInfo.mipLevels = m_activeImageMetaInfo.mipLevels;
        createInfo.filter = m_activeImageMetaInfo.filter;

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

        m_isDirty = true;
        m_hasErrors = false;

        IGraphicsResource::FreeVMemory();
    }

    void Texture::SetImageMetaInfoInternal(const ImageMetaInfo& meta) {
        m_imageMetaInfo = m_activeImageMetaInfo = meta;
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
            pTexture = nullptr;
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

    void Texture::PrepareFrame() {
        SR_TRACY_ZONE;

        if (m_imageMetaInfo == m_activeImageMetaInfo) {
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
}
