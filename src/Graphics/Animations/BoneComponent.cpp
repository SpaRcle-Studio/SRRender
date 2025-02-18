//
// Created by Monika on 28.07.2023.
//

#include <Graphics/Animations/BoneComponent.h>
#include <Graphics/Animations/Skeleton.h>
#include <Utils/ECS/ComponentManager.h>

#include <Codegen/BoneComponent.generated.hpp>

namespace SR_ANIMATIONS_NS {
    BoneComponent::BoneComponent()
        : Super()
        , m_skeleton(GetThis())
    { }

    void BoneComponent::Initialize(Skeleton* pSkeleton, uint16_t boneIndex) {
        m_skeleton.SetPathTo(pSkeleton->GetEntity());
        SRAssert(m_skeleton.GetComponent<Skeleton>());
        m_boneIndex = boneIndex;
    }
}
