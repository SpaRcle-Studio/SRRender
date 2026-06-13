//
// Created by Monika on 28.05.2026.
//

#ifndef SR_ENGINE_GRAPHICS_TEXTURE_IMPL_H
#define SR_ENGINE_GRAPHICS_TEXTURE_IMPL_H

#include <Graphics/Font/GlyphRenderType.h>
#include <Graphics/Font/FontAsset.h>
#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Utils/ImageMetaInfo.h>

#include <Utils/Resources/ResourceRef.h>

namespace SR_GTYPES_NS {
    class Texture;

    class TextureImpl {
    protected:
        TextureImpl(Texture& texture)
            : m_texture(texture)
        { }

    public:
        virtual ~TextureImpl() = default;

    public:
        SR_NODISCARD static SR_HTYPES_NS::RawPointerHolder<TextureImpl> TryCreate(Texture& texture);

    public:
        virtual void PrepareFrame() = 0;
        SR_NODISCARD virtual bool IsReference() const = 0;
        SR_NODISCARD virtual bool CanBeUsed() const = 0;

    protected:
        SR_NODISCARD int32_t& GetTextureIdRef() const;
        SR_NODISCARD SR_HTYPES_NS::SharedPtr<TextureData>& GetTextureDataRef() const;
        SR_NODISCARD ImageFormat& GetImageFormatRef() const;
        SR_NODISCARD ImageMetaInfo& GetImageMetaInfoRef() const;
        SR_NODISCARD ImageMetaInfo& GetActiveImageMetaInfoRef() const;
        SR_NODISCARD std::atomic<bool>& GetIsDirtyRef() const;

    protected:
        Texture& m_texture;

    };

    class TextureImplRenderTarget final : public TextureImpl {
        using Super = TextureImpl;
    public:
        explicit TextureImplRenderTarget(Texture& texture, std::vector<std::string_view>& parts);

        void PrepareFrame() override;

        SR_NODISCARD bool IsReference() const override;
        SR_NODISCARD bool CanBeUsed() const override;

    private:
        SR_UTILS_NS::StringAtom m_name;
        uint32_t m_layer = 0;
        bool m_depth = false;

    };

    class TextureImplFontAtlas final : public TextureImpl {
        using Super = TextureImpl;
    public:
        explicit TextureImplFontAtlas(Texture& texture, std::vector<std::string_view>& parts);

        void PrepareFrame() override;

        SR_NODISCARD bool IsReference() const override;
        SR_NODISCARD bool CanBeUsed() const override;

    private:
        uint16_t m_fontIndex = 0;
        uint16_t m_pageIndex = 0;
        GlyphRenderType m_renderType = GlyphRenderType::SDF;
        SR_UTILS_NS::ResourceRef<FontAsset> m_pFont;

    };
}

#endif //SR_ENGINE_GRAPHICS_TEXTURE_IMPL_H
