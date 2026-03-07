//
// Created by Nikita on 03.12.2020.
//

#ifndef SR_GRAPHICS_TEXTURE_LOADER_H
#define SR_GRAPHICS_TEXTURE_LOADER_H

#include <Graphics/Utils/ImageMetaInfo.h>

#include <Utils/Types/SharedPtr.h>
#include <Utils/Types/Function.h>
#include <Utils/FileSystem/Path.h>

namespace SR_GTYPES_NS {
    class Texture;
}

namespace SR_GRAPH_NS {
    namespace Memory {
        struct TextureConfig;
    }

    class TextureData : public SR_HTYPES_NS::SharedPtr<TextureData>, SR_UTILS_NS::NonCopyable {
        using Super = SR_HTYPES_NS::SharedPtr<TextureData>;
        using DeleterFn = SR_HTYPES_NS::Function<void(uint8_t*)>;
    private:
        TextureData();

    public:
        ~TextureData() override;

    public:
        static TextureData::Ptr Create(uint32_t width, uint32_t height, uint8_t* pData, DeleterFn&& deleter, TextureLoadInfo info);

        SR_NODISCARD bool Save(const SR_UTILS_NS::Path& path) const;

    public:
        SR_NODISCARD SR_MATH_NS::UVector2 GetSize() const { return { m_width, m_height }; }
        SR_NODISCARD uint32_t GetWidth() const { return m_width; }
        SR_NODISCARD uint32_t GetHeight() const { return m_height; }
        SR_NODISCARD uint8_t GetChannels() const;
        SR_NODISCARD const uint8_t* GetData() const { return m_data; }
        SR_NODISCARD uint32_t GetNumberOfBytes() const;
        SR_NODISCARD SR_UTILS_NS::Path GetPath() const { return m_path; }
        SR_NODISCARD const TextureLoadInfo& GetInfo() const { return m_info; }

        void SetPath(const SR_UTILS_NS::Path& path) { m_path = path; }

    private:
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint8_t* m_data = nullptr;
        SR_UTILS_NS::Path m_path;
        TextureLoadInfo m_info;
        DeleterFn m_deleter;
    };

    class TextureLoader {
        static constexpr uint64_t VERSION = 1003;
    public:
        TextureLoader() = delete;
        TextureLoader(const TextureLoader&) = delete;
        TextureLoader(TextureLoader&&) = delete;
        ~TextureLoader() = delete;

    public:
        static TextureData::Ptr Load(const SR_UTILS_NS::Path& path, TextureLoadInfo info);
        static TextureData::Ptr LoadFromMemory(const std::string& data, const ImageMetaInfo& meta);

        static bool Free(unsigned char* data);
        static int GetAlignedChannels(ImageFormat format);
        static bool IsAllowedChannelsCount(uint8_t channels);

    private:
        static void AsyncCompressTexture(const TextureData::Ptr& pData, TextureCompression compression);
        static void CompressTexture(const TextureData::Ptr& pData, TextureCompression compression);
        static TextureData::Ptr LoadFromCache(const SR_UTILS_NS::Path& path);

    };
}

#endif //SR_GRAPHICS_TEXTURE_LOADER_H
