//
// Created by Monika on 20.06.2026.
//

#include <Graphics/Animations/Retarget/RetargetProfile.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Types/MeshSceneStructure.h>
#include <Utils/Types/RawMesh.h>

#include <Codegen/RetargetProfile.generated.hpp>

namespace SR_ANIMATIONS_NS {
    SR_UTILS_NS::GameObject::Ptr RetargetAnimationSystem::BuildSkeletonHierarchy(const SkeletonRig& rig) const {
        SR_TRACY_ZONE;

        auto&& pRawMesh = rig.GetSkeleton().GetRawMesh();

        static SR_UTILS_NS::Vector<SR_UTILS_NS::GameObject::Ptr> nodesPool;
        nodesPool.resize(pRawMesh->GetSceneStructure().GetNodesCount());

        pRawMesh->GetSceneStructure().ForEachNode(true, [&](const SR_HTYPES_NS::MeshSceneStructure::SceneNode& node) {
            auto&& pGameObject = nodesPool.emplace_back(SRNew<SR_UTILS_NS::GameObject>(node.name));
            pGameObject->GetTransform()->SetMatrix(node.localTransform.translation, node.localTransform.rotation, node.localTransform.scale);

            nodesPool[node.index] = pGameObject;

            if (node.parent) {
                nodesPool[node.parent.value()]->AddChild(pGameObject.StaticCast<SR_UTILS_NS::SceneObject>());
            }
        });

        if (nodesPool.empty()) {
            SR_ERROR("RetargetAnimationSystem::BuildSkeletonHierarchy() : failed to build skeleton hierarchy!");
            return nullptr;
        }

        nodesPool.front()->GetTransform()->SetTranslation(rig.GetWorldSettings().translationOffset);
        nodesPool.front()->GetTransform()->SetRotation(rig.GetWorldSettings().rotationOffset);
        nodesPool.front()->GetTransform()->SetScale(rig.GetWorldSettings().scaleFactor);

        if (auto&& pSkeleton = SR_ANIMATIONS_NS::Skeleton::ImportSkeletonFromRawMesh(*pRawMesh)) {
            nodesPool.front()->AddComponent(pSkeleton.StaticCast<SR_UTILS_NS::Component>());
            pSkeleton->SetRig(rig.GetResourcePath());
            pSkeleton->ReCalculateSkeleton();
        }

        SR_UTILS_NS::GameObject::Ptr pRoot = nodesPool.front();
        nodesPool.clear();

        return pRoot;
    }

    bool RetargetAnimationSystem::PrepareContext(RetargetAnimationContext& context) const {
        SR_TRACY_ZONE;

        auto&& pSourceSkeleton = BuildSkeletonHierarchy(context.sourceRig);
        auto&& pTargetSkeleton = BuildSkeletonHierarchy(context.targetRig);

        if (!pSourceSkeleton || !pTargetSkeleton) {
            SR_ERROR("RetargetAnimationSystem::PrepareContext() : failed to build skeleton hierarchy!");
            return false;
        }

        context.pSourceSkeletonHierarchy = pSourceSkeleton;
        context.pTargetSkeletonHierarchy = pTargetSkeleton;
        context.pSourceSkeleton = pSourceSkeleton->GetComponent<Skeleton>();
        context.pTargetSkeleton = pTargetSkeleton->GetComponent<Skeleton>();

        for (auto&& channel : context.sourceChannels) {
            context.maxKeyFrame = SR_MAX(context.maxKeyFrame, channel.GetKeys().size());
        }

        return true;
    }

    bool RetargetAnimationSystem::Retarget(RetargetAnimationContext& context) {
        SR_TRACY_ZONE;

        if (context.sourceChannels.empty()) {
            SR_ERROR("RetargetAnimationSystem::Retarget() : source animation has no channels!");
            return false;
        }

        RetargetProfileEmbedded defaultProfile;
        if (!context.pProfile) {
            defaultProfile = RetargetProfileEmbedded::CreateDefault();
            defaultProfile.sourceRig = context.sourceRig.GetResourcePath();
            defaultProfile.targetRig = context.targetRig.GetResourcePath();
            context.pProfile = &defaultProfile; /// NOLINT
        }

        if (!context.pProfile->algorithm) {
            SR_ERROR("RetargetAnimationSystem::Retarget() : profile has no algorithm!");
            return false;
        }

        if (!PrepareContext(context)) {
            SR_ERROR("RetargetAnimationSystem::Retarget() : failed to prepare context!");
            return false;
        }

        const bool result = context.pProfile->algorithm->Retarget(context);

        context.pSourceSkeletonHierarchy->Destroy();
        context.pTargetSkeletonHierarchy->Destroy();

        SRAssert2(context.pSourceSkeletonHierarchy.GetPtrData()->GetStrongCount() == 1, "Source skeleton hierarchy is not destroyed!");
        SRAssert2(context.pTargetSkeletonHierarchy.GetPtrData()->GetStrongCount() == 1, "Target skeleton hierarchy is not destroyed!");

        return result;
    }

    RetargetProfileEmbedded RetargetProfileEmbedded::CreateDefault() {
        SR_TRACY_ZONE;

        RetargetProfileEmbedded profile;

        auto&& factory = SR_UTILS_NS::Factory::Instance();

        auto&& algorithmInheritances = factory.GetInheritances(RetargetAlgorithmBase::GetClassStaticName());
        for (auto&& algorithmInheritance : algorithmInheritances) {
            if (factory.IsAbstract(algorithmInheritance)) {
                continue;
            }
            if (auto&& pAlgorithm = factory.Create<RetargetAlgorithmBase>(algorithmInheritance)) {
                profile.algorithm = pAlgorithm;
                break;
            }
        }

        auto&& ikCorrectionInheritances = factory.GetInheritances(RetargetIKCorrectionBase::GetClassStaticName());
        for (auto&& ikCorrectionInheritance : ikCorrectionInheritances) {
            if (factory.IsAbstract(ikCorrectionInheritance)) {
                continue;
            }
            if (auto&& pIKCorrection = factory.Create<RetargetIKCorrectionBase>(ikCorrectionInheritance)) {
                profile.IKCorrection = pIKCorrection;
                break;
            }
        }

        auto&& poseRefinementInheritances = factory.GetInheritances(RetargetPoseRefinementBase::GetClassStaticName());
        for (auto&& poseRefinementInheritance : poseRefinementInheritances) {
            if (factory.IsAbstract(poseRefinementInheritance)) {
                continue;
            }
            if (auto&& pPoseRefinement = factory.Create<RetargetPoseRefinementBase>(poseRefinementInheritance)) {
                profile.poseRefinement = pPoseRefinement;
                break;
            }
        }

        return profile;
    }
}