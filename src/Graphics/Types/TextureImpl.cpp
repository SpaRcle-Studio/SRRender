//
// Created by Monika on 28.05.2026.
//

#include <Graphics/Types/TextureImpl.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/RenderTarget.h>
#include <Graphics/Loaders/TextureLoader.h>
#include <Graphics/Render/RenderContext.h>

#include <Utils/Common/LexicalCast.h>
#include <Utils/Common/StringUtils.h>

namespace SR_GTYPES_NS {
    int32_t& TextureImpl::GetTextureIdRef() const {
        return m_texture.m_id;
    }

    SR_HTYPES_NS::SharedPtr<TextureData>& TextureImpl::GetTextureDataRef() const {
        return m_texture.m_textureData;
    }

    ImageFormat& TextureImpl::GetImageFormatRef() const {
        return m_texture.m_format;
    }

    ImageMetaInfo& TextureImpl::GetImageMetaInfoRef() const {
        return m_texture.m_imageMetaInfo;
    }

    ImageMetaInfo& TextureImpl::GetActiveImageMetaInfoRef() const {
        return m_texture.m_activeImageMetaInfo;
    }

    std::atomic<bool>& TextureImpl::GetIsDirtyRef() const {
        return m_texture.m_isDirty;
    }

    SR_HTYPES_NS::RawPointerHolder<TextureImpl> TextureImpl::TryCreate(Texture& texture) {
        auto&& path = texture.GetResourcePath();

        if (path.View().starts_with("RenderTarget/")) {
            SR_LOG("TextureImpl::TryCreate() : loading render target texture: {}", path);
            auto&& parts = SR_UTILS_NS::StringUtils::SplitView(path.View(), "/");
            if (parts.size() != 3) {
                SR_ERROR("TextureImpl::TryCreate() : invalid render target texture path! Path: {}", path);
                return SR_HTYPES_NS::RawPointerHolder<TextureImpl>();
            }
            SR_HTYPES_NS::RawPointerHolder<TextureImpl> pImpl = (TextureImpl*)(new TextureImplRenderTarget(texture, parts));
            texture.m_textureData = TextureData::CreateEmpty();
            return pImpl;
        }
        else if (path.View().starts_with("FontAtlas/")) {
            SR_LOG("TextureImpl::TryCreate() : loading font atlas texture: {}", path);
            auto&& parts = SR_UTILS_NS::StringUtils::SplitView(path.View(), "/");
            if (parts.size() != 4) {
                SR_ERROR("TextureImpl::TryCreate() : invalid font atlas texture path! Path: {}", path);
                return SR_HTYPES_NS::RawPointerHolder<TextureImpl>();
            }
            SR_HTYPES_NS::RawPointerHolder<TextureImpl> pImpl = (TextureImpl*)(new TextureImplFontAtlas(texture, parts));
            return pImpl;
        }

        return SR_HTYPES_NS::RawPointerHolder<TextureImpl>();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    TextureImplRenderTarget::TextureImplRenderTarget(Texture& texture, std::vector<std::string_view>& parts)
        : Super(texture)
    {
        m_name = parts[1];
        if (parts[2] == "Depth") {
            m_depth = true;
            m_layer = -1;
        }
        else {
            m_layer = SR_UTILS_NS::LexicalCast<uint32_t>(parts[2]);
        }
    }


    void TextureImplRenderTarget::PrepareFrame() {
        auto&& pRenderContext = m_texture.GetRenderContext();
        SR_GTYPES_NS::Framebuffer* pFramebuffer = nullptr;
        if (auto&& pRT = pRenderContext->FindRenderTarget(m_name)) {
            pFramebuffer = pRT->GetFramebuffer();
        }
        if (pFramebuffer && pFramebuffer->Update() && pFramebuffer->IsValid()) {
            if (m_depth) {
                GetTextureIdRef() = pFramebuffer->GetDepthTexture(m_layer, 0);
            }
            else {
                GetTextureIdRef() = pFramebuffer->GetColorTexture(m_layer, 0);
            }
        }
        else {
            GetTextureIdRef() = SR_ID_INVALID;
        }
    }

    bool TextureImplRenderTarget::IsReference() const {
        return true;
    }

    bool TextureImplRenderTarget::CanBeUsed() const {
        if (GetTextureIdRef() == SR_ID_INVALID) {
            return false;
        }
        return true;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    TextureImplFontAtlas::TextureImplFontAtlas(Texture& texture, std::vector<std::string_view>& parts)
        : Super(texture)
    {
        m_fontIndex = SR_UTILS_NS::LexicalCast<uint32_t>(parts[1]);
        m_renderType = SR_UTILS_NS::EnumReflector::FromString<GlyphRenderType>(parts[2]);
        m_pageIndex = SR_UTILS_NS::LexicalCast<uint32_t>(parts[3]);
    }

    void TextureImplFontAtlas::PrepareFrame() {
        auto&& pRenderContext = m_texture.GetRenderContext();
        auto&& pFont = m_pFont.GetResource();
        if (!pFont) {
            SR_UTILS_NS::StringAtom fontId = FontIndexer::Instance().GetFontIdByIndex(m_fontIndex);
            pFont = CoreResLoader::Load<FontAsset>(fontId);
            m_pFont.SetResource(fontId);
        }

        if (pFont) {
            pFont->UpdateGlyphs();
            if (pFont->IsAtlasPageDirty(m_renderType, m_pageIndex)) {
                GetTextureDataRef() = pFont->GetAtlasTexture(m_renderType, m_pageIndex);
                if (GetTextureDataRef()) {
                    GetImageFormatRef() = GetTextureDataRef()->GetInfo().channels == 4 ? ImageFormat::RGBA8_UNORM : ImageFormat::R8_UNORM;
                }
                GetIsDirtyRef() = true;
                pFont->OnAtlasPageUploaded(m_renderType, m_pageIndex);
                m_texture.Broadcast(SR_UTILS_NS::IResource::RELOAD_BEGIN_EVENT);
                m_texture.Broadcast(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT);
                pRenderContext->SetDirty();
            }
        }
        GetImageMetaInfoRef().addressMode = AddressMode::ClampToEdge;
        GetImageMetaInfoRef().filter = TextureFilter::LINEAR;
        GetImageMetaInfoRef().loadMode = SR_UTILS_NS::ResourceLoadMode::Sync;
        GetActiveImageMetaInfoRef() = GetImageMetaInfoRef();
    }

    bool TextureImplFontAtlas::IsReference() const {
        return false;
    }

    bool TextureImplFontAtlas::CanBeUsed() const {
        if (!GetTextureDataRef()) {
            return false;
        }
        return true;
    }
}
