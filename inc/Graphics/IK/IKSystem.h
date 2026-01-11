//
// Created by Monika on 22.11.2025.
//

#ifndef SR_ENGINE_GRAPHICS_IK_SYSTEM_H
#define SR_ENGINE_GRAPHICS_IK_SYSTEM_H

#include <Graphics/IK/IKTwoBoneSolver.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/ECS/GameObject.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(MaintainTargetOffsetMode, uint8_t,
        None,
        Position,
        Rotation,
        PositionAndRotation
    )

    SR_ENUM_NS_CLASS_T(IKType, uint8_t,
        TwoBone,
        CCD,
        FABRIK
    )

    /// @displayName(IK System) @category(Animations)
    class IKSystem : public SR_UTILS_NS::Component {
        using Super = Component;
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<IKSystem>;

    public:
        void OnDisable() override;
        void LateUpdate() override;
        void UpdateIK(float_t dt);

        SR_NODISCARD IKType GetIKType() const noexcept { return m_type; }

        void SetControlledByAnimator(bool isControlled) noexcept { m_isControlledByAnimator = isControlled; }

        IK::IKTwoBoneState& GetTwoBoneState() noexcept { return m_twoBoneState; }

    private:
        IK::IKTwoBoneState m_twoBoneState;
        bool m_isControlledByAnimator = false;

        /// @property
        IKType m_type = IKType::TwoBone;

        /// @property
        float_t m_weight = 1.0f;

        /// @property
        SR_UTILS_NS::EntityRef<SR_UTILS_NS::GameObject> m_rotationReference;
        /// @property
        SR_UTILS_NS::EntityRef<SR_UTILS_NS::GameObject> m_root;
        /// @property @propertyCondition(This.GetIKType() == SR_GRAPH_NS::IKType::TwoBone)
        SR_UTILS_NS::EntityRef<SR_UTILS_NS::GameObject> m_mid;
        /// @property
        SR_UTILS_NS::EntityRef<SR_UTILS_NS::GameObject> m_tip;

        /// @property
        SR_UTILS_NS::EntityRef<SR_UTILS_NS::GameObject> m_hint;
        /// @property
        SR_UTILS_NS::EntityRef<SR_UTILS_NS::GameObject> m_target;

        /// @property
        float_t m_smoothing = 10.f;
        /// @property
        bool m_useInitialRotations = true;
        /// @property
        float m_rootAngleLimit = 0.f;
        /// @property
        float m_midAngleLimit = 0.f;
        /// @property
        float m_maxTwistChangePerFrame = 45.f;
        /// @property
        bool m_preventTwist = true;
        /// @property
        bool m_showDebugGizmos = true;
        /// @property
        bool m_tipRotationFromTarget = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_IK_SYSTEM_H
