//
// Created by Monika on 30.07.2022.
//

#ifndef SR_ENGINE_SPRITE_H
#define SR_ENGINE_SPRITE_H

#include <Graphics/Types/Geometry/MeshComponent.h>

namespace SR_GTYPES_NS {
    const std::vector<uint32_t> SR_SPRITE_INDICES = { 0, 1, 2, 0, 2, 3 }; /// NOLINT

    const std::vector<Vertices::UIVertex> SR_SPRITE_VERTICES = { /// NOLINT
        { {  1.000000,  1.000000,  0.000000 }, { 0.000000, 1.000000 } },
        { { -1.000000,  1.000000, -0.000000 }, { 1.000000, 1.000000 } },
        { { -1.000000, -1.000000, -0.000000 }, { 1.000000, 0.000000 } },
        { {  1.000000, -1.000000,  0.000000 }, { 0.000000, 0.000000 } }
    };

    class SR_RENDERER_DLL_API Sprite : public SR_GTYPES_NS::Mesh {
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

        SR_NODISCARD SR_MATH_NS::FVector2 GetTextureBorder() const;
        SR_NODISCARD SR_MATH_NS::FVector2 GetWindowBorder() const;

        void SetTextureBorder(const SR_MATH_NS::FVector2& border);
        void SetWindowBorder(const SR_MATH_NS::FVector2& border);

    protected:
        bool Calculate() override;

    protected:
        /// @property
        bool m_sliced = true;
        /// @property @setter(SetTextureBorder) @drag(0.01f) @resetValue(SR_MATH_NS::FVector2(0.15f, 0.15f))
        SR_MATH_NS::FVector2 m_textureBorder = 0.15f;
        /// @property @setter(SetWindowBorder) @drag(0.01f) @resetValue(SR_MATH_NS::FVector2(0.15f, 0.15f))
        SR_MATH_NS::FVector2 m_windowBorder = 0.15f;

    };
}

#endif //SR_ENGINE_SPRITE_H
