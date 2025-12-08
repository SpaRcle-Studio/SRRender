//
// Created by Monika on 22.11.2025.
//

#include <Graphics/IK/IKSystem.h>
#include <Graphics/IK/IKTwoBoneSolver.h>

#include <Utils/ECS/Transform.h>
#include <Utils/Types/Time.h>

#include <Codegen/IKSystem.generated.hpp>

namespace SR_GRAPH_NS {
    void IKSystem::LateUpdate() {
        Super::LateUpdate();

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

            IK::IKTwoBoneParams params;
            params.weight = m_weight;
            params.smoothing = m_smoothing;
            params.useInitialRotations = m_useInitialRotations;
            params.rootAngleLimit = m_rootAngleLimit;
            params.midAngleLimit = m_midAngleLimit;
            params.maxTwistChangePerFrame = m_maxTwistChangePerFrame;
            params.preventTwist = m_preventTwist;
            params.showDebugGizmos = m_showDebugGizmos;
            params.tipRotationFromTarget = m_tipRotationFromTarget;
            params.dt = SR_HTYPES_NS::Time::Instance().DeltaTime();

            m_twoBoneState.ikRotation = m_rotationReference ? m_rotationReference.Get()->GetTransform()->GetGlobalRotation() : GetTransform()->GetGlobalRotation();

            IK::SolveTwoBone(
                *pRoot->GetTransform(),
                *pMid->GetTransform(),
                *pTip->GetTransform(),
                *pTarget->GetTransform(),
                pHint ? pHint->GetTransform().Get() : nullptr,
                m_twoBoneState,
                params
            );
        }
    }

    void IKSystem::OnDisable() {
        IK::RemoveTwoBoneIKDebugGizmos(m_twoBoneState);
        Super::OnDisable();
    }
}