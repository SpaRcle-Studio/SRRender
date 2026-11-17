//
// Created by Monika on 30.07.2022.
//

#ifndef SR_ENGINE_SPRITE_H
#define SR_ENGINE_SPRITE_H

#include <Graphics/Types/Mesh.h>
#include <Graphics/UI/Canvas.h>

#include <Utils/Math/Rect.h>
#include <Utils/UI/ILayoutElement.h>
#include <Graphics/Types/Texture.h>
#include <Utils/Common/Subscription.h>

namespace SR_GTYPES_NS {
    SR_ENUM_NS_CLASS_T(SliceMode, uint8_t,
        None,
        Auto,
        Manual
    );

    SR_ENUM_NS_CLASS_T(SpriteMode, uint8_t,
        Simple = 1 << 0,
        Sliced = 1 << 1,
        Filled = 1 << 2,
        SlicedFilled = Sliced | Filled
    );

    SR_ENUM_NS_CLASS_T(SpriteFillOrigin, uint8_t,
        Left, Right, Bottom, Top,
        BottomLeft, TopLeft,
        TopRight, BottomRight
    );

    SR_ENUM_NS_CLASS_T(SpriteFillMethod, uint8_t,
        Horizontal, Vertical,
        Radial90, Radial180, Radial360
    );

    class Sprite;
    SR_NODISCARD bool IsSpriteFillOriginApplicable(const Sprite& sprite, SpriteFillOrigin origin);

    /// @category(UI)
    class SR_GRAPHICS_DLL_API Sprite : public SR_GTYPES_NS::UIRenderComponent, public SR_GRAPH_UI_NS::IFindCanvasOwner, public SR_UTILS_NS::ILayoutElement {
        SR_CLASS()
        using Super = SR_GTYPES_NS::UIRenderComponent;
    public:
        /// @method
        bool SetTexturePath(const SR_UTILS_NS::Path& path);
        /// @method @evaluate
        SR_MATH_NS::FVector2 GetTextureSize() const;
        void UseSamplers(SR_GTYPES_NS::Shader& shader) override;
        SR_UTILS_NS::ILayoutElement* GetLayoutElement() noexcept override { return m_texture.GetId().empty() ? nullptr : this; }
        void CalculateLayoutInput(bool horizontal) override;
        void OnPostLoad() override;
        float_t GetMinWidth() const override { return 0.f; }
        float_t GetMinHeight() const override { return 0.f; }
        float_t GetPreferredWidth() const override { return m_preferredSize.x; }
        float_t GetPreferredHeight() const override { return m_preferredSize.y; }
        float_t GetFlexibleWidth() const override { return -1.f; }
        float_t GetFlexibleHeight() const override { return -1.f; }
        int32_t GetLayoutPriority() const override { return 0; }

        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;
        void UseModelMatrix(SR_GTYPES_NS::Shader& shader) override;

        SR_NODISCARD SR_MATH_NS::FRect GetTextureBorder() const;
        SR_NODISCARD SR_MATH_NS::FRect GetWindowBorder() const;
        SR_NODISCARD SpriteFillMethod GetFillMethod() const { return m_fillMethod; }

        void SetTextureBorder(const SR_MATH_NS::FRect& border);
        void SetWindowBorder(const SR_MATH_NS::FRect& border);
        void SetSliceMode(SliceMode mode);
        void SetSpriteMode(SpriteMode mode);
        void SetFillCenter(bool fill);
        void SetPixelsPerUnitMultiplier(float_t multiplier);
        void SetFillMethod(SpriteFillMethod method);
        void SetFillOrigin(SpriteFillOrigin origin);
        void SetFillAmount(float_t amount);
        void SetFillClockwise(bool clockwise);

    protected:
        void OnTextureChanged();
        SR_MATH_NS::FVector2 m_preferredSize;
        SR_UTILS_NS::Subscription m_textureReloadSubscription;
        /// @property @onChanged(OnTextureChanged)
        SR_UTILS_NS::ResourceRef<Texture> m_texture;
        void Draw() override;
        void ApplyFillModeParams(Shader* pShader);
        void ApplySliceModeParams(Shader* pShader);

    protected:
        /// @property @setter(SetSpriteMode)
        SpriteMode m_spriteMode = SpriteMode::Sliced;

        /// @property @setter(SetFillMethod) @condition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled))
        SpriteFillMethod m_fillMethod = SpriteFillMethod::Radial90;
        /// @property @setter(SetFillOrigin) @condition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled))
        /// @enumFilter(IsSpriteFillOriginApplicable)
        SpriteFillOrigin m_fillOrigin = SpriteFillOrigin::BottomLeft;
        /// @property @drag(0.01f) @resetValue(1.f) @condition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled)) @setter(SetFillAmount) @range(0.f, 1.f)
        float_t m_fillAmount = 1.f;
        /// @property @condition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled)) @setter(SetFillClockwise)
        bool m_fillClockwise = true;

        /// @property @setter(SetSliceMode) @condition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        SliceMode m_sliceMode = SliceMode::Auto;
        /// @property @setter(SetFillCenter) @condition(This.m_sliceMode != SR_GTYPES_NS::SliceMode::None && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        bool m_fillCenter = true;
        /// @property @drag(0.01f) @resetValue(1.f) @condition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Auto && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced)) @setter(SetPixelsPerUnitMultiplier)
        float_t m_pixelsPerUnitMultiplier = 1.f;
        /// @property @setter(SetTextureBorder) @drag(0.01f) @condition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Manual && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        SR_MATH_NS::FRect m_textureBorder;
        /// @property @setter(SetWindowBorder) @drag(0.01f) @condition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Manual && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        SR_MATH_NS::FRect m_windowBorder;

    };
}

#endif //SR_ENGINE_SPRITE_H
