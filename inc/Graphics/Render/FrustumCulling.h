//
// Created by Monika on 07.04.2024.
//

#ifndef SR_ENGINE_GRAPHICS_FRUSTUM_CULLING_H
#define SR_ENGINE_GRAPHICS_FRUSTUM_CULLING_H

#include <Utils/Common/NonCopyable.h>
#include <Utils/Math/Matrix4x4.h>

namespace SR_GRAPH_NS {
    struct FrustumPlane {
        SR_MATH_NS::FVector3 normal = { 0.f, 1.f, 0.f };
        float_t distance = 0.f;
    };

    struct Frustum {
        FrustumPlane topFace;
        FrustumPlane bottomFace;

        FrustumPlane rightFace;
        FrustumPlane leftFace;

        FrustumPlane farFace;
        FrustumPlane nearFace;
    };

    class FrustumCulling : public SR_UTILS_NS::NonCopyable {
    public:
        FrustumCulling() = default;

        //void UpdateFrustum(const SR_GTYPES_NS::Camera& camera) noexcept;
        //SR_NODISCARD bool IsSphereInFrustum(const SR_GTYPES_NS::Vector3& center, float_t radius) const noexcept;
        //SR_NODISCARD bool IsBoxInFrustum(const SR_GTYPES_NS::Vector3& min, const SR_GTYPES_NS::Vector3& max) const noexcept;

    private:
        SR_MATH_NS::FVector4 m_planes[6];

    };

}

#endif //SR_ENGINE_GRAPHICS_FRUSTUM_CULLING_H
