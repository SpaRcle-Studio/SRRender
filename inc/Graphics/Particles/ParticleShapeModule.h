//
// Created by Nariman on 04.06.2026.
//

#ifndef SRENGINE_PARTICLESHAPEMODULE_H
#define SRENGINE_PARTICLESHAPEMODULE_H

#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

#include <Utils/ECS/Component.h>
#include <Utils/FileSystem/Path.h>
#include <Utils/Platform/PlatformType.h>


namespace SR_GRAPH_NS {
	/// @abstract
    class ParticleShape : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<ParticleShape> {
        SR_CLASS()

    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<ParticleShape>;

        ParticleShape() 
            : SR_HTYPES_NS::SharedPtr<ParticleShape>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic) 
        { }

        virtual ~ParticleShape() = default;

        virtual SR_MATH_NS::FVector3 GeneratePosition() const { return SR_MATH_NS::FVector3(); }
        virtual SR_MATH_NS::FVector3 GenerateDirection() const { return SR_MATH_NS::FVector3(); }
    };

    class PointShape : public ParticleShape {
        SR_CLASS()

    public:
        SR_MATH_NS::FVector3 GeneratePosition() const override;
        SR_MATH_NS::FVector3 GenerateDirection() const override;
    };


    class SphereShape : public ParticleShape {
        SR_CLASS()

    public:
        SR_MATH_NS::FVector3 GeneratePosition() const override;
        SR_MATH_NS::FVector3 GenerateDirection() const override;

    private:
        /// @property
        float m_radius = 5.0f;
        mutable SR_MATH_NS::FVector3 m_point = SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0);
    };


    class BoxShape : public ParticleShape {
    SR_CLASS()

    public:
        SR_MATH_NS::FVector3 GeneratePosition() const override;
        SR_MATH_NS::FVector3 GenerateDirection() const override;

    private:
        /// @property
        float m_width = 80.0f;
        float m_depth = 40.0f;
        float m_height = 2.0f;
    };

    class ConusShape : public ParticleShape {
    SR_CLASS()

    public:
        SR_MATH_NS::FVector3 GeneratePosition() const override;
        SR_MATH_NS::FVector3 GenerateDirection() const override;

    private:
        /// @property
        float m_radius = 0.1f;
        float m_height = 5.0f;
        mutable SR_MATH_NS::FVector3 m_point = SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0);
    };
}
#endif //SRENGINE_PARTICLESHAPEMODULE_H
