//
// Created by Monika on 07.04.2024.
//

#include <Graphics/Utils/Frustum.h>
#include <Graphics/Types/Camera.h>

#include <Utils/ECS/Transform.h>

namespace SR_GRAPH_NS {
    float_t FrustumPlane::Distance(const SR_MATH_NS::FVector3& p) const {
        return normal.Dot(p) + d;
        //return normal.Dot(p) - d;
    }

    FrustumPlane NormalizeFrustumPlane(float_t a, float_t b, float_t c, float_t d) {
        const SR_MATH_NS::FVector3 n(a, b, c);
        const float_t len = n.Length();
        return { n / len, d / len };
    }

    Frustum ExtractFrustum(const SR_GTYPES_NS::Camera& camera) {
        Frustum frustum;

        const float_t fovY = camera.GetFOV();
        const float_t aspect = camera.GetAspect();
        const float_t zNear = camera.GetNear();
        const float_t zFar = camera.GetFar();

        const SR_MATH_NS::FVector3& front = -camera.GetTransform()->Forward();
        //const SR_MATH_NS::FVector3& front = camera.GetTransform()->Forward();
        const SR_MATH_NS::FVector3& right = camera.GetTransform()->Right();
        const SR_MATH_NS::FVector3& up = camera.GetTransform()->Up();

        const float halfVSide = zFar * tanf(fovY * .5f);
        const float halfHSide = halfVSide * aspect;
        const SR_MATH_NS::FVector3 frontMultFar = front * zFar;
        //const SR_MATH_NS::FVector3 position = -camera.GetPosition();
        const SR_MATH_NS::FVector3 position = -camera.GetPosition();

        frustum.planes[FrustumSide::FRUSTUM_SIDE_NEAR] = { position + front * zNear, front };
        frustum.planes[FrustumSide::FRUSTUM_SIDE_FAR] = { position + frontMultFar, -front };
        frustum.planes[FrustumSide::FRUSTUM_SIDE_RIGHT] = { position, SR_MATH_NS::FVector3::Cross(frontMultFar - right * halfHSide, up) };
        frustum.planes[FrustumSide::FRUSTUM_SIDE_LEFT] = { position, SR_MATH_NS::FVector3::Cross(up,frontMultFar + right * halfHSide) };
        frustum.planes[FrustumSide::FRUSTUM_SIDE_TOP] = { position, SR_MATH_NS::FVector3::Cross(right, frontMultFar - up * halfVSide) };
        frustum.planes[FrustumSide::FRUSTUM_SIDE_BOTTOM] = { position, SR_MATH_NS::FVector3::Cross(frontMultFar + up * halfVSide, right) };

        return frustum;
    }

    Frustum ExtractFrustum(const SR_MATH_NS::Matrix4x4 &m) {
        SR_TRACY_ZONE;

        Frustum f;

        f.planes[FrustumSide::FRUSTUM_SIDE_LEFT] = NormalizeFrustumPlane(
                m[0][3] + m[0][0],
                m[1][3] + m[1][0],
                m[2][3] + m[2][0],
                m[3][3] + m[3][0]); // left

        f.planes[FrustumSide::FRUSTUM_SIDE_RIGHT] = NormalizeFrustumPlane(
                m[0][3] - m[0][0],
                m[1][3] - m[1][0],
                m[2][3] - m[2][0],
                m[3][3] - m[3][0]); // right

        f.planes[FrustumSide::FRUSTUM_SIDE_BOTTOM] = NormalizeFrustumPlane(
                m[0][3] + m[0][1],
                m[1][3] + m[1][1],
                m[2][3] + m[2][1],
                m[3][3] + m[3][1]); // bottom

        f.planes[FrustumSide::FRUSTUM_SIDE_TOP] = NormalizeFrustumPlane(
                m[0][3] - m[0][1],
                m[1][3] - m[1][1],
                m[2][3] - m[2][1],
                m[3][3] - m[3][1]); // top

        f.planes[FrustumSide::FRUSTUM_SIDE_NEAR] = NormalizeFrustumPlane(
                m[0][3] + m[0][2],
                m[1][3] + m[1][2],
                m[2][3] + m[2][2],
                m[3][3] + m[3][2]); // near

        f.planes[FrustumSide::FRUSTUM_SIDE_FAR] = NormalizeFrustumPlane(
                m[0][3] - m[0][2],
                m[1][3] - m[1][2],
                m[2][3] - m[2][2],
                m[3][3] - m[3][2]); // far

        return f;
    }

    SR_MATH_NS::FVector3 IntersectFrustumPlanes(const FrustumPlane& p1, const FrustumPlane& p2, const FrustumPlane& p3) {
        const SR_MATH_NS::FVector3 n1 = p1.normal;
        const SR_MATH_NS::FVector3 n2 = p2.normal;
        const SR_MATH_NS::FVector3 n3 = p3.normal;

        const SR_MATH_NS::FVector3 n2n3 = n2.Cross(n3);
        const SR_MATH_NS::FVector3 n3n1 = n3.Cross(n1);
        const SR_MATH_NS::FVector3 n1n2 = n1.Cross(n2);

        const float_t denom = n1.Dot(n2n3);
        if (fabs(denom) < 1e-6f) {
            return SR_MATH_NS::FVector3(0, 0, 0); // плоскости параллельны, точки нет
        }

        return (n2n3 * (-p1.d) + n3n1 * (-p2.d) + n1n2 * (-p3.d)) / denom;
    }

    bool IsOnOrForwardPlane(const SR_MATH_NS::AABB& box, const FrustumPlane& plane) {
        // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
        const SR_MATH_NS::FVector3 extents = box.GetExtends();

        const float_t r =
            extents.x * std::abs(plane.normal.x) +
            extents.y * std::abs(plane.normal.y) +
            extents.z * std::abs(plane.normal.z);

        return -r <= plane.Distance(box.GetCenter());
    }

    bool Frustum::IsAABBVisible(const SR_MATH_NS::AABB& box) const {
        for (const auto& plane : planes) {
            if (!IsOnOrForwardPlane(box, plane)) {
                return false;
            }
        }
        return true;

        //for (const FrustumPlane& plane : planes) {
        //    SR_MATH_NS::FVector3 positive = box.min;
        //    if (plane.normal.x >= 0) positive.x = box.max.x;
        //    if (plane.normal.y >= 0) positive.y = box.max.y;
        //    if (plane.normal.z >= 0) positive.z = box.max.z;

        //    if (plane.Distance(positive) < 0) {
        //        return false;
        //    }
        //}
        //return true;
    }

    Frustum::FrustumCorners Frustum::GetFrustumCorners() const {
        FrustumCorners corners;

        const auto& near = planes[FRUSTUM_SIDE_NEAR];
        const auto& far = planes[FRUSTUM_SIDE_FAR];
        const auto& left = planes[FRUSTUM_SIDE_LEFT];
        const auto& right = planes[FRUSTUM_SIDE_RIGHT];
        const auto& top = planes[FRUSTUM_SIDE_TOP];
        const auto& bottom = planes[FRUSTUM_SIDE_BOTTOM];

        corners[0] = IntersectFrustumPlanes(near, left, top);    // Near Top Left
        corners[1] = IntersectFrustumPlanes(near, right, top);   // Near Top Right
        corners[2] = IntersectFrustumPlanes(near, right, bottom);// Near Bottom Right
        corners[3] = IntersectFrustumPlanes(near, left, bottom); // Near Bottom Left

        corners[4] = IntersectFrustumPlanes(far, left, top);     // Far Top Left
        corners[5] = IntersectFrustumPlanes(far, right, top);    // Far Top Right
        corners[6] = IntersectFrustumPlanes(far, right, bottom); // Far Bottom Right
        corners[7] = IntersectFrustumPlanes(far, left, bottom);  // Far Bottom Left

        return corners;
    }
}