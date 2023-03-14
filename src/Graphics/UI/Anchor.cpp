//
// Created by Monika on 01.08.2022.
//

#include <Graphics/UI/Anchor.h>
#include <Graphics/UI/Canvas.h>

namespace SR_GRAPH_NS::UI {
    SR_REGISTER_COMPONENT(Anchor);

    Anchor::Anchor()
        : SR_UTILS_NS::Component()
    { }

    void Anchor::TransformUI() {
        /*GetParent()->Do([this](SR_UTILS_NS::GameObject* pThis) {
            if (auto&& pParent = pThis->GetParent(); pParent.RecursiveLockIfValid())
            {
                auto&& pTransform = pThis->GetTransform();
                auto&& pParentTransform = pParent->GetTransform();

                auto&& translate = (pParentTransform->GetTranslation().x + (pParentTransform->GetScale().x)) - (GetSizes().x / 2.f);

                pTransform->SetTranslation(translate, 0, 0);

                pParent.Unlock();
            }
            else {
                SRHalt("Parent is invalid!");
            }
        });*/
    }

    SR_UTILS_NS::Component* Anchor::LoadComponent(SR_HTYPES_NS::Marshal &marshal, const SR_HTYPES_NS::DataStorage *dataStorage) {
        return new Anchor();
    }

    void Anchor::OnDestroy() {
        Super::OnDestroy();
        delete this;
    }

    SR_MATH_NS::FVector2 Anchor::GetSizes() const {
        SR_MATH_NS::FVector2 size;

       //if (!m_parent->RecursiveLockIfValid()) {
       //    return size;
       //}

        //for (auto&& pComponent : m_parent->GetComponentsRef()) {
        //    if (auto&& pSprite = dynamic_cast<Sprite2D*>(pComponent)) {
        //        size = pSprite->GetSizes() * GetTransform()->GetScale2D();
        //    }
        //}

       // m_parent->Unlock();

        return size;
    }
}