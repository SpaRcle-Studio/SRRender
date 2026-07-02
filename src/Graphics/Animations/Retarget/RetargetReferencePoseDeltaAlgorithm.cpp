//
// Created by Monika on 01.07.2026.
//

#include <Graphics/Animations/Retarget/RetargetReferencePoseDeltaAlgorithm.h>

#include <Graphics/Animations/AnimationChannel.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/HumanoidBoneType.h>

#include <Utils/Common/EnumReflector.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/Types/SetVector.h>

#include <Codegen/RetargetReferencePoseDeltaAlgorithm.generated.hpp>

namespace SR_ANIMATIONS_NS {
    bool RetargetReferencePoseDeltaAlgorithm::Retarget(RetargetAnimationContext& context) const {
        SR_TRACY_ZONE;

        if (!context.pSourceSkeleton || !context.pTargetSkeleton) {
            SR_ERROR("RetargetReferencePoseDeltaAlgorithm::Retarget() : source or target skeleton is null!");
            return false;
        }

        PrepareState(context);

        if (context.pProfile && context.pProfile->IKCorrection) {
            context.pProfile->IKCorrection->ResetState();
        }

        const float_t dt = 1.f / m_bakeFPS;
        uint32_t frame = 0;
        bool isEnded = false;

        do {
            isEnded = AnimateSourceObject(context, dt);
            ApplyAnimation(context);

            RetargetReferencePoseDeltaAlgorithmState::RetargetFrameContext frameContext(GetState().rotationFollowStates);
            frameContext.pSourceSkeleton = context.pSourceSkeleton;
            frameContext.pTargetSkeleton = context.pTargetSkeleton;
            frameContext.targetHipsOffset = m_targetHipsOffset;
            frameContext.scaleFactor = m_scaleFactor;

            RetargetFrame(frameContext, frame);

            if (context.pProfile && context.pProfile->IKCorrection) {
                context.pProfile->IKCorrection->Apply(context);
            }

            SaveFrameToAnimation(context, frame++);
        }
        while (!isEnded);

        context.targetChannels.erase_if([](const AnimationChannel& channel) {
            return !channel.IsValid();
        });

        ClearState();

        return true;
    }

    void RetargetReferencePoseDeltaAlgorithm::SaveFrameToAnimation(RetargetAnimationContext& context, uint32_t frame) const {
        SR_TRACY_ZONE;

        auto&& targetChannels = context.targetChannels;
        auto&& pTargetSkeleton = context.pTargetSkeleton;
        if (!pTargetSkeleton) {
            return;
        }

        const float_t timePoint = static_cast<float_t>(frame) / m_bakeFPS;

        auto&& targetSkeletonHolder = context.targetRig.GetSkeleton();
        auto&& pTargetRawMesh = targetSkeletonHolder.GetRawMesh();
        const uint32_t targetMeshId = targetSkeletonHolder.GetMeshId();

        auto getOrCreateChannel = [&](SR_UTILS_NS::StringAtom boneName, AnimationKeyType type) -> AnimationChannel* {
            /// Fast-path: find existing channel with same bone name and key type.
            for (auto&& channel : targetChannels) {
                if (channel.GetChannelName() != boneName) {
                    continue;
                }

                auto&& keys = channel.GetKeys();
                if (!keys.empty() && keys.front().type == type) {
                    return &channel;
                }
            }

            /// Create new typed channel for this bone.
            auto&& channel = targetChannels.emplace_back();
            channel.SetName(boneName);
            channel.ReserveKeys(context.maxKeyFrame);

            if (pTargetRawMesh && targetMeshId != SR_ID_INVALID) {
                const auto& info = pTargetRawMesh->GetBoneInfo(targetMeshId, boneName);
                if (info.boneId.has_value()) {
                    channel.SetBoneIndex(static_cast<uint16_t>(info.boneId.value()));
                }
            }

            return &channel;
        };

        pTargetSkeleton->ForEachTransform([&](auto&& transform) {
            auto&& pGameObject = transform.GetGameObject();
            if (!pGameObject) {
                return;
            }

            const SR_UTILS_NS::StringAtom boneName = pGameObject->GetName();
            if (boneName.empty()) {
                return;
            }

            if (auto&& pTChannel = getOrCreateChannel(boneName, AnimationKeyType::Translation)) {
                pTChannel->AddKey(timePoint, TranslationKey(transform.GetTranslation()));
            }

            if (auto&& pRChannel = getOrCreateChannel(boneName, AnimationKeyType::Rotation)) {
                pRChannel->AddKey(timePoint, RotationKey(transform.GetQuaternion().NormalizeSafe()));
            }

            if (auto&& pSChannel = getOrCreateChannel(boneName, AnimationKeyType::Scaling)) {
                pSChannel->AddKey(timePoint, ScalingKey(transform.GetScale()));
            }
        });
    }

    RetargetReferencePoseDeltaAlgorithmState& RetargetReferencePoseDeltaAlgorithm::GetState() {
        static RetargetReferencePoseDeltaAlgorithmState state;
        return state;
    }

    void RetargetReferencePoseDeltaAlgorithm::PrepareState(const RetargetAnimationContext& context) const {
        SR_TRACY_ZONE;

        auto&& state = GetState();
        auto&& gameObjects = state.gameObjects;

        gameObjects.reserve(context.sourceChannels.size());
        state.channelPlayState.resize(context.sourceChannels.size());
        memset(state.channelPlayState.data(), 0, state.channelPlayState.size() * sizeof(uint32_t));

        for (auto&& channel : context.sourceChannels) {
            auto&& channelContext = state.channelContexts.emplace_back();
            if (channel.HasBoneIndex()) {
                auto&& pBone = context.pSourceSkeleton->GetAnimationBone(channel.GetChannelName());
                if (!pBone) {
                    continue;
                }

                auto&& pBoneGameObject = pBone->GetGameObject();
                if (!pBoneGameObject) {
                    continue;
                }

                auto&& pIt = std::find(gameObjects.begin(), gameObjects.end(), pBoneGameObject);
                if (pIt != gameObjects.end()) {
                    channelContext.gameObjectIndex = static_cast<uint16_t>(std::distance(gameObjects.begin(), pIt));
                    continue;
                }

                channelContext.gameObjectIndex = static_cast<uint16_t>(gameObjects.size());
                gameObjects.emplace_back(pBoneGameObject);
            }
        }

        state.poseGameObjects.resize(gameObjects.size());
    }

    bool RetargetReferencePoseDeltaAlgorithm::AnimateSourceObject(const RetargetAnimationContext& context, float_t dt) const {
        SR_TRACY_ZONE;

        auto&& channels = context.sourceChannels;
        const auto channelsCount = static_cast<uint32_t>(channels.size());

        uint32_t currentKeyFrame = 0;

        auto&& state = GetState();
        auto&& channelsPlayState = state.channelPlayState;
        float_t& animationTime = state.animationTime;

        ChannelAnimationUpdateContext channelAnimationContext;
        channelAnimationContext.weight = m_animationWeight;
        channelAnimationContext.fpsCompensation = m_animationFPSCompensation;
        channelAnimationContext.tolerance = m_animationTolerance / 1000.f / 1000.f;
        channelAnimationContext.frameRate = SR_MAX(1, m_animationFrameRate);

        for (uint32_t i = 0; i < channelsCount; ++i) {
            ChannelUpdateContext& channelContext = state.channelContexts[i];
            if (!channelContext.gameObjectIndex) SR_UNLIKELY_ATTRIBUTE {
                currentKeyFrame = SR_MAX(currentKeyFrame, channelsPlayState[i]);
                continue;
            }

            auto&& data = state.poseGameObjects[channelContext.gameObjectIndex.value()];

            uint32_t keyFrame = 0;
            if (m_animationWeight > 0.f && m_animationWeight < 1.f) SR_UNLIKELY_ATTRIBUTE {
                keyFrame = channels[i].UpdateChannelWithWeight(channelsPlayState[i], animationTime, channelAnimationContext, data);
            }
            else {
                keyFrame = channels[i].UpdateChannel(channelsPlayState[i], animationTime, channelAnimationContext, data);
            }
            currentKeyFrame = SR_MAX(currentKeyFrame, keyFrame);
        }

        animationTime += dt;

        return currentKeyFrame >= context.maxKeyFrame;
    }

    void RetargetReferencePoseDeltaAlgorithm::RetargetFrame(RetargetReferencePoseDeltaAlgorithmState::RetargetFrameContext& context, uint32_t frame) {
        SR_TRACY_ZONE;

        auto&& sourceRig = *context.pSourceSkeleton->GetRig();
        auto&& targetRig = *context.pTargetSkeleton->GetRig();

        auto&& pSourceRawMesh = sourceRig.GetSkeleton().GetRawMesh();
        auto&& pTargetRawMesh = targetRig.GetSkeleton().GetRawMesh();

        if (!pSourceRawMesh || !pTargetRawMesh) {
            return;
        }

        const auto sourceMeshId = sourceRig.GetSkeleton().GetMeshId();
        const auto targetMeshId = targetRig.GetSkeleton().GetMeshId();

        if (sourceMeshId == SR_ID_INVALID || targetMeshId == SR_ID_INVALID) {
            return;
        }

        const auto& srcScene = pSourceRawMesh->GetSceneStructure();
        const auto& tgtScene = pTargetRawMesh->GetSceneStructure();

        const uint16_t srcNodesCount = srcScene.GetNodesCount();
        const uint16_t tgtNodesCount = tgtScene.GetNodesCount();

        if (srcNodesCount == 0 || tgtNodesCount == 0) {
            return;
        }

        const SR_MATH_NS::Quaternion sourceRigSkeletonRot = SR_MATH_NS::Quaternion().NormalizeSafe();
        const SR_MATH_NS::Quaternion targetRigSkeletonRot = SR_MATH_NS::Quaternion().NormalizeSafe();

        auto buildRefCSRot = [](
            const SR_ANIMATIONS_NS::SkeletonRig& rig,
            const SR_HTYPES_NS::MeshSceneStructure& scene,
            const SR_MATH_NS::Quaternion& rigSkeletonRotation
        ) -> SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> {
            const uint16_t nodesCount = scene.GetNodesCount();

            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> refLocalR;
            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> refCSRot;
            refLocalR.resize(nodesCount);
            refCSRot.resize(nodesCount);

            for (uint16_t i = 0; i < nodesCount; ++i) {
                const auto& node = scene.GetNodeByIndex(i);

                refLocalR[i] = node.localTransform.rotation;

                SR_MATH_NS::DecomposedMatrix poseOverride;
                if (rig.TryGetRetargetPoseLocal(node.name, poseOverride)) {
                    refLocalR[i] = poseOverride.rotation;
                }

                if (node.parent.has_value()) {
                    const uint16_t parent = node.parent.value();
                    refCSRot[i] = (refCSRot[parent] * refLocalR[i]).NormalizeSafe();
                }
                else {
                    refCSRot[i] = (rigSkeletonRotation * refLocalR[i]).NormalizeSafe();
                }
            }

            return refCSRot;
        };

        const auto srcRefCSRot = buildRefCSRot(sourceRig, srcScene, sourceRigSkeletonRot);
        const auto tgtRefCSRot = buildRefCSRot(targetRig, tgtScene, targetRigSkeletonRot);

        /// Precompute node depths for chain ordering (import order guarantees parent < child).
        auto buildNodeDepth = [](const SR_HTYPES_NS::MeshSceneStructure& scene) -> SR_UTILS_NS::Vector<uint32_t> {
            const uint16_t n = scene.GetNodesCount();
            SR_UTILS_NS::Vector<uint32_t> depth;
            depth.resize(n);
            for (uint16_t i = 0; i < n; ++i) {
                const auto& node = scene.GetNodeByIndex(i);
                if (node.parent.has_value()) {
                    const uint16_t parent = node.parent.value();
                    depth[i] = parent < i ? (depth[parent] + 1u) : 0u;
                }
                else {
                    depth[i] = 0u;
                }
            }
            return depth;
        };

        const auto srcNodeDepth = buildNodeDepth(srcScene);
        const auto tgtNodeDepth = buildNodeDepth(tgtScene);

        /// Map target node -> source node using humanoid chains, proportionally along chain depth.
        std::unordered_map<uint16_t, uint16_t> mappedTargetToSource;
        mappedTargetToSource.reserve(128);

        auto orderChainNodesByDepth = [](
                const SR_HTYPES_NS::RawMesh& rawMesh,
                uint32_t meshId,
                const SR_UTILS_NS::Vector<uint32_t>& nodeDepth,
                const SR_ANIMATIONS_NS::SkeletonRigBoneChain& chain
        ) -> SR_UTILS_NS::Vector<uint16_t> {
            SR_UTILS_NS::Vector<uint16_t> nodes;
            nodes.reserve(chain.bones.size());

            for (const auto& bone : chain.bones) {
                const auto& info = rawMesh.GetBoneInfo(meshId, bone.name);
                if (info.nodeIndex.has_value()) {
                    nodes.emplace_back(info.nodeIndex.value());
                }
            }

            std::sort(nodes.begin(), nodes.end(), [&](uint16_t a, uint16_t b) {
                return nodeDepth[a] < nodeDepth[b];
            });

            nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
            return nodes;
        };

        SR_UTILS_NS::EnumReflector::ForEach<SR_ANIMATIONS_NS::HumanoidBoneType>([&](SR_ANIMATIONS_NS::HumanoidBoneType type) {
            if (type == SR_ANIMATIONS_NS::HumanoidBoneType::Unknown) {
                return;
            }

            const SR_UTILS_NS::StringAtom key = SR_UTILS_NS::EnumReflector::ToStringAtom(type);
            auto&& pSrcChain = sourceRig.GetBoneChain(key);
            auto&& pTgtChain = targetRig.GetBoneChain(key);
            if (!pSrcChain || !pTgtChain) {
                return;
            }

            auto srcNodes = orderChainNodesByDepth(*pSourceRawMesh, sourceMeshId, srcNodeDepth, *pSrcChain);
            auto tgtNodes = orderChainNodesByDepth(*pTargetRawMesh, targetMeshId, tgtNodeDepth, *pTgtChain);

            if (srcNodes.empty() || tgtNodes.empty()) {
                return;
            }

            const float denomT = static_cast<float>(SR_MAX(1, static_cast<int32_t>(tgtNodes.size()) - 1));
            const float denomS = static_cast<float>(SR_MAX(1, static_cast<int32_t>(srcNodes.size()) - 1));

            for (uint32_t k = 0; k < tgtNodes.size(); ++k) {
                const float s = static_cast<float>(k) / denomT;
                const uint32_t j = static_cast<uint32_t>(std::round(s * denomS));

                const uint16_t tgtNode = tgtNodes[k];
                const uint16_t srcNode = srcNodes[SR_MIN(j, static_cast<uint32_t>(srcNodes.size() - 1))];

                if (mappedTargetToSource.count(tgtNode) == 0) {
                    mappedTargetToSource[tgtNode] = srcNode;
                }
            }
        });

        /// Dense mapping table for top-down application on target nodes.
        static SR_UTILS_NS::Vector<uint16_t> tgtToSrcNode;
        tgtToSrcNode.assign(tgtNodesCount, SR_UINT16_MAX);
        for (const auto& [tgtNode, srcNode] : mappedTargetToSource) {
            if (tgtNode < tgtToSrcNode.size()) {
                tgtToSrcNode[tgtNode] = srcNode;
            }
        }

        /// Translation retarget only for hips root (avoid stretching).
        static SR_HTYPES_NS::SetVector<uint16_t> translationRetargetNodes;
        translationRetargetNodes.clear();

        static const auto hipsKey = SR_UTILS_NS::EnumReflector::ToStringAtom(SR_ANIMATIONS_NS::HumanoidBoneType::Hips);
        if (auto&& pTgtHips = targetRig.GetBoneChain(hipsKey)) {
            auto tgtNodes = orderChainNodesByDepth(*pTargetRawMesh, targetMeshId, tgtNodeDepth, *pTgtHips);
            if (!tgtNodes.empty()) {
                translationRetargetNodes.insert(tgtNodes.front());
            }
        }

        const auto sourceSkeletonWorldRot = context.pSourceSkeleton->GetGameObject()->GetTransform()->GetGlobalRotation();
        const auto targetSkeletonWorldRot = context.pTargetSkeleton->GetGameObject()->GetTransform()->GetGlobalRotation();

        /// Apply in target import order (parent index < child index) so parent rotations are updated first.
        for (uint16_t tgtNode = 0; tgtNode < tgtNodesCount; ++tgtNode) {
            const uint16_t srcNode = tgtToSrcNode[tgtNode];
            if (srcNode == SR_UINT16_MAX) {
                continue;
            }

            const auto& tgtNodeDesc = tgtScene.GetNodeByIndex(tgtNode);
            const auto& srcNodeDesc = srcScene.GetNodeByIndex(srcNode);

            auto&& pTargetBone = context.pTargetSkeleton->GetBone(tgtNodeDesc.name);
            auto&& pSourceBone = context.pSourceSkeleton->GetBone(srcNodeDesc.name);

            if (!pTargetBone || !pSourceBone) {
                continue;
            }

            if (translationRetargetNodes.contains(tgtNode)) {
                SR_MATH_NS::Matrix4x4 sourceGlobalMatrix = pSourceBone->GetGameObject()->GetTransform()->GetMatrix();
                SR_MATH_NS::FVector3 sourceGlobalTranslation = sourceGlobalMatrix.GetTranslation();
                SR_MATH_NS::Matrix4x4 targetGlobalParentMatrix = pTargetBone->GetGameObject()->GetParentTransform()->GetMatrix();
                SR_MATH_NS::Matrix4x4 targetLocalMatrix = targetGlobalParentMatrix.Inverse() * SR_MATH_NS::Matrix4x4(sourceGlobalTranslation, SR_MATH_NS::Quaternion::Identity(), SR_MATH_NS::FVector3::One());
                pTargetBone->GetGameObject()->GetTransform()->SetTranslation(targetLocalMatrix.GetTranslation() + context.targetHipsOffset / context.scaleFactor);
            }

            if (srcNode >= srcRefCSRot.size() || tgtNode >= tgtRefCSRot.size()) {
                continue;
            }

            const SR_MATH_NS::Quaternion srcRefWorld = (sourceSkeletonWorldRot * srcRefCSRot[srcNode]).NormalizeSafe();
            const SR_MATH_NS::Quaternion tgtRefWorld = (targetSkeletonWorldRot * tgtRefCSRot[tgtNode]).NormalizeSafe();

            const SR_MATH_NS::Quaternion srcWorld = pSourceBone->GetGameObject()->GetTransform()->GetGlobalRotation().NormalizeSafe();
            const SR_MATH_NS::Quaternion deltaWorld = (srcWorld * srcRefWorld.Inverse()).NormalizeSafe();
            const SR_MATH_NS::Quaternion desiredWorld = (deltaWorld * tgtRefWorld).NormalizeSafe();

            const SR_MATH_NS::Quaternion targetParentWorld = pTargetBone->GetGameObject()->GetParentTransform()->GetGlobalRotation().NormalizeSafe();
            SR_MATH_NS::Quaternion desiredTargetLocal = (targetParentWorld.Inverse() * desiredWorld).NormalizeSafe();

            auto& state = context.rotationFollowStates[tgtNodeDesc.name];
            if (state.hasLastLocal && SR_MATH_NS::Quaternion::Dot(desiredTargetLocal, state.lastLocal) < 0.f) {
                desiredTargetLocal = -desiredTargetLocal;
            }
            state.lastLocal = desiredTargetLocal;
            state.hasLastLocal = true;

            pTargetBone->GetGameObject()->GetTransform()->SetRotation(desiredTargetLocal);
        }
    }

    void RetargetReferencePoseDeltaAlgorithm::ApplyAnimation(const RetargetAnimationContext& context) const {
        SR_TRACY_ZONE;

        auto&& gameObjectsData = GetState().poseGameObjects;
        auto&& gameObjects = GetState().gameObjects;

        {
            SR_TRACY_ZONE_N("Normalize");

            for (auto&& data : gameObjectsData) {
                if (data.rotation.has_value()) SR_LIKELY_ATTRIBUTE {
                    data.rotation = data.rotation.value().Normalized();
                }
            }
        }

        for (uint32_t i = 0; i < gameObjectsData.size(); ++i) {
            AnimationGameObjectData& data = gameObjectsData[i];
            if (!data.dirty) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            data.dirty = false;

            gameObjects[i]->GetTransform()->SetMatrix(
                data.translation,
                data.rotation,
                data.scaling
            );
        }
    }

    void RetargetReferencePoseDeltaAlgorithm::ClearState() const {
        auto&& state = GetState();

        state.animationTime = 0.f;
        state.rotationFollowStates.clear();
        state.poseGameObjects.clear();
        state.channelContexts.clear();
        state.channelPlayState.clear();
        state.gameObjects.clear();
    }
}
