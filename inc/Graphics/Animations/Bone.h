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
        using GameObjectPtr = SR_HTYPES_NS::SharedPtr<SR_UTILS_NS::GameObject>;
    public:
        Bone()
            : SR_HTYPES_NS::SharedPtr<Bone>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        bool Initialize();
        void InitTreeIfNeed();

        void SetGameObject(const GameObjectPtr& pGameObject) noexcept;
        SR_NODISCARD const GameObjectPtr& GetGameObject() const noexcept;

    private:
        void InitTree(Bone* pParent);

    public:
        /// @property
        SR_UTILS_NS::StringAtom name;
        /// @property
        SR_UTILS_NS::Vector<Bone::Ptr> bones;

        Bone* pParent = nullptr;
        Bone* pRoot = nullptr;

    private:
        GameObjectPtr m_gameObject;
        bool m_hasError = false;

    };
}

#endif //SR_ENGINE_BONE_H
