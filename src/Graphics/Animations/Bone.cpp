//
// Created by Monika on 19.08.2021.
//

#include <Graphics/Animations/Bone.h>
#include <Graphics/Animations/BoneComponent.h>

namespace SR_ANIMATIONS_NS {
    bool Bone::Initialize() {
        SR_TRACY_ZONE;

        if (!pRoot->gameObject && !pRoot->pScene) {
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
                if (!((gameObject = gameObject->Find(names[i])))) {
                    break;
                }
            }
            else {
                if (!((gameObject = pRoot->pScene->Find(names[i])))) {
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
}
