//
// Created by Monika on 22.11.2025.
//

#ifndef SR_ENGINE_GRAPHICS_IK_COMPONENT_H
#define SR_ENGINE_GRAPHICS_IK_COMPONENT_H

#include <Graphics/stdInclude.h>

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

    /// @displayName(IK) @category(Animations)
    class IKComponent : public SR_UTILS_NS::Component {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<IKComponent>;

    public:
        void Update(float_t dt) override;

        SR_NODISCARD IKType GetIKType() const noexcept { return m_type; }

    private:
        /// @property
        IKType m_type = IKType::TwoBone;

        /// @property
        float_t m_weight = 1.0f;

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

        /// @property @propertyCondition(This.GetIKType() == SR_GRAPH_NS::IKType::TwoBone)
        float_t m_targetPositionWeight = 1.0f;
        /// @property @propertyCondition(This.GetIKType() == SR_GRAPH_NS::IKType::TwoBone)
        float_t m_targetRotationWeight = 1.0f;
        /// @property @propertyCondition(This.GetIKType() == SR_GRAPH_NS::IKType::TwoBone)
        float_t m_hintWeight = 1.0f;

    };
}

#endif //SR_ENGINE_GRAPHICS_IK_COMPONENT_H
