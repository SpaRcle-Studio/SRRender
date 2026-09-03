//
// Created by Monika on 08.01.2023.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATION_CLIP_H
#define SR_ENGINE_GRAPHICS_ANIMATION_CLIP_H

#include <Graphics/Animations/Retarget/RetargetProfile.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Resources/Asset.h>
#include <Utils/Resources/ResourceRef.h>
#include <Utils/Types/IRawMeshHolder.h>

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_ANIMATIONS_NS {
    class AnimationChannel;

    struct ExcludeBoneTransforms : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
        /// @property
        bool translation = true;
        /// @property
        bool rotation = true;
        /// @property
        bool scale = true;

    };

    struct LockTransformAxis : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
        /// @property
        SR_MATH_NS::Axis translation;
        /// @property
        SR_MATH_NS::Axis rotation;
        /// @property
        SR_MATH_NS::Axis scale;

    };

    /// @extension(animation)
    class AnimationClip : public SR_UTILS_NS::Asset {
        SR_CLASS()
        using Super = SR_UTILS_NS::Asset;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationClip>;
        using Channels = SR_UTILS_NS::Vector<AnimationChannel>;

    public:
        AnimationClip();
        ~AnimationClip() override;

    public:
        SR_NODISCARD const Channels& GetChannels(const SkeletonRig* pTargetRig) const;
        SR_NODISCARD bool IsAllowedToRevive() const override { return true; }

        SR_NODISCARD SR_UTILS_NS::StringAtom GetClipName() const noexcept;

        SR_NODISCARD float_t GetDuration() const noexcept { return m_duration; }
        SR_NODISCARD uint32_t GetMaxKeyFrame() const noexcept { return m_maxKeyFrame; }

    protected:
        bool Unload() override;
        void OnAssetLoaded() override;
        void PostProcess();

    private:
        SR_NODISCARD bool LoadChannels(SR_HTYPES_NS::RawMesh* pRawMesh, SR_UTILS_NS::StringAtom name);

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_clipName;
        /// @property
        /// @customArgs(pick: enabled, filter name: Animation, relative: resources)
        /// @customArg(filter value: fbx)
        SR_UTILS_NS::Path m_clipPath;
        /// @property
        SR_UTILS_NS::ResourceRef<SkeletonRig> m_rig;
        /// @property @condition(!This.m_rig.IsValid())
        SR_HTYPES_NS::RawMeshHolder m_skeleton;
        /// @property
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, ExcludeBoneTransforms> m_excludedBones;
        /// @property
        SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, LockTransformAxis> m_lockedBones;
        /// @property
        SR_UTILS_NS::Vector<RetargetProfileEmbedded> m_retargetProfiles;
        /// @property
        SR_UTILS_NS::Optional<float_t> m_startTime;

        Channels m_channels;
        mutable SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringAtom, Channels> m_retargetCache;

        float_t m_duration = 0.f;
        uint32_t m_maxKeyFrame = 0;
        SR_UTILS_NS::Vector<SR_UTILS_NS::Subscription> m_rigReloadSubscriptions;

    };
}

#endif //SR_ENGINE_GRAPHICS_ANIMATION_CLIP_H
