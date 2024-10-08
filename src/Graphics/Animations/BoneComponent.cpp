//
// Created by Monika on 28.07.2023.
//

#include <Graphics/Animations/BoneComponent.h>
#include <Graphics/Animations/Skeleton.h>
#include <Utils/ECS/ComponentManager.h>

namespace SR_ANIMATIONS_NS {
    SR_REGISTER_COMPONENT(BoneComponent);

    BoneComponent::BoneComponent()
        : Super()
        , m_skeleton(GetThis())
    { }

    SR_UTILS_NS::Component* BoneComponent::LoadComponent(SR_HTYPES_NS::Marshal& marshal, const SR_HTYPES_NS::DataStorage* pDataStorage) {
        return new BoneComponent();
    }

    SR_UTILS_NS::Component* BoneComponent::CopyComponent() const {
        return new BoneComponent();
    }

    SR_HTYPES_NS::Marshal::Ptr BoneComponent::SaveLegacy(SR_UTILS_NS::SavableContext data) const {
        return Super::SaveLegacy(data);
    }

    void BoneComponent::OnMatrixDirty() {
        Super::OnMatrixDirty();
    }

    void BoneComponent::Initialize(Skeleton* pSkeleton, uint16_t boneIndex) {
        m_skeleton.SetPathTo(pSkeleton->GetEntity());
        SRAssert(m_skeleton.GetComponent<Skeleton>());
        m_boneIndex = boneIndex;
    }
}
