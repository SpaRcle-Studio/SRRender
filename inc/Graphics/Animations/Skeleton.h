//
// Created by Igor on 08/12/2022.
//

#ifndef SR_ENGINE_SKELETON_H
#define SR_ENGINE_SKELETON_H

#include <Graphics/Animations/Bone.h>

namespace SR_ANIMATIONS_NS {
    class Skeleton : public SR_UTILS_NS::Component {
        SR_ENTITY_SET_VERSION(1001);
        SR_INITIALIZE_COMPONENT(Skeleton);
        using Super = SR_UTILS_NS::Component;
    public:
        using RenderScenePtr = SR_HTYPES_NS::SafePtr<RenderScene>;
    public:
        static Component* LoadComponent(SR_HTYPES_NS::Marshal& marshal, const SR_HTYPES_NS::DataStorage* dataStorage);

        ~Skeleton() override;

    public:
        SR_NODISCARD Component* CopyComponent() const override;
        SR_NODISCARD SR_HTYPES_NS::Marshal::Ptr SaveLegacy(SR_UTILS_NS::SavableContext data) const override;

        void Update(float_t dt) override;

        void OnAttached() override;
        void OnLoaded() override;
        void OnDestroy() override;

        bool ReCalculateSkeleton();
        void CalculateMatrices();

        void ResetSkeleton();

        void SetOptimizedBones(const ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t>& bones);
        void SetBonesOffsets(const std::vector<SR_MATH_NS::Matrix4x4>& offsets);

        Bone* AddBone(Bone* pParent, const std::string& name, bool recalculate);
        SR_NODISCARD Bone* GetRootBone() const noexcept { return m_rootBone; }

        const SR_MATH_NS::Matrix4x4& GetMatrixByIndex(uint16_t index) noexcept;
        SR_NODISCARD const std::vector<SR_MATH_NS::Matrix4x4>& GetMatrices() noexcept;
        SR_NODISCARD const std::vector<SR_MATH_NS::Matrix4x4>& GetOffsets() noexcept { return m_skeletonOffsets; }
        SR_UTILS_NS::Transform* GetTransformByIndex(uint16_t index) noexcept;
        SR_NODISCARD const std::vector<Bone*>& GetBones() const noexcept { return m_bonesByIndex; };
        SR_NODISCARD Bone* TryGetBone(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD Bone* GetBone(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD Bone* GetBoneByIndex(uint16_t index) const;
        SR_NODISCARD uint64_t GetBoneIndex(SR_UTILS_NS::StringAtom name);
        SR_NODISCARD bool IsDebugEnabled() const noexcept { return m_debugEnabled; }
        SR_NODISCARD bool IsDirtyMatrices() const noexcept { return m_dirtyMatrices; }
        SR_NODISCARD const ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t>& GetOptimizedBones() const noexcept { return m_optimizedBones; }
        void SetDebugEnabled(bool enabled) { m_debugEnabled = enabled; }

        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

    private:
        void UpdateDebug();
        void DisableDebug();

    private:
        bool m_debugEnabled = false;

        ska::flat_hash_map<Bone*, uint64_t> m_debugLines;
        ska::flat_hash_map<SR_UTILS_NS::StringAtom, Bone*> m_bonesByName;

        std::vector<Bone*> m_bonesByIndex;

        ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t> m_optimizedBones;

        std::vector<SR_MATH_NS::Matrix4x4> m_matrices;
        std::vector<SR_MATH_NS::Matrix4x4> m_skeletonOffsets;

        bool m_dirtyMatrices = false;

        Bone* m_rootBone = nullptr;

    };
}

#endif //SR_ENGINE_SKELETON_H
