//
// Created by Monika on 30.07.2022.
//

#ifndef SR_ENGINE_SPRITE_H
#define SR_ENGINE_SPRITE_H

#include <Graphics/Types/Mesh.h>

#include <Utils/Math/Rect.h>

namespace SR_GTYPES_NS {
    SR_ENUM_NS_CLASS_T(SliceMode, uint8_t,
        None,
        Auto,
        Manual
    );

    SR_ENUM_NS_CLASS_T(SpriteMode, uint8_t,
        Sliced = 1 << 0,
        Filled = 1 << 1,
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
    class SR_GRAPHICS_DLL_API Sprite : public SR_GTYPES_NS::UIRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::UIRenderComponent;
    public:
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
        void Draw() override;
        void ApplyFillModeParams(Shader* pShader);
        void ApplySliceModeParams(Shader* pShader);

    protected:
        /// @property @setter(SetSpriteMode)
        SpriteMode m_spriteMode = SpriteMode::Sliced;

        /// @property @setter(SetFillMethod) @propertyCondition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled))
        SpriteFillMethod m_fillMethod = SpriteFillMethod::Radial90;
        /// @property @setter(SetFillOrigin) @propertyCondition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled))
        /// @enumFilter(IsSpriteFillOriginApplicable)
        SpriteFillOrigin m_fillOrigin = SpriteFillOrigin::BottomLeft;
        /// @property @drag(0.01f) @resetValue(1.f) @propertyCondition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled)) @setter(SetFillAmount) @range(0.f, 1.f)
        float_t m_fillAmount = 1.f;
        /// @property @propertyCondition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Filled)) @setter(SetFillClockwise)
        bool m_fillClockwise = true;

        /// @property @setter(SetSliceMode) @propertyCondition(SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        SliceMode m_sliceMode = SliceMode::Auto;
        /// @property @setter(SetFillCenter) @propertyCondition(This.m_sliceMode != SR_GTYPES_NS::SliceMode::None && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        bool m_fillCenter = true;
        /// @property @drag(0.01f) @resetValue(1.f) @propertyCondition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Auto && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced)) @setter(SetPixelsPerUnitMultiplier)
        float_t m_pixelsPerUnitMultiplier = 1.f;
        /// @property @setter(SetTextureBorder) @drag(0.01f) @propertyCondition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Manual && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        SR_MATH_NS::FRect m_textureBorder;
        /// @property @setter(SetWindowBorder) @drag(0.01f) @propertyCondition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Manual && SR_MATH_NS::IsMaskIncludedSubMask(This.m_spriteMode, SR_GTYPES_NS::SpriteMode::Sliced))
        SR_MATH_NS::FRect m_windowBorder;

    };
}

#endif //SR_ENGINE_SPRITE_H
