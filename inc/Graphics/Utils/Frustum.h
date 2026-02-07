//
// Created by Monika on 28.09.2025.
//

#ifndef SR_ENGINE_RENDER_FRUSTUM_H
#define SR_ENGINE_RENDER_FRUSTUM_H

#include <Graphics/macros.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/AABB.h>

namespace SR_GTYPES_NS {
    class Camera;
}

namespace SR_GRAPH_NS {
    struct FrustumPlane {
        SR_MATH_NS::FVector3 normal;
        SR_MATH_NS::FVector3 normalAbs;
        float_t d = 0.f;

        FrustumPlane() = default;

        FrustumPlane(const SR_MATH_NS::FVector3& norm, float_t distance)
            : normal(norm)
            , d(distance)
        {
            normalAbs = SR_MATH_NS::FVector3(std::abs(normal.x), std::abs(normal.y), std::abs(normal.z));
        }

        FrustumPlane(const SR_MATH_NS::FVector3& p1, const SR_MATH_NS::FVector3& norm)
            : normal(norm.Normalize())
            , d(normal.Dot(p1))
        {
            normalAbs = SR_MATH_NS::FVector3(std::abs(normal.x), std::abs(normal.y), std::abs(normal.z));
        }

        SR_NODISCARD float_t Distance(const SR_MATH_NS::FVector3& p) const;
    };

    enum FrustumSide : uint8_t {
        FRUSTUM_SIDE_TOP = 0,
        FRUSTUM_SIDE_BOTTOM,
        FRUSTUM_SIDE_RIGHT,
        FRUSTUM_SIDE_LEFT,
        FRUSTUM_SIDE_FAR,
        FRUSTUM_SIDE_NEAR
    };

    struct Frustum {
        std::array<FrustumPlane, 6> planes;

        using FrustumCorners = std::array<SR_MATH_NS::FVector3, 8>;

        SR_NODISCARD FrustumCorners GetFrustumCorners() const;

        SR_NODISCARD bool IsAABBVisible(const SR_MATH_NS::AABB& box) const;

        SR_NODISCARD const FrustumPlane& Top() const { return planes[FRUSTUM_SIDE_TOP]; }
        SR_NODISCARD const FrustumPlane& Bottom() const { return planes[FRUSTUM_SIDE_BOTTOM]; }
        SR_NODISCARD const FrustumPlane& Right() const { return planes[FRUSTUM_SIDE_RIGHT]; }
        SR_NODISCARD const FrustumPlane& Left() const { return planes[FRUSTUM_SIDE_LEFT]; }
        SR_NODISCARD const FrustumPlane& Far() const { return planes[FRUSTUM_SIDE_FAR]; }
        SR_NODISCARD const FrustumPlane& Near() const { return planes[FRUSTUM_SIDE_NEAR]; }
    };

    // Нормализация уравнения плоскости
    FrustumPlane NormalizeFrustumPlane(float_t a, float_t b, float_t c, float_t d);

    Frustum ExtractFrustum(const SR_GTYPES_NS::Camera& camera);

    // Извлекаем 6 плоскостей из ViewProjection матрицы
    Frustum ExtractFrustum(const SR_MATH_NS::Matrix4x4& m);

    SR_MATH_NS::FVector3 IntersectFrustumPlanes(const FrustumPlane& p1, const FrustumPlane& p2, const FrustumPlane& p3);
}

#endif //SR_ENGINE_RENDER_FRUSTUM_H
