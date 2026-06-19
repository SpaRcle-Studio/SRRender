//
// Created by Monika on 19.08.2021.
//

#include <Graphics/Animations/Bone.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/BoneComponent.h>

#include <Utils/World/Scene.h>
#include <Utils/ECS/GameObject.h>

#include <Codegen/Bone.generated.hpp>

namespace SR_ANIMATIONS_NS {
    void Bone::InitTree(Bone* pParent) {
        this->pRoot = pParent ? pParent->pRoot : this;
        this->pScene = pParent ? pParent->pScene : pScene;
        this->pParent = pParent;

        for (auto&& pChild : bones) {
            pChild->InitTree(this);
        }
    }

    void Bone::SetGameObject(const GameObjectPtr& pGameObject) noexcept {
        m_gameObject = pGameObject;
    }

    bool Bone::Initialize() {
        SR_TRACY_ZONE;

        if (!pRoot) {
            SRHalt0();
            m_hasError = true;
            return false;
        }

        if (!pRoot->m_gameObject && !pRoot->pScene) {
            SRHalt0();
            m_hasError = true;
            return false;
        }

        std::vector<SR_UTILS_NS::StringAtom> names = { name };

        Bone* pParentBone = pParent;
        /// рутовую ноду в расчет не берем
        while (pParentBone && pParentBone->pParent) {
            names.emplace_back(pParentBone->name);
            pParentBone = pParentBone->pParent;
        }

        if (pRoot->m_gameObject) {
            m_gameObject = pRoot->m_gameObject;
        }

        for (int32_t i = static_cast<int32_t>(names.size()) - 1; i >= 0; --i) {
            if (m_gameObject) {
                if (!((m_gameObject = SR_UTILS_NS::DynamicPointerCast<SR_UTILS_NS::GameObject>(m_gameObject->Find(names[i]))))) {
                    break;
                }
            }
            else {
                if (!((m_gameObject = SR_UTILS_NS::DynamicPointerCast<SR_UTILS_NS::GameObject>(pRoot->pScene->GetScene()->Find(names[i]))))) {
                    break;
                }
            }
        }

        if ((m_hasError = !m_gameObject.Valid())) {
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

    const Bone::GameObjectPtr& Bone::GetGameObject() const noexcept {
        if (!m_gameObject && !m_hasError) {
            if (!const_cast<Bone&>(*this).Initialize()) {
                SR_WARN("Bone::GetGameObject() : failed to initialize bone! Name: {}", name);
            }
        }

        return m_gameObject;
    }
}
