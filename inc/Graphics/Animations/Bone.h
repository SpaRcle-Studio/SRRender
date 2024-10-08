//
// Created by Monika on 19.08.2021.
//

#ifndef SR_ENGINE_BONE_H
#define SR_ENGINE_BONE_H

#include <Utils/ECS/Component.h>
#include <Utils/ECS/GameObject.h>

#include <Graphics/Types/Mesh.h>

namespace SR_ANIMATIONS_NS {
    class BoneComponent;

    struct Bone : public SR_UTILS_NS::NonCopyable {
    public:
        ~Bone() override {
            for (auto&& pBone : bones) {
                delete pBone;
            }
        }

        SR_NODISCARD Bone* CloneRoot() const noexcept {
            Bone* pRootBone = new Bone();

            pRootBone->name = name;
            pRootBone->gameObject = gameObject;
            pRootBone->pRoot = pRootBone;
            pRootBone->pParent = nullptr;
            pRootBone->pScene = pScene;

            pRootBone->bones.reserve(bones.size());

            for (auto&& pSubBone : bones) {
                pRootBone->bones.emplace_back(pSubBone->Clone(pRootBone));
            }

            return pRootBone;
        }

        bool Initialize();

    private:
        SR_NODISCARD Bone* Clone(Bone* pParentBone) const noexcept {
            Bone* pBone = new Bone();

            pBone->name = name;
            pBone->gameObject = gameObject;
            pBone->pParent = pParentBone;
            pBone->pRoot = pParentBone->pRoot;
            pBone->pScene = pParentBone->pScene;

            pBone->bones.reserve(bones.size());

            for (auto&& pSubBone : bones) {
                pBone->bones.emplace_back(pSubBone->Clone(pBone));
            }

            return pBone;
        }

    public:
        SR_UTILS_NS::StringAtom name;
        SR_WORLD_NS::Scene* pScene = nullptr;
        SR_HTYPES_NS::SharedPtr<SR_UTILS_NS::GameObject> gameObject;
        std::vector<Bone*> bones;
        Bone* pParent = nullptr;
        Bone* pRoot = nullptr;
        bool hasError = false;

    };
}

#endif //SR_ENGINE_BONE_H
