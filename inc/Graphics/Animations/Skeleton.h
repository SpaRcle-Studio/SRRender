//
// Created by Igor on 08/12/2022.
//

#ifndef SR_ENGINE_SKELETON_H
#define SR_ENGINE_SKELETON_H

#include <Graphics/Memory/SSBO.h>
#include <Graphics/Animations/Bone.h>
#include <Graphics/Animations/SkeletonRig.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/Types/IRawMeshHolder.h>
#include <Utils/Types/FlatHashMap.h>
#include <Utils/Common/Subscription.h>
#include <Utils/Resources/ResourceRef.h>

namespace SR_GRAPH_NS {
    class RenderScene;
    class SSBOInstance;
}

namespace SR_ANIMATIONS_NS {
    /// @category(Animations)
    class Skeleton : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Skeleton>;
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;

    public:
        ~Skeleton() override;

    public:
        void LateUpdate() override;

        void OnPostLoad() override;

        void OnEnable() override;
        void OnDisable() override;

        void OnAttached() override;
        void OnDestroy() override;

        bool ReCalculateSkeleton();

        Bone* AddBone(Bone* pParent, SR_UTILS_NS::StringAtom name, bool recalculate);
        SR_NODISCARD int32_t GetOffsetsSSBO() const noexcept;
        SR_NODISCARD int32_t GetBonesSSBO() const noexcept;

        SR_NODISCARD const SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4>& GetMatrices() noexcept;
        SR_NODISCARD const SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4>& GetOffsets() const noexcept;
        SR_NODISCARD Bone* GetBone(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD Bone* GetAnimationBone(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD bool IsDebugEnabled() const noexcept { return m_debugEnabled; }
        SR_NODISCARD bool IsDirtyMatrices() const noexcept { return m_dirtyMatrices; }
        void SetDebugEnabled(bool enabled) { m_debugEnabled = enabled; }

        void ForEachBone(const SR_HTYPES_NS::Function<void(Bone&)>& callback);

        SR_NODISCARD const SR_HTYPES_NS::SafePtr<RenderContext>& GetRenderContext() const noexcept;
        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<Pipeline>& GetPipeline() const noexcept;

        void SetRawMesh(const SR_HTYPES_NS::RawMeshHolder& rawMesh) { m_skeleton = rawMesh; OnRawMeshChanged(); }
        const SR_HTYPES_NS::RawMeshHolder& GetSkeletonRawMesh() const;

        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        SR_NODISCARD const SkeletonRig* GetRig() const noexcept;

    private:
        void UpdateBonesSSBO();
        void UpdateDebug();
        void DisableDebug();
        void CalculateTransforms();
        bool TryInitializeBonesFromMesh();
        void OnRawMeshChanged();

    private:
        /// @method @editorButton
        void SwitchDebug();

        SR_NODISCARD SR_HTYPES_NS::SharedPtr<Bone>& GetRootBone() noexcept;

    private:
        SR_HTYPES_NS::FlatHashMap<Bone*, uint64_t> m_debugLines;
        SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringAtom, Bone*> m_bonesByName;

        SR_UTILS_NS::Subscription m_prepareFrameSubscription;

        bool m_debugEnabled = false;
        bool m_isNeedRecalcTransforms = true;
        bool m_multiFrameSSBOResources = true;
        mutable bool m_isBonesSSBODirty = true;
        mutable bool m_isOffsetsSSBODirty = true;

        SR_UTILS_NS::Vector<uint32_t> m_indices;
        SR_UTILS_NS::Vector<SR_HTYPES_NS::SharedPtr<SR_UTILS_NS::Transform3D>> m_transforms;
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> m_matrices;
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> m_bindPoseMatrices;

        mutable std::unique_ptr<SSBOInstance> m_offsetsSSBO;
        mutable std::array<std::unique_ptr<SSBOInstance>, SR_MAX_FRAMES_IN_FLIGHT> m_bonesSSBO;
        mutable SR_HTYPES_NS::SharedPtr<Pipeline> m_pipeline;
        mutable SR_HTYPES_NS::SafePtr<RenderContext> m_renderContext;

    private:
        /// @property
        SR_UTILS_NS::EntityRef<Skeleton> m_parent;
        /// @property @onChanged(OnRawMeshChanged)
        SR_UTILS_NS::ResourceRef<SkeletonRig> m_rig;
        /// @property @condition(!This.m_rig.IsValid())
        /// @onChanged(OnRawMeshChanged)
        SR_HTYPES_NS::RawMeshHolder m_skeleton;

        /// @property @dontSave @readOnly @debugOnly
        bool m_dirtyMatrices = false;
        /// @property @dontSave @readOnly @debugOnly
        bool m_hasInvalidBones = false;

        /// @property @notNull @debugOnly @dontSave @readOnly
        SR_HTYPES_NS::SharedPtr<Bone> m_rootBone;

    };
}

#endif //SR_ENGINE_SKELETON_H
