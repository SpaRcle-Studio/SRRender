//
// Created by Monika on 18.06.2026.
//

/// WRONG IMPLEMENTATION, NOT WORKING

/*#include <Graphics/Animations/RetargetAnimation.h>
#include <Graphics/Animations/SkeletonRig.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/Common/EnumReflector.h>

#include <algorithm>
#include <cmath>

namespace SR_ANIMATIONS_NS {
    namespace {
        /// If false (default) we do humanoid-only retarget and ignore unmapped bones.
        /// If true we additionally try to map by identical bone names (safe only when skeletons match by name).
        static constexpr bool kEnableUnmappedBonePassThrough = false;

        struct OrientAndScaleCache {
            SR_MATH_NS::Quaternion deltaOrient = SR_MATH_NS::Quaternion::Identity();
            float_t scale = 1.f;
            SR_MATH_NS::FVector3 sourceRefT = SR_MATH_NS::FVector3::Zero();
            SR_MATH_NS::FVector3 targetRefT = SR_MATH_NS::FVector3::Zero();
            bool valid = false;
        };

        struct RigGraph final {
            const SR_HTYPES_NS::RawMesh* pRawMesh = nullptr;
            SR_HTYPES_NS::IRawMeshHolder::MeshIndex meshId = SR_ID_INVALID;

            const SR_HTYPES_NS::MeshSceneStructure* pScene = nullptr;
            const SR_HTYPES_NS::MeshSceneStructure::MeshData* pMeshData = nullptr;

            uint16_t nodesCount = 0;

            /// Node-space reference pose (local and component)
            SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> refNodeLocal;
            SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> refNodeCS;

            /// Reference TRS (local)
            SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> refLocalT;
            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> refLocalR;
            SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> refLocalS;

            /// Node hierarchy helpers (for ordering / chain sorting)
            SR_UTILS_NS::Vector<uint32_t> nodeDepth;

            /// Bone helpers
            std::unordered_map<SR_UTILS_NS::StringAtom, uint16_t> nodeIndexByName;
            std::unordered_map<SR_UTILS_NS::StringAtom, uint32_t> boneIdByName;

            struct BoneOutInfo {
                SR_UTILS_NS::StringAtom name;
                uint32_t boneId = SR_ID_INVALID;
                uint16_t nodeIndex = SR_UINT16_MAX;
            };

            SR_UTILS_NS::Vector<BoneOutInfo> orderedBones; /// sorted by boneId
        };

        SR_NODISCARD static bool BuildRigGraph(const SkeletonRig& rig, RigGraph& out) {
            auto&& skeletonHolder = rig.GetSkeleton();
            auto&& pRawMesh = skeletonHolder.GetRawMesh();
            if (!pRawMesh) {
                return false;
            }

            const auto meshId = skeletonHolder.GetMeshId();
            if (meshId == SR_ID_INVALID) {
                return false;
            }

            out.pRawMesh = pRawMesh.Get();
            out.meshId = meshId;
            out.pScene = &out.pRawMesh->GetSceneStructure();
            out.pMeshData = &out.pRawMesh->GetMeshData(meshId);

            out.nodesCount = out.pScene->GetNodesCount();
            if (out.nodesCount == 0) {
                return false;
            }

            out.refNodeLocal.resize(out.nodesCount);
            out.refNodeCS.resize(out.nodesCount);
            out.refLocalT.resize(out.nodesCount);
            out.refLocalR.resize(out.nodesCount);
            out.refLocalS.resize(out.nodesCount);
            out.nodeDepth.resize(out.nodesCount);

            /// Import order guarantees parent index < child index.
            for (uint16_t i = 0; i < out.nodesCount; ++i) {
                const auto& node = out.pScene->GetNodeByIndex(i);

                out.nodeIndexByName[node.name] = node.index;

                /// Default reference pose comes from mesh bind/rest, but can be overridden by rig retarget pose.
                out.refLocalT[i] = node.localTransform.translation;
                out.refLocalR[i] = node.localTransform.rotation;
                out.refLocalS[i] = node.localTransform.scale;

                SR_MATH_NS::DecomposedMatrix poseOverride;
                if (rig.TryGetRetargetPoseLocal(node.name, poseOverride)) {
                    out.refLocalT[i] = poseOverride.translation;
                    out.refLocalR[i] = poseOverride.rotation;
                    out.refLocalS[i] = poseOverride.scale;
                }
                out.refNodeLocal[i] = SR_MATH_NS::Matrix4x4::CreateTRS(out.refLocalT[i], out.refLocalR[i], out.refLocalS[i]);

                if (node.parent.has_value()) {
                    const uint16_t parent = node.parent.value();
                    out.nodeDepth[i] = parent < i ? (out.nodeDepth[parent] + 1u) : 0u;
                    out.refNodeCS[i] = out.refNodeCS[parent] * out.refNodeLocal[i];
                }
                else {
                    out.nodeDepth[i] = 0u;
                    out.refNodeCS[i] = out.refNodeLocal[i];
                }
            }

            out.orderedBones.clear();
            out.orderedBones.reserve(out.pMeshData->bones.size());

            for (const auto& [boneName, boneInfo] : out.pMeshData->bones) {
                if (!boneInfo.boneId.has_value() || !boneInfo.nodeIndex.has_value()) {
                    continue;
                }

                out.boneIdByName[boneName] = boneInfo.boneId.value();

                RigGraph::BoneOutInfo info;
                info.name = boneName;
                info.boneId = boneInfo.boneId.value();
                info.nodeIndex = boneInfo.nodeIndex.value();
                out.orderedBones.emplace_back(info);
            }

            std::sort(out.orderedBones.begin(), out.orderedBones.end(), [](const RigGraph::BoneOutInfo& a, const RigGraph::BoneOutInfo& b) {
                return a.boneId < b.boneId;
            });

            return true;
        }

        SR_NODISCARD static uint16_t GetNodeIndexSafe(const RigGraph& graph, SR_UTILS_NS::StringAtom name) {
            if (auto&& it = graph.nodeIndexByName.find(name); it != graph.nodeIndexByName.end()) {
                return it->second;
            }
            return SR_UINT16_MAX;
        }

        SR_NODISCARD static SR_UTILS_NS::Vector<uint16_t> OrderChainNodesByDepth(
            const RigGraph& graph,
            const SkeletonRigBoneChain& chain
        ) {
            SR_UTILS_NS::Vector<uint16_t> nodes;
            nodes.reserve(chain.bones.size());

            for (const auto& bone : chain.bones) {
                const uint16_t nodeIndex = GetNodeIndexSafe(graph, bone.name);
                if (nodeIndex != SR_UINT16_MAX) {
                    nodes.emplace_back(nodeIndex);
                }
            }

            std::sort(nodes.begin(), nodes.end(), [&](uint16_t a, uint16_t b) {
                return graph.nodeDepth[a] < graph.nodeDepth[b];
            });

            nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
            return nodes;
        }

        SR_NODISCARD static float_t ComputeChannelsDuration(const RetargetAnimation::Channels& sourceChannels) {
            float_t duration = 0.f;
            for (const auto& ch : sourceChannels) {
                for (const auto& key : ch.GetKeys()) {
                    duration = SR_MAX(duration, key.time);
                }
            }
            return duration;
        }

        SR_NODISCARD static OrientAndScaleCache BuildOrientAndScaleCache(
            const SR_MATH_NS::FVector3& sourceRefT,
            const SR_MATH_NS::FVector3& targetRefT
        ) {
            OrientAndScaleCache cache;
            cache.sourceRefT = sourceRefT;
            cache.targetRefT = targetRefT;

            const float_t srcLen = sourceRefT.Length();
            const float_t tgtLen = targetRefT.Length();
            if (srcLen <= 1e-6f || tgtLen <= 1e-6f) {
                cache.valid = false;
                return cache;
            }

            const SR_MATH_NS::FVector3 srcDir = sourceRefT / srcLen;
            const SR_MATH_NS::FVector3 tgtDir = targetRefT / tgtLen;

            cache.deltaOrient = SR_MATH_NS::Quaternion::FromToRotation(srcDir, tgtDir);
            cache.scale = tgtLen / srcLen;
            cache.valid = true;
            return cache;
        }
    }

    SR_NODISCARD bool RetargetAnimation::Retarget(
        const SkeletonRig& sourceRig,
        const SkeletonRig& targetRig,
        const Channels& sourceChannels,
        Channels& outTargetChannels
    ) {
        RigGraph srcGraph;
        RigGraph tgtGraph;

        if (!BuildRigGraph(sourceRig, srcGraph) || !BuildRigGraph(targetRig, tgtGraph)) {
            return false;
        }

        const float_t duration = ComputeChannelsDuration(sourceChannels);

        static constexpr float_t bakeFps = 60.f;
        static constexpr float_t bakeDt = 1.f / bakeFps;
        const uint32_t samplesCount = duration > 0.f ? (static_cast<uint32_t>(std::ceil(duration * bakeFps)) + 1u) : 1u;

        /// --- build output channels (full TRS bake for every target bone with valid boneId) ---
        outTargetChannels.clear();
        outTargetChannels.reserve(tgtGraph.orderedBones.size() * 3u);

        struct OutChannelsIndex {
            uint32_t t = 0;
            uint32_t r = 0;
            uint32_t s = 0;
        };

        SR_UTILS_NS::Vector<OutChannelsIndex> boneOutToChannels;
        boneOutToChannels.resize(tgtGraph.orderedBones.size());

        for (uint32_t i = 0; i < tgtGraph.orderedBones.size(); ++i) {
            const auto& bone = tgtGraph.orderedBones[i];

            const uint16_t boneIndex16 = static_cast<uint16_t>(SR_MIN(bone.boneId, SR_UINT16_MAX));

            OutChannelsIndex idx{};
            idx.t = static_cast<uint32_t>(outTargetChannels.size());
            outTargetChannels.emplace_back();
            outTargetChannels.back().SetName(bone.name);
            outTargetChannels.back().SetBoneIndex(boneIndex16);
            outTargetChannels.back().ReserveKeys(samplesCount);

            idx.r = static_cast<uint32_t>(outTargetChannels.size());
            outTargetChannels.emplace_back();
            outTargetChannels.back().SetName(bone.name);
            outTargetChannels.back().SetBoneIndex(boneIndex16);
            outTargetChannels.back().ReserveKeys(samplesCount);

            idx.s = static_cast<uint32_t>(outTargetChannels.size());
            outTargetChannels.emplace_back();
            outTargetChannels.back().SetName(bone.name);
            outTargetChannels.back().SetBoneIndex(boneIndex16);
            outTargetChannels.back().ReserveKeys(samplesCount);

            boneOutToChannels[i] = idx;
        }

        /// --- precompute mapping: target node -> source node (humanoid chains + pass-through by name) ---
        std::unordered_map<uint16_t, uint16_t> mappedTargetToSource;
        mappedTargetToSource.reserve(tgtGraph.orderedBones.size());

        /// Translation retargeting is only safe for root / hips-like bones.
        /// For the rest we keep target reference translations to avoid stretching on different proportions.
        std::unordered_set<uint16_t> translationRetargetNodes;
        translationRetargetNodes.reserve(8);

        /// Hips basis correction (legacy-style) for translation deltas.
        /// We use local ref rotations to build a stable basis and apply qOffset for translation.
        SR_MATH_NS::Quaternion hipsRootBasis = SR_MATH_NS::Quaternion::Identity();
        OrientAndScaleCache hipsOrientAndScale;

        SR_UTILS_NS::EnumReflector::ForEach<HumanoidBoneType>([&](HumanoidBoneType type) {
            if (type == HumanoidBoneType::Unknown) {
                return;
            }

            const SR_UTILS_NS::StringAtom key = SR_UTILS_NS::EnumReflector::ToStringAtom(type);
            auto&& pSrcChain = sourceRig.GetBoneChain(key);
            auto&& pTgtChain = targetRig.GetBoneChain(key);
            if (!pSrcChain || !pTgtChain) {
                return;
            }

            auto srcNodes = OrderChainNodesByDepth(srcGraph, *pSrcChain);
            auto tgtNodes = OrderChainNodesByDepth(tgtGraph, *pTgtChain);

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

            if (key == SR_UTILS_NS::StringAtom("Hips")) {
                /// Only the chain root (hips) gets translation retarget. Children keep local translations.
                translationRetargetNodes.insert(tgtNodes.front());

                /// Derive hipsRootBasis from local ref rotations once.
                hipsRootBasis = tgtGraph.refLocalR[tgtNodes.front()] * srcGraph.refLocalR[srcNodes.front()].Inverse();

                hipsOrientAndScale = BuildOrientAndScaleCache(srcGraph.refLocalT[srcNodes.front()], tgtGraph.refLocalT[tgtNodes.front()]);
            }
        });

        /// Pass-through by name (only if bone exists in both skeletons)
        if (kEnableUnmappedBonePassThrough) {
            for (const auto& tgtBone : tgtGraph.orderedBones) {
                const uint16_t tgtNode = tgtBone.nodeIndex;
                if (mappedTargetToSource.count(tgtNode) != 0) {
                    continue;
                }

                const uint16_t srcNode = GetNodeIndexSafe(srcGraph, tgtBone.name);
                if (srcNode != SR_UINT16_MAX) {
                    mappedTargetToSource[tgtNode] = srcNode;
                }
            }
        }

        /// Dense mapping table for per-node top-down evaluation.
        SR_UTILS_NS::Vector<uint16_t> tgtToSrcNode;
        tgtToSrcNode.resize(tgtGraph.nodesCount, SR_UINT16_MAX);
        for (const auto& [tgtNode, srcNode] : mappedTargetToSource) {
            if (tgtNode < tgtToSrcNode.size()) {
                tgtToSrcNode[tgtNode] = srcNode;
            }
        }

        /// --- source pose state (node-local TRS updated by channels) ---
        SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> srcLocalT;
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> srcLocalR;
        SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> srcLocalS;
        srcLocalT.resize(srcGraph.nodesCount);
        srcLocalR.resize(srcGraph.nodesCount);
        srcLocalS.resize(srcGraph.nodesCount);

        for (uint16_t i = 0; i < srcGraph.nodesCount; ++i) {
            srcLocalT[i] = srcGraph.refLocalT[i];
            srcLocalR[i] = srcGraph.refLocalR[i];
            srcLocalS[i] = srcGraph.refLocalS[i];
        }

        /// Channel -> nodeIndex lookup + cursor state for fast sequential sampling
        SR_UTILS_NS::Vector<uint16_t> channelNodeIndex;
        SR_UTILS_NS::Vector<uint32_t> channelCursor;
        channelNodeIndex.resize(sourceChannels.size());
        channelCursor.resize(sourceChannels.size());

        for (uint32_t i = 0; i < sourceChannels.size(); ++i) {
            channelNodeIndex[i] = GetNodeIndexSafe(srcGraph, sourceChannels[i].GetChannelName());
            channelCursor[i] = 0;
        }

        UpdateContext updateCtx;
        updateCtx.dt = bakeDt;
        updateCtx.fpsCompensation = false;
        updateCtx.frameRate = 1;
        updateCtx.tolerance = 0.f;

        /// --- per-frame scratch ---
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> srcNodeLocal;
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> srcNodeCS;
        srcNodeLocal.resize(srcGraph.nodesCount);
        srcNodeCS.resize(srcGraph.nodesCount);

        /// Component-space rotations (avoid Matrix4x4::Decompose artifacts, important for arms/twists).
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> srcCSRotRef;
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> srcCSRotAnim;
        srcCSRotRef.resize(srcGraph.nodesCount);
        srcCSRotAnim.resize(srcGraph.nodesCount);

        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> tgtCSRotRef;
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> tgtCSRotFinal;
        tgtCSRotRef.resize(tgtGraph.nodesCount);
        tgtCSRotFinal.resize(tgtGraph.nodesCount);

        for (uint16_t ni = 0; ni < tgtGraph.nodesCount; ++ni) {
            const auto& node = tgtGraph.pScene->GetNodeByIndex(ni);
            if (node.parent.has_value()) {
                const uint16_t parent = node.parent.value();
                tgtCSRotRef[ni] = (tgtCSRotRef[parent] * tgtGraph.refLocalR[ni]).Normalized();
            }
            else {
                tgtCSRotRef[ni] = tgtGraph.refLocalR[ni].Normalized();
            }
        }

        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> tgtNodeLocalFinal;
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> tgtNodeCSFinal;
        tgtNodeLocalFinal.resize(tgtGraph.nodesCount);
        tgtNodeCSFinal.resize(tgtGraph.nodesCount);

        /// Quaternion continuity (per target bone)
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> prevRot;
        prevRot.resize(tgtGraph.orderedBones.size(), SR_MATH_NS::Quaternion::Identity());
        SR_UTILS_NS::Vector<bool> hasPrevRot;
        hasPrevRot.resize(tgtGraph.orderedBones.size(), false);

        for (uint32_t sample = 0; sample < samplesCount; ++sample) {
            const float_t t = SR_MIN(duration, static_cast<float_t>(sample) * bakeDt);

            /// 1) Update source node-local TRS from source channels at time t
            for (uint32_t ci = 0; ci < sourceChannels.size(); ++ci) {
                const uint16_t nodeIndex = channelNodeIndex[ci];
                if (nodeIndex == SR_UINT16_MAX) {
                    continue;
                }

                AnimationGameObjectData tmp;
                channelCursor[ci] = sourceChannels[ci].UpdateChannel(channelCursor[ci], t, updateCtx, tmp);

                if (tmp.translation.has_value()) {
                    srcLocalT[nodeIndex] = tmp.translation.value();
                }
                if (tmp.rotation.has_value()) {
                    srcLocalR[nodeIndex] = tmp.rotation.value();
                }
                if (tmp.scaling.has_value()) {
                    srcLocalS[nodeIndex] = tmp.scaling.value();
                }
            }

            /// 2) Build source node matrices (local + component) in node import order
            for (uint16_t ni = 0; ni < srcGraph.nodesCount; ++ni) {
                srcNodeLocal[ni] = SR_MATH_NS::Matrix4x4::CreateTRS(srcLocalT[ni], srcLocalR[ni], srcLocalS[ni]);

                const auto& node = srcGraph.pScene->GetNodeByIndex(ni);
                if (node.parent.has_value()) {
                    const uint16_t parent = node.parent.value();
                    srcNodeCS[ni] = srcNodeCS[parent] * srcNodeLocal[ni];
                }
                else {
                    srcNodeCS[ni] = srcNodeLocal[ni];
                }
            }

            /// Build source component-space rotations (ref + anim) from local quaternions.
            for (uint16_t ni = 0; ni < srcGraph.nodesCount; ++ni) {
                const auto& node = srcGraph.pScene->GetNodeByIndex(ni);
                if (node.parent.has_value()) {
                    const uint16_t parent = node.parent.value();
                    srcCSRotRef[ni] = (srcCSRotRef[parent] * srcGraph.refLocalR[ni]).Normalized();
                    srcCSRotAnim[ni] = (srcCSRotAnim[parent] * srcLocalR[ni]).Normalized();
                }
                else {
                    srcCSRotRef[ni] = srcGraph.refLocalR[ni].Normalized();
                    srcCSRotAnim[ni] = srcLocalR[ni].Normalized();
                }
            }

            /// 3) Retarget top-down so unmapped children follow mapped parents.
            /// We compute CS from parentCS * localFinal; for mapped nodes we override localFinal.
            for (uint16_t ni = 0; ni < tgtGraph.nodesCount; ++ni) {
                const auto& tgtNode = tgtGraph.pScene->GetNodeByIndex(ni);

                const SR_MATH_NS::Matrix4x4 parentCS = tgtNode.parent.has_value()
                    ? tgtNodeCSFinal[tgtNode.parent.value()]
                    : SR_MATH_NS::Matrix4x4::Identity();

                SR_MATH_NS::Matrix4x4 localFinal = tgtGraph.refNodeLocal[ni];

                const uint16_t srcNode = tgtToSrcNode[ni];
                if (srcNode != SR_UINT16_MAX) {
                    /// Rotation retarget in component space using pure quaternions (no matrix decomposition).
                    const SR_MATH_NS::Quaternion deltaCS = srcCSRotAnim[srcNode] * srcCSRotRef[srcNode].Inverse();
                    const SR_MATH_NS::Quaternion desiredCSRot = (deltaCS * tgtCSRotRef[ni]).Normalized();

                    const SR_MATH_NS::Quaternion parentCSRot = tgtNode.parent.has_value()
                        ? tgtCSRotFinal[tgtNode.parent.value()]
                        : SR_MATH_NS::Quaternion::Identity();

                    const SR_MATH_NS::Quaternion localR = (parentCSRot.Inverse() * desiredCSRot).Normalized();
                    tgtCSRotFinal[ni] = (parentCSRot * localR).Normalized();

                    if (translationRetargetNodes.count(ni) != 0) {
                        /// UE-like OrientAndScale for Hips translation.
                        const SR_MATH_NS::FVector3 srcAnimatedLocalT = srcLocalT[srcNode];
                        const SR_MATH_NS::FVector3 sourceRefT = hipsOrientAndScale.sourceRefT;
                        const SR_MATH_NS::FVector3 targetRefT = hipsOrientAndScale.targetRefT;

                        SR_MATH_NS::FVector3 desiredLocalT = targetRefT;

                        /// If translation is not animated (or cache invalid), fall back to target ref.
                        if (hipsOrientAndScale.valid && !srcAnimatedLocalT.IsEquals(sourceRefT, 0.0001f)) {
                            desiredLocalT = (hipsOrientAndScale.deltaOrient * srcAnimatedLocalT) * hipsOrientAndScale.scale;
                        }

                        localFinal = SR_MATH_NS::Matrix4x4::CreateTRS(desiredLocalT, localR, tgtGraph.refLocalS[ni]);
                    }
                    else {
                        localFinal = SR_MATH_NS::Matrix4x4::CreateTRS(tgtGraph.refLocalT[ni], localR, tgtGraph.refLocalS[ni]);
                    }
                }
                else {
                    /// Unmapped: keep reference rotations (but still propagate component-space rotation for children).
                    const SR_MATH_NS::Quaternion parentCSRot = tgtNode.parent.has_value()
                        ? tgtCSRotFinal[tgtNode.parent.value()]
                        : SR_MATH_NS::Quaternion::Identity();
                    tgtCSRotFinal[ni] = (parentCSRot * tgtGraph.refLocalR[ni]).Normalized();
                }

                tgtNodeLocalFinal[ni] = localFinal;
                tgtNodeCSFinal[ni] = parentCS * localFinal;
            }

            /// 4) Bake local pose for each target bone, decompose to TRS, emit keys
            for (uint32_t bi = 0; bi < tgtGraph.orderedBones.size(); ++bi) {
                const auto& bone = tgtGraph.orderedBones[bi];
                const uint16_t nodeIndex = bone.nodeIndex;
                const SR_MATH_NS::Matrix4x4& localM = tgtNodeLocalFinal[nodeIndex];

                SR_MATH_NS::FVector3 outT;
                SR_MATH_NS::Quaternion outR;
                SR_MATH_NS::FVector3 outS;
                localM.Decompose(outT, outR, outS);

                outR = outR.Normalized();
                if (hasPrevRot[bi]) {
                    if (SR_MATH_NS::Quaternion::Dot(prevRot[bi], outR) < 0.f) {
                        outR = -outR;
                    }
                }
                prevRot[bi] = outR;
                hasPrevRot[bi] = true;

                const auto idx = boneOutToChannels[bi];
                outTargetChannels[idx.t].AddKey(t, TranslationKey(outT));
                outTargetChannels[idx.r].AddKey(t, RotationKey(outR));
                outTargetChannels[idx.s].AddKey(t, ScalingKey(outS));
            }
        }

        outTargetChannels.erase_if([](const AnimationChannel& channel) {
            return !channel.IsValid();
        });

        return true;
    }
}*/