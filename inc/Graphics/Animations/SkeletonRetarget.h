//
// Created by Monika on 14.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_SKELETON_RETARGET_H
#define SR_ENGINE_GRAPHICS_SKELETON_RETARGET_H

#include <Graphics/Animations/SkeletonType.h>
#include <Graphics/Animations/HumanoidBoneType.h>

#include <Utils/Types/SharedPtr.h>
#include <Utils/Resources/Asset.h>

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_ANIMATIONS_NS {
    struct BoneChain : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom> bones;
    };

    /// @extension(sretarget)
    class SkeletonRetargetProfile : public SR_UTILS_NS::Asset {
        SR_CLASS()
    private:
        /// @method @editorButton @condition(This.m_skeletonType == SkeletonType::Humanoid)
        void AutoRemapHumanoidBones();

        void AutoRemapHumanoidBonesImpl(const SR_HTYPES_NS::RawMesh& rawMesh, uint32_t index);

    private:
        /// @property
        SkeletonType m_skeletonType = SkeletonType::Generic;

        /// @property
        std::unordered_map<HumanoidBoneType, BoneChain> m_mapping;

    };
}

#endif //SR_ENGINE_GRAPHICS_SKELETON_RETARGET_H
