//
// Created by Monika on 20.06.2024.
//

#ifndef SR_ENGINE_I_TEXT_H
#define SR_ENGINE_I_TEXT_H

#include <Graphics/Types/Vertices.h>
#include <Graphics/Types/Mesh.h>
#include <Utils/Types/UnicodeString.h>

namespace SR_GTYPES_NS {
    class Font;

    class IText : public Mesh {
        using Super = Mesh;
    public:
        IText();
        ~IText() override;

    public:
        void UseMaterial() override;
        void UseModelMatrix() override;

        void UseSamplers() override;

        SR_NODISCARD bool IsFlatMesh() const noexcept override;

        SR_NODISCARD uint32_t GetIndicesCount() const override { return 6; }

        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD SR_FORCE_INLINE bool GetKerning() const noexcept { return m_kerning; }
        SR_NODISCARD SR_FORCE_INLINE bool IsDebugEnabled() const noexcept { return m_debug; }
        SR_NODISCARD SR_FORCE_INLINE bool IsPreprocessorEnabled() const noexcept { return m_preprocessor; }
        SR_NODISCARD SR_FORCE_INLINE bool IsLocalizationEnabled() const noexcept { return m_localization; }
        SR_NODISCARD SR_FORCE_INLINE Font* GetFont() const noexcept { return m_font; }
        SR_NODISCARD SR_FORCE_INLINE SR_MATH_NS::UVector2 GetFontSize() const noexcept { return m_fontSize; }

        SR_NODISCARD bool IsSupportVBO() const override { return false; }

        SR_NODISCARD uint32_t GetAtlasWidth() const noexcept { return m_atlasSize.x; }
        SR_NODISCARD uint32_t GetAtlasHeight() const noexcept { return m_atlasSize.y; }

        SR_NODISCARD const SR_HTYPES_NS::UnicodeString& GetText() const { return m_text; }

        void SetText(const std::string& text);
        void SetText(const std::u16string& text);
        void SetText(const std::u32string& text);
        void SetKerning(bool enabled);
        void SetDebug(bool enabled);
        void SetFont(Font* pFont);
        void SetFont(const SR_UTILS_NS::Path& path);
        void SetFontSize(const SR_MATH_NS::UVector2& size);
        void SetUseLocalization(bool enabled);
        void SetUsePreprocessor(bool enabled);

        bool Calculate() override;
        void FreeVideoMemory() override;

        SR_NODISCARD virtual RenderScene* GetTextRenderScene() const = 0;

    protected:
        void OnTextDirty();
        SR_NODISCARD bool BuildAtlas();

    protected:
        Font* m_font = nullptr;

        int32_t m_id = SR_ID_INVALID;
        SR_MATH_NS::UVector2 m_atlasSize;

        SR_MATH_NS::UVector2 m_fontSize = SR_MATH_NS::UVector2(512, 512);

        bool m_is3D = false;
        bool m_kerning = true;
        bool m_debug = false;
        bool m_preprocessor = false;
        bool m_localization = false;

        SR_HTYPES_NS::UnicodeString m_text;

    };
}

#endif //SR_ENGINE_I_TEXT_H
