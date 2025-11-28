//
// Created by Monika on 22.11.2025.
//

#include <Graphics/IK/IKComponent.h>
#include <Graphics/IK/IKTwoBoneSolver.h>

#include <Utils/ECS/Transform.h>

#include <Codegen/IKComponent.generated.hpp>

namespace SR_GRAPH_NS {
    void IKComponent::Update(float_t dt) {
        SR_TRACY_ZONE;

     //  ik.init();

     //  /* Create a solver using the FABRIK algorithm */
     //  struct ik_solver_t* solver = ik.solver.create(IK_FABRIK);

     //  /* Create a simple 3-bone structure */
     //  struct ik_node_t* root = solver->node->create(0);
     //  struct ik_node_t* child1 = solver->node->create_child(root, 1);
     //  struct ik_node_t* child2 = solver->node->create_child(child1, 2);
     //  struct ik_node_t* child3 = solver->node->create_child(child2, 3);

     //  /* Set node positions in local space so they form a straight line in the Y direction*/
     //  child1->position = ik.vec3.vec3(0, 10, 0);
     //  child2->position = ik.vec3.vec3(0, 10, 0);
     //  child3->position = ik.vec3.vec3(0, 10, 0);

     //  /* Attach an effector at the end */
     //  struct ik_effector_t* eff = solver->effector->create();
     //  solver->effector->attach(eff, child3);

     //  /* set the target position of the effector to be somewhere within range */
     //  eff->target_position = ik.vec3.vec3(2, -3, 5);

     //  /* We want to calculate rotations as well as positions */
     //  solver->flags |= IK_ENABLE_TARGET_ROTATIONS;

     //  /* Assign our tree to the solver, rebuild data and calculate solution */
     //  ik.solver.set_tree(solver, root);
     //  ik.solver.rebuild(solver);
     //  ik.solver.solve(solver);

        if (m_type == IKType::TwoBone) {
            auto&& pRoot = m_root.Get();
            auto&& pMid = m_mid.Get();
            auto&& pTip = m_tip.Get();
            auto&& pTarget = m_target.Get();
            auto&& pHint = m_hint.Get();

            if (!pRoot || !pMid || !pTip || !pTarget) {
                return;
            }

            /*IK::SolveTwoBoneIK_Twist(
                *pRoot->GetTransform(),
                *pMid->GetTransform(),
                *pTip->GetTransform(),
                *pTarget->GetTransform(),
                pHint ? pHint->GetTransform()->GetMatrix().GetTranslate() : std::optional<SR_MATH_NS::FVector3>(),
                m_ikState,
                m_targetPositionWeight * m_weight,
                m_targetRotationWeight * m_weight,
                m_hintWeight * m_weight
            );*/

            IK::IKTwoBoneParams params;
            params.weight = m_weight;
            params.smoothing = m_smoothing;
            params.useInitialRotations = m_useInitialRotations;
            params.rootAngleLimit = m_rootAngleLimit;
            params.midAngleLimit = m_midAngleLimit;
            params.maxTwistChangePerFrame = m_maxTwistChangePerFrame;
            params.preventTwist = m_preventTwist;
            params.showDebugGizmos = m_showDebugGizmos;
            params.dt = dt;

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
}