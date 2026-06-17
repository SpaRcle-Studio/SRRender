//
// Created by Monika on 18.06.2026.
//

#include <Graphics/Animations/RetargetAnimation.h>
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

                const auto& trs = node.transform;
                out.refLocalT[i] = trs.translation;
                out.refLocalR[i] = trs.rotation;
                out.refLocalS[i] = trs.scale;
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
            /// Fallback to legacy math if skeleton metadata is missing.
            return BrokenLegacyRetarget(sourceRig, targetRig, sourceChannels, outTargetChannels);
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
                    const SR_MATH_NS::Matrix4x4& tgtRefCS = tgtGraph.refNodeCS[ni];
                    const SR_MATH_NS::Matrix4x4& srcRefCS = srcGraph.refNodeCS[srcNode];
                    const SR_MATH_NS::Matrix4x4& srcAnimCS = srcNodeCS[srcNode];

                    SR_MATH_NS::FVector3 srcRefT, srcAnimT, tgtRefT;
                    SR_MATH_NS::Quaternion srcRefR, srcAnimR, tgtRefR;
                    SR_MATH_NS::FVector3 srcRefS, srcAnimS, tgtRefS;

                    srcRefCS.Decompose(srcRefT, srcRefR, srcRefS);
                    srcAnimCS.Decompose(srcAnimT, srcAnimR, srcAnimS);
                    tgtRefCS.Decompose(tgtRefT, tgtRefR, tgtRefS);

                    const SR_MATH_NS::Quaternion deltaR = srcAnimR * srcRefR.Inverse();
                    const SR_MATH_NS::Quaternion desiredR = (deltaR * tgtRefR).Normalized();

                    if (translationRetargetNodes.count(ni) != 0) {
                        const SR_MATH_NS::FVector3 desiredT = tgtRefT + (srcAnimT - srcRefT);
                        const SR_MATH_NS::Matrix4x4 desiredCS = SR_MATH_NS::Matrix4x4::CreateTRS(desiredT, desiredR, tgtRefS);
                        localFinal = parentCS.Inverse() * desiredCS;
                    }
                    else {
                        SR_MATH_NS::FVector3 refLocalT;
                        SR_MATH_NS::Quaternion refLocalR;
                        SR_MATH_NS::FVector3 refLocalS;
                        tgtGraph.refNodeLocal[ni].Decompose(refLocalT, refLocalR, refLocalS);

                        SR_MATH_NS::FVector3 parentT;
                        SR_MATH_NS::Quaternion parentR;
                        SR_MATH_NS::FVector3 parentS;
                        parentCS.Decompose(parentT, parentR, parentS);

                        const SR_MATH_NS::Quaternion localR = (parentR.Inverse() * desiredR).Normalized();
                        localFinal = SR_MATH_NS::Matrix4x4::CreateTRS(refLocalT, localR, refLocalS);
                    }
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

    SR_NODISCARD bool RetargetAnimation::BrokenLegacyRetarget(
        const SkeletonRig& sourceRig,
        const SkeletonRig& targetRig,
        const Channels& sourceChannels,
        Channels& outTargetChannels
    ) {
        /** formula:
         *  prepare offsets: offset = bindTarget * inverse(bindSource)
         *  and when animating: key = offset * key * inverse(offset)
        */

        /// Иногда риги импортируются в разных глобальных базисах (разные FBX пайплайны),
        /// и тогда локальные оси костей не совпадают. Приводим источник в базис цели
        /// через bind-поворот "Hips" как опорной кости.
        SR_MATH_NS::Quaternion rootBasis = SR_MATH_NS::Quaternion::Identity();
        if (auto&& pSourceHips = sourceRig.GetBoneChain(SR_UTILS_NS::StringAtom("Hips"))) {
            if (auto&& pTargetHips = targetRig.GetBoneChain(SR_UTILS_NS::StringAtom("Hips"))) {
                const auto& srcHipsR = pSourceHips->bones.front().bindRotation;
                const auto& tgtHipsR = pTargetHips->bones.front().bindRotation;
                rootBasis = tgtHipsR * srcHipsR.Inverse();
            }
        }
        const SR_MATH_NS::Quaternion rootBasisInv = rootBasis.Inverse();

        outTargetChannels = sourceChannels;

        for (auto&& channel : outTargetChannels) {
            SR_UTILS_NS::StringAtom sourceName;
            auto&& pSourceChain = sourceRig.RetargetBone(channel.GetChannelName(), sourceName);
            if (!pSourceChain) {
                continue;
            }
            auto&& pTargetChain = targetRig.GetBoneChain(sourceName);
            if (!pTargetChain) {
                continue;
            }

            auto&& sourceBoneInfo = pSourceChain->bones.front();
            auto&& targetBoneInfo = pTargetChain->bones.front();

            channel.SetName(targetBoneInfo.name);
            channel.SetBoneIndex(targetBoneInfo.index);

            const auto& sourceBindT = sourceBoneInfo.bindTranslation;
            const auto& sourceBindR = sourceBoneInfo.bindRotation;
            const auto& sourceBindS = sourceBoneInfo.bindScale;

            const auto& targetBindT = targetBoneInfo.bindTranslation;
            const auto& targetBindR = targetBoneInfo.bindRotation;
            const auto& targetBindS = targetBoneInfo.bindScale;



            //const auto Bs = sourceBoneInfo.bindRotation;
            //const auto Bt = targetBoneInfo.bindRotation;

            //// conversion between spaces
            //const auto C = Bt * Bs.Inverse();


            //const SR_MATH_NS::FVector3 tOffset = targetBoneInfo.bindTranslation - sourceBoneInfo.bindTranslation;
            const SR_MATH_NS::Quaternion sourceBindRAdj = rootBasis * sourceBindR * rootBasisInv;
            const SR_MATH_NS::FVector3 sourceBindTAdj = sourceBindT.Rotate(rootBasis);

            const SR_MATH_NS::Quaternion qOffset = targetBindR * sourceBindRAdj.Inverse();
            //const SR_MATH_NS::FVector3 sOffset = targetBoneInfo.bindScale / sourceBoneInfo.bindScale;

            for (UnionAnimationKey& key : channel.GetKeys()) {
                switch (key.type) {
                    case AnimationKeyType::Rotation: {
                        //auto&& rotation = key.data.rotation.rotation;
                        //rotation = qOffset * rotation * qOffset.Inverse();

                        auto& rotation = key.data.rotation.rotation;
                        /// приводим ключ источника в базис цели (глобально, через hips)
                        const SR_MATH_NS::Quaternion rotationAdj = rootBasis * rotation * rootBasisInv;

                        /// delta относительно bind позы источника
                        SR_MATH_NS::Quaternion delta = sourceBindRAdj.Inverse() * rotationAdj;

                        /// конвертация базиса: source local -> target local
                        /// (иначе при разных осях костей дельта крутится "не туда")
                        const SR_MATH_NS::Quaternion basis = targetBindR.Inverse() * sourceBindRAdj;
                        delta = basis * delta * basis.Inverse();

                        /// применяем дельту к bind позе цели
                        rotation = targetBindR * delta;


                        // animation delta in source space
                        //const auto Rdelta = Bs.Inverse() * rotation;
                        //rotation = Bt * C * Rdelta * C.Inverse();

                        break;
                    }
                    case AnimationKeyType::Translation: {
                        //auto&& translation = key.data.translation.translation;
                        //translation += tOffset;


                        auto& translation = key.data.translation.translation;
                        const SR_MATH_NS::FVector3 translationAdj = translation.Rotate(rootBasis);
                        const SR_MATH_NS::FVector3 delta = translationAdj - sourceBindTAdj;
                        translation = targetBindT + delta.Rotate(qOffset);

                        break;
                    }
                    case AnimationKeyType::Scaling: {
                        //auto&& scale = key.data.scaling.scaling;
                        //scale *= sOffset;

                        auto& scale = key.data.scaling.scaling;
                        const SR_MATH_NS::FVector3 delta = scale / sourceBindS;
                        scale = targetBindS * delta;
                        break;
                    }
                    default: {
                        SRHalt("AnimationClip::RetargetChannels() : unknown key type!");
                        break;
                    }
                }
            }

        }

        outTargetChannels.erase_if([](const AnimationChannel& channel) {
            return !channel.IsValid();
        });

        return true;
    }
}