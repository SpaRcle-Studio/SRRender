//
// Created by Igor on 08/12/2022.
//

#ifndef SR_ENGINE_SKELETON_H
#define SR_ENGINE_SKELETON_H

#include <Graphics/Memory/SSBO.h>

#include <Utils/ECS/Component.h>
#include <Utils/Types/IRawMeshHolder.h>

namespace SR_GRAPH_NS {
    class RenderScene;
    class SSBOInstance;
}

namespace SR_ANIMATIONS_NS {
    class Bone;

    /// @category(Animations)
    class Skeleton : public SR_UTILS_NS::Component, public SR_HTYPES_NS::IRawMeshHolder {
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

        void OnAttached() override;
        void OnDestroy() override;

        bool ReCalculateSkeleton();

        Bone* AddBone(Bone* pParent, SR_UTILS_NS::StringAtom name, bool recalculate);
        SR_NODISCARD const Bone* GetRootBone() const noexcept { return m_rootBone.Get(); }
        SR_NODISCARD Bone* GetRootBone() noexcept { return m_rootBone.Get(); }
        SR_NODISCARD int32_t GetOffsetsSSBO() const noexcept;
        SR_NODISCARD int32_t GetBonesSSBO() const noexcept;

        const SR_MATH_NS::Matrix4x4& GetMatrixByIndex(uint16_t index) noexcept;
        SR_NODISCARD const std::vector<SR_MATH_NS::Matrix4x4>& GetMatrices() noexcept;
        SR_NODISCARD const std::vector<SR_MATH_NS::Matrix4x4>& GetOffsets() const noexcept;
        SR_NODISCARD const std::vector<Bone*>& GetBones() const noexcept { return m_bonesByIndex; };
        SR_NODISCARD Bone* TryGetBone(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD Bone* GetBone(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD Bone* GetBoneByIndex(uint16_t index) const;
        SR_NODISCARD uint64_t GetBoneIndex(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD bool IsDebugEnabled() const noexcept { return m_debugEnabled; }
        SR_NODISCARD bool IsDirtyMatrices() const noexcept { return m_dirtyMatrices; }
        SR_NODISCARD const ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t>& GetOptimizedBones() const noexcept;
        void SetDebugEnabled(bool enabled) { m_debugEnabled = enabled; }

        SR_NODISCARD const SR_HTYPES_NS::SafePtr<RenderContext>& GetRenderContext() const noexcept;
        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<Pipeline>& GetPipeline() const noexcept;

        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        void OnRawMeshChanged() override;

    private:
        void UpdateDebug();
        void DisableDebug();
        void CalculateTransforms();

    private:
        ska::flat_hash_map<Bone*, uint64_t> m_debugLines;
        ska::flat_hash_map<SR_UTILS_NS::StringAtom, Bone*> m_bonesByName;

        std::vector<Bone*> m_bonesByIndex;

        bool m_isNeedRecalcTransforms = true;
        bool m_multiFrameSSBOResources = true;
        mutable bool m_isBonesSSBODirty = true;
        mutable bool m_isOffsetsSSBODirty = true;

        std::vector<uint32_t> m_indices;
        std::vector<SR_HTYPES_NS::SharedPtr<SR_UTILS_NS::Transform3D>> m_transforms;
        std::vector<SR_MATH_NS::Matrix4x4> m_matrices;

        mutable std::unique_ptr<SSBOInstance> m_offsetsSSBO;
        mutable std::array<std::unique_ptr<SSBOInstance>, SR_MAX_FRAMES_IN_FLIGHT> m_bonesSSBO;
        mutable SR_HTYPES_NS::SharedPtr<Pipeline> m_pipeline;
        mutable SR_HTYPES_NS::SafePtr<RenderContext> m_renderContext;

    private:
        /// @property @notNull
        SR_HTYPES_NS::SharedPtr<Bone> m_rootBone;
        /// @property @dontSave @setter(SetDebugEnabled)
        bool m_debugEnabled = false;
        /// @property @dontSave @readOnly
        bool m_dirtyMatrices = false;
        /// @property @dontSave @readOnly
        bool m_hasInvalidBones = false;
        /// @virtualProperty(meshPath) @getter(GetMeshPath) @setter(SetRawMesh)
        /// @customArgs(pick: enabled, filter name: Meshes, relative: resources)
        /// @customArg(filter value: fbx,blend,obj,pmx,stl,dae)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshId) @getter(GetMeshId) @setter(SetMeshId)
        SR_VIRTUAL_PROPERTY

    };
}

#endif //SR_ENGINE_SKELETON_H
