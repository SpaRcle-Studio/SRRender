//
// Created by Nikita on 17.11.2020.
//

#ifndef SR_ENGINE_TEXTURE_H
#define SR_ENGINE_TEXTURE_H

#include <Graphics/Utils/ImageMetaInfo.h>
#include <Graphics/Memory/IGraphicsResource.h>

#include <Utils/Resources/IResource.h>

namespace SR_GRAPH_NS {
    class TextureLoader;
    class RenderContext;
    class Render;
    class TextureData;
    class FontAsset;
}

namespace SR_GTYPES_NS {
    class TextureImpl;
    
    /// @extension(png, jpg, jpeg, tga, bmp)
    class Texture : public SR_UTILS_NS::IResource, public Memory::IGraphicsResource {
        SR_CLASS()
        friend class TextureImpl;
        friend class ::SR_GRAPH_NS::TextureLoader;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Texture>;

    public:
        Texture();
        ~Texture() override;

    public:
        static Texture::Ptr LoadFromMemory(const std::string& data, const ImageMetaInfo& config);

    public:
        SR_NODISCARD SR_MATH_NS::FRect GetBorder() const noexcept;
        SR_NODISCARD float_t GetPPU() const noexcept;
        SR_NODISCARD uint32_t GetWidth() const noexcept;
        SR_NODISCARD uint32_t GetHeight() const noexcept;
        SR_NODISCARD uint32_t GetChannels() const noexcept;
        SR_NODISCARD int32_t GetId() noexcept;
        SR_NODISCARD void* GetDescriptor();
        SR_NODISCARD SR_UTILS_NS::Path GetAssociatedPath() const override;
        SR_NODISCARD const ImageMetaInfo& GetImageMetaInfo() const noexcept { return m_imageMetaInfo; }

        SR_NODISCARD bool IsAllowedToRevive() const override { return true; }
        SR_NODISCARD bool CanBeUsed() const;

        void FreeVMemory() override;
        void SetImageMetaInfo(const ImageMetaInfo& meta);
        void PrepareFrame();

    protected:
        bool Unload() override;
        bool Load() override;

    private:
        bool Calculate();
        void FreeTextureData();
        void SetImageMetaInfoInternal(const ImageMetaInfo& meta);
        void OnAsyncLoaded(SR_HTYPES_NS::SharedPtr<TextureData>&& pTextureData);

    private:
        SR_HTYPES_NS::SharedPtr<TextureData> m_textureData;

        SR_HTYPES_NS::RawPointerHolder<TextureImpl> m_impl;
        int32_t m_id = SR_ID_INVALID;

        std::atomic<bool> m_hasErrors = false;
        std::atomic<bool> m_isDirty = true;

        std::optional<uint64_t> m_syncLoadTaskId;

        ImageFormat m_format = ImageFormat::Unknown;
        ImageMetaInfo m_imageMetaInfo;
        ImageMetaInfo m_activeImageMetaInfo;

    };
}

#endif //SR_ENGINE_TEXTURE_H
