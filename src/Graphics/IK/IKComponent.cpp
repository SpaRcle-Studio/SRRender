//
// Created by Monika on 22.11.2025.
//

#include <Graphics/IK/IKComponent.h>
#include <Graphics/IK/IKUtils.h>

#include <Utils/ECS/Transform.h>

#include <Codegen/IKComponent.generated.hpp>

namespace SR_GRAPH_NS {
    void IKComponent::Update(float_t dt) {
        SR_TRACY_ZONE;

        if (m_type == IKType::TwoBone) {
            auto&& pRoot = m_root.Get();
            auto&& pMid = m_mid.Get();
            auto&& pTip = m_tip.Get();
            auto&& pTarget = m_target.Get();
            auto&& pHint = m_hint.Get();

            if (!pRoot || !pMid || !pTip || !pTarget) {
                return;
            }

            //IK::SolveTwoBoneIK_UE(
            //    *pRoot->GetTransform(),
            //    *pMid->GetTransform(),
            //    *pTip->GetTransform(),
            //    pTarget->GetTransform()->GetMatrix().GetTranslate(),
            //    pHint ? pHint->GetTransform()->GetMatrix().GetEulers() : SR_MATH_NS::FVector3()
            //);

            IK::SolveTwoBoneIK(
                *pRoot->GetTransform(),
                *pMid->GetTransform(),
                *pTip->GetTransform(),
                *pTarget->GetTransform(),
                pHint ? pHint->GetTransform()->GetMatrix().GetTranslate() : std::optional<SR_MATH_NS::FVector3>(),
                m_ikState,
                m_targetPositionWeight * m_weight,
                m_targetRotationWeight * m_weight,
                m_hintWeight * m_weight
            );
        }
    }
}