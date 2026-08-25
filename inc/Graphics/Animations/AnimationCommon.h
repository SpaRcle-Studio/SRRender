//
// Created by Monika on 23.04.2023.
//

#ifndef SR_ENGINE_ANIMATION_COMMON_H
#define SR_ENGINE_ANIMATION_COMMON_H

#include <Graphics/macros.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/Types/Time.h>
#include <Utils/Types/SharedPtr.h>
#include <Utils/Common/Enumerations.h>
#include <Utils/Math/Vector3.h>
#include <Utils/Serialization/Serializable.h>

#ifdef SR_UTILS_ASSIMP
    #include <assimp/vector3.h>
    #include <assimp/quaternion.h>
#endif

namespace SR_ANIMATIONS_NS {
    /// Это тип свойства которое изменяет AnimationKey
    /*SR_ENUM_NS_CLASS_T(AnimationPropertyType, uint8_t,
        Translation,
        Rotation,
        Scale,
        Skew,

        InstanceFromFile,

        ComponentEnable,
        ComponentProperty,
        ComponentRemove,
        ComponentAdd,

        GameObjectAdd,
        GameObjectRemove,
        GameObjectMove,
        GameObjectEnable,
        GameObjectName,
        GameObjectTag
    );*/

#ifdef SR_UTILS_ASSIMP
    SR_MAYBE_UNUSED static SR_MATH_NS::FVector3 AiV3ToFV3(const aiVector3D& v, float_t multiplier) {
        return SR_MATH_NS::FVector3(v.x, v.y, v.z) * multiplier;
    }

    SR_MAYBE_UNUSED static SR_MATH_NS::Quaternion AiQToQ(const aiQuaternion& q) {
        return SR_MATH_NS::Quaternion(q.x, q.y, q.z, q.w);
    }
#endif

    SR_ENUM_NS_CLASS_T(QuaternionBlendMode, uint8_t,
        Nlerp,
        Slerp
    );

    SR_ENUM_NS_CLASS_T(AnimationStateType, uint8_t ,
        None, Graph, Entry
    );

    SR_ENUM_NS_CLASS_T(AnimationStateConditionOperationType, uint8_t,
        Equals, Less, More, NotEquals
    );

    struct AnimationLink : public SR_UTILS_NS::Serializable {
        SR_CLASS()
    public:
        AnimationLink() = default;

        AnimationLink(uint16_t targetNodeIndex, uint16_t targetPinIndex)
            : m_connected(true)
            , m_targetNodeIndex(targetNodeIndex)
            , m_targetPinIndex(targetPinIndex)
        { }

        SR_NODISCARD bool IsConnected() const { return m_connected; }
        SR_NODISCARD uint16_t GetTargetNodeIndex() const { return m_targetNodeIndex; }
        SR_NODISCARD uint16_t GetTargetPinIndex() const { return m_targetPinIndex; }

        bool operator==(const AnimationLink& other) const noexcept {
            return m_connected == other.m_connected &&
                m_targetNodeIndex == other.m_targetNodeIndex &&
                m_targetPinIndex == other.m_targetPinIndex;
        }

        bool operator!=(const AnimationLink& other) const noexcept {
            return !(*this == other);
        }

        void Connect(uint16_t targetNodeIndex, uint16_t targetPinIndex) {
            m_connected = true;
            m_targetNodeIndex = targetNodeIndex;
            m_targetPinIndex = targetPinIndex;
        }

        void Disconnect() {
            m_connected = false;
            m_targetNodeIndex = 0;
            m_targetPinIndex = 0;
        }

    public:
        SR_UTILS_NS::StringAtom name;

        /// @property
        bool m_connected = false;
        /// @property
        uint16_t m_targetNodeIndex = 0;
        /// @property
        uint16_t m_targetPinIndex = 0;

    };

    class IAnimationDataSet;
    class AnimationGraphNode;
    class AnimationState;
    class AnimationStateMachine;
    class AnimationPose;

    struct StateConditionContext {
        float_t dt = 0.f;
        const AnimationStateMachine* pMachine = nullptr;
        AnimationState* pState = nullptr;
    };

    /// @noCopyable @noMovable
    class IAnimationDataSet : public SR_HTYPES_NS::SharedPtr<IAnimationDataSet>, public SR_UTILS_NS::Serializable, public SR_UTILS_NS::NonCopyable {
        SR_CLASS()
    public:
        IAnimationDataSet();
        ~IAnimationDataSet() override = default;

    public:
        void SetBool(const SR_UTILS_NS::StringAtom& name, const bool value) {
            m_boolTable[name] = value;
        }
        void SetInt(const SR_UTILS_NS::StringAtom& name, const int32_t value) {
            m_intTable[name] = value;
        }
        void SetFloat(const SR_UTILS_NS::StringAtom& name, const float_t value) {
            m_floatTable[name] = value;
        }
        void SetString(const SR_UTILS_NS::StringAtom& name, SR_UTILS_NS::StringView value) {
            m_stringTable[name] = value;
        }

        SR_NODISCARD std::optional<bool> GetBool(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD std::optional<int32_t> GetInt(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD std::optional<float_t> GetFloat(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD std::optional<std::string> GetString(const SR_UTILS_NS::StringAtom& name) const;

        void SetAnimationDataSetParent(IAnimationDataSet* pParent) {
            m_parent = pParent;
        }

    protected:
        /// @property
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, bool> m_boolTable;
        /// @property
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, int32_t> m_intTable;
        /// @property
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, float_t> m_floatTable;
        /// @property
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SR_UTILS_NS::String> m_stringTable;

        IAnimationDataSet* m_parent = nullptr;

    };
}

#endif //SR_ENGINE_ANIMATION_COMMON_H
