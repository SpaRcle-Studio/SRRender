//
// Created by Monika on 14.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_SKELETON_RIG_H
#define SR_ENGINE_GRAPHICS_SKELETON_RIG_H

#include <Graphics/Animations/SkeletonType.h>
#include <Graphics/Animations/HumanoidBoneType.h>

#include <Utils/Types/SharedPtr.h>
#include <Utils/Types/IRawMeshHolder.h>
#include <Utils/Resources/Asset.h>
#include <Utils/Resources/ResourceRef.h>

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_ANIMATIONS_NS {
    struct SkeletonRigBoneInfo : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::StringAtom name;
        /// @property
        uint32_t index = SR_ID_INVALID;
        /// @property
        SR_MATH_NS::FVector3 bindTranslation;
        /// @property
        SR_MATH_NS::Quaternion bindRotation;
        /// @property
        SR_MATH_NS::FVector3 bindScale;
    };

    struct SkeletonRigBoneChain : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::Vector<SkeletonRigBoneInfo> bones;
    };

    struct SkeletonAutoRigRules : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom> ignoreBonesStartsWith;
        /// @property
        SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom> ignoreBonesContains;
        /// @property
        SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom> ignoreBonesEndsWith;

        SR_NODISCARD bool IsBoneIgnored(SR_UTILS_NS::StringAtom name) const;
    };

    /// @extension(rig)
    class SkeletonRig : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        SR_NODISCARD const SkeletonRigBoneChain* RetargetBone(SR_UTILS_NS::StringAtom name, SR_UTILS_NS::StringAtom& outName) const;

        SR_NODISCARD uint32_t GetBoneIndex(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD const SkeletonRigBoneChain* GetBoneChain(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetBoneName(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD const SR_HTYPES_NS::RawMeshHolder& GetSkeleton() const { return m_skeleton; }

    private:
        /// @method @editorButton @condition(This.m_skeletonType == SkeletonType::Humanoid)
        void AutoRemapHumanoidBones();

        void AutoRemapHumanoidBonesImpl(const SR_HTYPES_NS::RawMesh& rawMesh, uint32_t index);

    private:
        /// @property
        SR_HTYPES_NS::RawMeshHolder m_skeleton;

        /// @property
        SkeletonType m_skeletonType = SkeletonType::Generic;

        /// @property
        std::unordered_map<SR_UTILS_NS::StringAtom, SkeletonRigBoneChain> m_mapping;

        /// @property @condition(This.m_skeletonType == SkeletonType::Humanoid)
        SkeletonAutoRigRules m_autoRigRules;


    };
}

#endif //SR_ENGINE_GRAPHICS_SKELETON_RIG_H
