//
// Created by Nariman on 04.06.2026.
//

#include <Graphics/Particles/ParticleShapeModule.h>

#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

#include <Utils/Common/Numeric.h>

#include <Codegen/ParticleShapeModule.generated.hpp>

namespace SR_GRAPH_NS{
    SR_MATH_NS::FVector3 PointShape::GenerateDirection() const {
                return SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f);
    }

    SR_MATH_NS::FVector3 PointShape::GeneratePosition() const {
        return SR_MATH_NS::FVector3(0.0f);
    }

    SR_MATH_NS::FVector3 SphereShape::GeneratePosition() const {
        while(true) {
            SR_MATH_NS::FVector3 temppoint(SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                                       SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                                       SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0));

            m_point = temppoint;
            if(m_point.Length() <= 1.0f) {
                return m_point * m_radius;
            }
        }
    }

    SR_MATH_NS::FVector3 SphereShape::GenerateDirection() const {
        return m_point.Normalize();
    }
}