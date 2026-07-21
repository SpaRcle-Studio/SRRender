//
// Created by Monika on 29.04.2026.
//

#ifndef SR_ENGINE_TEXT_MESH_H
#define SR_ENGINE_TEXT_MESH_H

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Font/FontAsset.h>
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

    class TextMesh : public UIRenderComponent, public UI::IFindCanvasOwner {
        SR_CLASS()
        using Super = UIRenderComponent;
    public:
        SR_UTILS_NS::VertexLayoutDescriptionsRef GetShaderVertexLayoutDescriptions() const noexcept override;

        void Draw() override;
        void UseSamplers(Shader& shader) override;
        void UseMaterial(Shader& shader) override;
        void UseModelMatrix(Shader& shader) override;
        void FreeVideoMemory() override;

        SR_NODISCARD std::optional<int32_t> GetVBO() const override;
        bool Bind() override;

    private:
        bool Calculate();
        void OnTextDirty();
        void OnMatrixDirty() override;

    private:
        int32_t m_VBO = SR_ID_INVALID;
        uint32_t m_instancesCount = 0;
        SR_UTILS_NS::Subscription m_onFontReloadedSubscription;

    private:
        /// @property @onChanged(OnTextDirty)
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: font)
        SR_UTILS_NS::ResourceRef<FontAsset> m_font;
        /// @property @onChanged(OnTextDirty)
        /// @customArg(text-box: enabled)
        SR_UTILS_NS::String m_text;
        /// @property @range(0.1f, std::numeric_limits<float_t>::max())
        /// @onChanged(OnTextDirty)
        float_t m_fontSize = 16.f;
        /// @property @group(Alignment) @onChanged(OnTextDirty)
        TextAlignmentHorizontal m_horizontalAlignment = TextAlignmentHorizontal::Left;
        /// @property @group(Alignment) @onChanged(OnTextDirty)
        TextAlignmentVertical m_verticalAlignment = TextAlignmentVertical::Top;

        /// @property @group(Features) @onChanged(OnTextDirty)
        bool m_kerning = true;

    };
}

#endif //SR_ENGINE_TEXT_MESH_H