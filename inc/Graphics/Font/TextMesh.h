//
// Created by Monika on 29.04.2026.
//

#ifndef SR_ENGINE_TEXT_MESH_H
#define SR_ENGINE_TEXT_MESH_H

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Font/FontAsset.h>
#include <Graphics/Font/TextStyle.h>
#include <Utils/UI/ILayoutElement.h>
#include <Utils/Localization/Grapheme.h>
#include <Utils/Common/Vertices.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/UI/Canvas.h>

#include <Utils/Types/UnicodeString.h>
#include <Utils/Common/Subscription.h>

namespace SR_GTYPES_NS {
    SR_ENUM_NS_CLASS_T(TextAlignmentVertical, uint8_t,
        Top,
        Middle,
        Bottom,
        Baseline,
        Midline,
        Capline
    );

    SR_ENUM_NS_CLASS_T(TextAlignmentHorizontal, uint8_t,
        Left,
        Center,
        Right,
        Justified,
        Flush,
        GeometryCenter
    );

    class SR_GRAPHICS_DLL_API TextMesh : public UIRenderComponent, public UI::IFindCanvasOwner, public SR_UTILS_NS::ILayoutElement {
        SR_CLASS()
    public:
        SR_UTILS_NS::VertexLayoutDescriptionsRef GetShaderVertexLayoutDescriptions() const noexcept override;

        void Draw() override;
        void UseSamplers(Shader& shader) override;
        void UseMaterial(Shader& shader) override;
        void UseModelMatrix(Shader& shader) override;
        void FreeVideoMemory() override;
        void OnDestroy() override;

        /// @method
        void SetText(SR_UTILS_NS::StringView text);
        /// @method
        void SetFontPath(SR_UTILS_NS::StringAtom path);
        /// @method
        void SetCharacterSpacing(float_t spacing);
        /// @method @evaluate
        float_t GetCharacterSpacing() const noexcept { return m_characterSpacing; }

        /// @method
        void SetVisibleGraphemeCount(uint32_t count);
        /// @method @evaluate
        uint32_t GetGraphemeCount();
        /// @method @evaluate
        uint32_t GetLoadedGlyphCount();
        /// @method
        void AddStyle(const TextStyle& style);
        /// @method
        void ClearStyles();
        /// @method
        void SetCharacterEffect(float_t time, float_t waveAmplitude, float_t shakeAmplitude, float_t frequency);
        /// @method @evaluate
        SR_MATH_NS::FRect GetTextBounds();
        /// @method @evaluate
        uint64_t GetLayoutInputRevision() const noexcept { return m_layoutInputRevision; }

        SR_UTILS_NS::ILayoutElement* GetLayoutElement() noexcept override { return this; }
        void CalculateLayoutInput(bool horizontal) override;
        /// @method @evaluate
        float_t GetMinWidth() const override { return m_minWidth; }
        /// @method @evaluate
        float_t GetPreferredWidth() const override { return m_preferredSize.x; }
        float_t GetFlexibleWidth() const override { return -1.f; }
        float_t GetMinHeight() const override { return m_preferredSize.y; }
        /// @method @evaluate
        float_t GetPreferredHeight() const override { return m_preferredSize.y; }
        float_t GetFlexibleHeight() const override { return -1.f; }
        int32_t GetLayoutPriority() const override { return 0; }

        SR_NODISCARD SR_UTILS_NS::Optional<int32_t> GetVBO() const override;
        bool Bind() override;

    private:
        bool Calculate();
        void OnTextDirty();
        void OnMatrixDirty() override;
        bool BuildGlyphs(SR_UTILS_NS::Vector<PositionedGlyph>& glyphs, SR_UTILS_NS::Vector<float_t>& scales);
        TextStyle ResolveStyle(uint32_t grapheme) const;
        bool UpdateGraphemes();

    private:
        struct Batch {
            uint64_t key = 0;
            int32_t vbo = SR_ID_INVALID;
            int32_t descriptor = SR_ID_INVALID;
            bool dirty = true;
            uint32_t count = 0;
            SR_UTILS_NS::VertexDataBuffer buffer;
            SR_UTILS_NS::ResourceRef<Texture> texture;
            SR_UTILS_NS::Subscription textureReloadSubscription;
        };
        SR_UTILS_NS::Vector<Batch> m_batches;
        uint32_t m_drawBatch = 0;
        uint32_t m_instancesCount = 0;
        uint32_t m_visibleGraphemes = std::numeric_limits<uint32_t>::max();
        uint32_t m_graphemeCount = 0;
        bool m_graphemesDirty = true;
        bool m_validUTF8 = true;
        SR_UTILS_NS::Vector<SR_UTILS_NS::Localization::GraphemeCodepoint> m_codepoints;
        SR_UTILS_NS::Vector<uint32_t> m_glyphGraphemes;
        SR_MATH_NS::FVector2 m_preferredSize;
        float_t m_minWidth = 0.f;
        bool m_metricsDirty = true;
        SR_MATH_NS::FVector2 m_calculatedLayoutSize;
        uint64_t m_layoutInputRevision = 0;
        SR_MATH_NS::FVector4 m_characterEffect;
        SR_UTILS_NS::Subscription m_onFontReloadedSubscription;

    private:
        /// @property @onChanged(OnTextDirty)
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: font)
        SR_UTILS_NS::ResourceRef<FontAsset> m_font = "Engine/Fonts/Default.font";
        /// @property @onChanged(OnTextDirty)
        /// @customArg(text-box: enabled)
        SR_UTILS_NS::String m_text;
        /// @property @onChanged(OnTextDirty)
        SR_UTILS_NS::Vector<TextStyle> m_styles;
        /// @property @onChanged(OnTextDirty)
        bool m_wordWrap = true;
        /// @property @range(0.1f, std::numeric_limits<float_t>::max())
        /// @onChanged(OnTextDirty)
        float_t m_fontSize = 16.f;
        /// @property @onChanged(OnTextDirty) @drag(0.1f) @tooltip(Additional spacing between graphemes in layout units. Negative values tighten text.)
        float_t m_characterSpacing = 0.f;
        /// @property @group(Alignment) @onChanged(OnTextDirty)
        TextAlignmentHorizontal m_horizontalAlignment = TextAlignmentHorizontal::Left;
        /// @property @group(Alignment) @onChanged(OnTextDirty)
        TextAlignmentVertical m_verticalAlignment = TextAlignmentVertical::Top;

        /// @property @group(Features) @onChanged(OnTextDirty)
        bool m_kerning = true;

    };
}

#endif //SR_ENGINE_TEXT_MESH_H
