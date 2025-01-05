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

    class Text : public Mesh {
        SR_CLASS()
        using Super = Mesh;
    public:
        Text();
        ~Text() override;

    public:
        bool InitializeEntity() noexcept override;

        SR_NODISCARD int64_t GetSortingPriority() const override;
        SR_NODISCARD bool HasSortingPriority() const override;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetMeshLayer() const override;
        const SR_MATH_NS::Matrix4x4& GetMatrix() const override;

        void UseMaterial() override;
        void UseModelMatrix() override;

        void UseSamplers() override;

        SR_NODISCARD MeshType GetMeshType() const noexcept override { return MeshType::Text; }

        SR_NODISCARD bool IsFlatMesh() const noexcept override;

        /// TODO: можно сделать при помощи 4х вершин
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

        SR_MATH_NS::UVector2 m_fontSize = { 512, 512 };

        /// @property
        bool m_is3D = false;
        /// @property
        bool m_kerning = true;
        /// @property
        bool m_debug = false;
        /// @property
        bool m_preprocessor = false;
        /// @property
        bool m_localization = false;

        SR_HTYPES_NS::UnicodeString m_text;

    };
}

#endif //SR_ENGINE_I_TEXT_H
