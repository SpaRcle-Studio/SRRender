//
// Created by Monika on 19.08.2021.
//

#include <Utils/World/Scene.h>

#include <Graphics/Animations/Bone.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/BoneComponent.h>

#include <Codegen/Bone.generated.hpp>

namespace SR_ANIMATIONS_NS {
    void Bone::InitTree(Bone* pParent) {
        this->pRoot = pParent ? pParent->pRoot : this;
        this->pSkeleton = pParent ? pParent->pSkeleton : pSkeleton;
        this->pParent = pParent;

        for (auto&& pChild : bones) {
            pChild->InitTree(this);
        }
    }

    bool Bone::Initialize() {
        SR_TRACY_ZONE;

        if (!pRoot) {
            SRHalt0();
            hasError = true;
            return false;
        }

        if (!pRoot->gameObject && !pRoot->pSkeleton) {
            SRHalt0();
            hasError = true;
            return false;
        }

        std::vector<SR_UTILS_NS::StringAtom> names = { name };

        Bone* pParentBone = pParent;
        /// рутовую ноду в расчет не берем
        while (pParentBone && pParentBone->pParent) {
            names.emplace_back(pParentBone->name);
            pParentBone = pParentBone->pParent;
        }

        if (pRoot->gameObject) {
            gameObject = pRoot->gameObject;
        }

        for (int32_t i = static_cast<int32_t>(names.size()) - 1; i >= 0; --i) {
            if (gameObject) {
                if (!((gameObject = gameObject->Find(names[i]).DynamicCast<SR_UTILS_NS::GameObject>()))) {
                    break;
                }
            }
            else {
                if (!((gameObject = pRoot->pSkeleton->GetScene()->Find(names[i]).DynamicCast<SR_UTILS_NS::GameObject>()))) {
                    break;
                }
            }
        }

        if ((hasError = !gameObject.Valid())) {
            return false;
        }

        SR_NOOP;

        return true;
    }

    void Bone::InitTreeIfNeed() {
        if (!pRoot) {
            InitTree(nullptr);
        }
    }
}
