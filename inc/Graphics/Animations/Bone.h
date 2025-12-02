//
// Created by Monika on 19.08.2021.
//

#ifndef SR_ENGINE_BONE_H
#define SR_ENGINE_BONE_H

#include <Graphics/Types/Mesh.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/GameObject.h>

namespace SR_ANIMATIONS_NS {
    class Skeleton;
    class BoneComponent;

    class Bone final : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<Bone> {
        SR_CLASS()
    public:
        Bone()
            : SR_HTYPES_NS::SharedPtr<Bone>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        void SetSkeleton(Skeleton* pSkeleton) {
            this->pSkeleton = pSkeleton;
        }

        bool Initialize();
        void InitTreeIfNeed();

    private:
        void InitTree(Bone* pParent);

    public:
        /// @property
        SR_UTILS_NS::StringAtom name;
        /// @property
        std::vector<Bone::Ptr> bones;

        SR_HTYPES_NS::SharedPtr<SR_UTILS_NS::GameObject> gameObject;
        Bone* pParent = nullptr;
        Bone* pRoot = nullptr;
        bool hasError = false;
        Skeleton* pSkeleton = nullptr;

    };
}

#endif //SR_ENGINE_BONE_H
