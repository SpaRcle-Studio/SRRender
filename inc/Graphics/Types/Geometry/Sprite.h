//
// Created by Monika on 30.07.2022.
//

#ifndef SR_ENGINE_SPRITE_H
#define SR_ENGINE_SPRITE_H

#include <Graphics/Types/Geometry/MeshComponent.h>

#include <Utils/Math/Rect.h>

namespace SR_GTYPES_NS {
    const std::vector<uint32_t> SR_SPRITE_INDICES = { 0, 1, 2, 0, 2, 3 }; /// NOLINT

    const std::vector<Vertices::UIVertex> SR_SPRITE_VERTICES = { /// NOLINT
        { {  1.000000,  1.000000,  0.000000 }, { 0.000000, 1.000000 } },
        { { -1.000000,  1.000000, -0.000000 }, { 1.000000, 1.000000 } },
        { { -1.000000, -1.000000, -0.000000 }, { 1.000000, 0.000000 } },
        { {  1.000000, -1.000000,  0.000000 }, { 0.000000, 0.000000 } }
    };

    SR_ENUM_NS_CLASS_T(SliceMode, uint8_t,
        None,
        Auto,
        Manual
    );

    SR_ENUM_NS_CLASS_T(SpriteMode, uint8_t,
        Sliced,
        Filled
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

    /// @category(UI)
    class SR_GRAPHICS_DLL_API Sprite : public SR_GTYPES_NS::Mesh {
        SR_CLASS()
        using Super = SR_GTYPES_NS::Mesh;
    public:
        typedef Vertices::UIVertex VertexType;

    public:
        void UseMaterial() override;
        void UseModelMatrix() override;

        bool IsSupportVBO() const override;

        bool BindMesh() override;

        SR_NODISCARD MeshType GetMeshType() const noexcept override;

        SR_NODISCARD uint32_t GetIndicesCount() const override;
        SR_NODISCARD bool IsFlatMesh() const noexcept override;
        SR_NODISCARD std::string GetMeshIdentifier() const override;

        SR_NODISCARD SR_MATH_NS::FRect GetTextureBorder() const;
        SR_NODISCARD SR_MATH_NS::FRect GetWindowBorder() const;

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
        bool Calculate() override;
        void ApplyFillModeParams(Shader* pShader);
        void ApplySliceModeParams(Shader* pShader);

    protected:
        /// @property @setter(SetSpriteMode)
        SpriteMode m_spriteMode = SpriteMode::Sliced;

        /// @property @setter(SetFillMethod) @propertyCondition(This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Filled)
        SpriteFillMethod m_fillMethod = SpriteFillMethod::Radial90;
        /// @property @setter(SetFillOrigin) @propertyCondition(This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Filled)
        SpriteFillOrigin m_fillOrigin = SpriteFillOrigin::BottomLeft;
        /// @property @drag(0.01f) @resetValue(1.f) @propertyCondition(This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Filled) @setter(SetFillAmount)
        float_t m_fillAmount = 1.f;
        /// @property @propertyCondition(This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Filled) @setter(SetFillClockwise)
        bool m_fillClockwise = true;

        /// @property @setter(SetSliceMode) @propertyCondition(This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Sliced)
        SliceMode m_sliceMode = SliceMode::Auto;
        /// @property @setter(SetFillCenter) @propertyCondition(This.m_sliceMode != SR_GTYPES_NS::SliceMode::None && This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Sliced)
        bool m_fillCenter = true;
        /// @property @drag(0.01f) @resetValue(1.f) @propertyCondition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Auto && This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Sliced) @setter(SetPixelsPerUnitMultiplier)
        float_t m_pixelsPerUnitMultiplier = 1.f;
        /// @property @setter(SetTextureBorder) @drag(0.01f) @propertyCondition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Manual && This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Sliced)
        SR_MATH_NS::FRect m_textureBorder;
        /// @property @setter(SetWindowBorder) @drag(0.01f) @propertyCondition(This.m_sliceMode == SR_GTYPES_NS::SliceMode::Manual && This.m_spriteMode == SR_GTYPES_NS::SpriteMode::Sliced)
        SR_MATH_NS::FRect m_windowBorder;

    };
}

#endif //SR_ENGINE_SPRITE_H
