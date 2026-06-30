//
// Created by Monika on 18.06.2026.
//

/// WRONG IMPLEMENTATION, NOT WORKING

/*
#include <Graphics/Animations/IKRetargetAnimation.h>
#include <Graphics/Animations/SkeletonRig.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/Common/EnumReflector.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace SR_ANIMATIONS_NS {
    namespace {
        struct RigGraph final {
            const SR_HTYPES_NS::RawMesh* pRawMesh = nullptr;
            SR_HTYPES_NS::IRawMeshHolder::MeshIndex meshId = SR_ID_INVALID;

            const SR_HTYPES_NS::MeshSceneStructure* pScene = nullptr;
            const SR_HTYPES_NS::MeshSceneStructure::MeshData* pMeshData = nullptr;

            uint16_t nodesCount = 0;

            SR_UTILS_NS::Vector<uint16_t> parentIndex; /// SR_UINT16_MAX means no parent

            /// Reference TRS (local)
            SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> refLocalT;
            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> refLocalR;
            SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> refLocalS;

            /// Reference pose matrices (local and component)
            SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> refNodeLocal;
            SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> refNodeCS;

            /// Reference component-space helpers
            SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> refCSPos;
            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> refCSRot;

            /// Ordering helpers
            SR_UTILS_NS::Vector<uint32_t> nodeDepth;

            /// Fast lookups
            std::unordered_map<SR_UTILS_NS::StringAtom, uint16_t> nodeIndexByName;

            struct BoneOutInfo {
                SR_UTILS_NS::StringAtom name;
                uint32_t boneId = SR_ID_INVALID;
                uint16_t nodeIndex = SR_UINT16_MAX;
            };

            SR_UTILS_NS::Vector<BoneOutInfo> orderedBones; /// sorted by boneId
        };

        SR_NODISCARD static bool HasValidParent(const SR_HTYPES_NS::MeshSceneStructure::SceneNode& node) {
            return node.parent.has_value() && node.parent.value() != SR_UINT16_MAX;
        }

        SR_NODISCARD static bool BuildRigGraph(const SkeletonRig& rig, RigGraph& out) {
            auto&& skeletonHolder = rig.GetSkeleton();
            auto&& pRawMeshRef = skeletonHolder.GetRawMesh();
            if (!pRawMeshRef) {
                return false;
            }

            const auto meshId = skeletonHolder.GetMeshId();
            if (meshId == SR_ID_INVALID) {
                return false;
            }

            out.pRawMesh = pRawMeshRef.Get();
            out.meshId = meshId;
            out.pScene = &out.pRawMesh->GetSceneStructure();
            out.pMeshData = &out.pRawMesh->GetMeshData(meshId);

            out.nodesCount = out.pScene->GetNodesCount();
            if (out.nodesCount == 0) {
                return false;
            }

            out.parentIndex.assign(out.nodesCount, SR_UINT16_MAX);
            out.refLocalT.resize(out.nodesCount);
            out.refLocalR.resize(out.nodesCount);
            out.refLocalS.resize(out.nodesCount);
            out.refNodeLocal.resize(out.nodesCount);
            out.refNodeCS.resize(out.nodesCount);
            out.refCSPos.resize(out.nodesCount);
            out.refCSRot.resize(out.nodesCount);
            out.nodeDepth.resize(out.nodesCount);
            out.nodeIndexByName.clear();
            out.nodeIndexByName.reserve(out.nodesCount);

            /// Scene import order guarantees that for most rigs parent index < child index,
            /// but we still guard against invalid/unsorted data.
            for (uint16_t i = 0; i < out.nodesCount; ++i) {
                const auto& node = out.pScene->GetNodeByIndex(i);
                out.nodeIndexByName[node.name] = node.index;

                if (HasValidParent(node)) {
                    out.parentIndex[i] = node.parent.value();
                }

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

                if (out.parentIndex[i] != SR_UINT16_MAX && out.parentIndex[i] < out.nodesCount && out.parentIndex[i] != i) {
                    const uint16_t parent = out.parentIndex[i];
                    out.nodeDepth[i] = parent != i ? (out.nodeDepth[parent] + 1u) : 0u;
                    out.refNodeCS[i] = out.refNodeCS[parent] * out.refNodeLocal[i];
                    out.refCSRot[i] = (out.refCSRot[parent] * out.refLocalR[i]).Normalized();
                }
                else {
                    out.parentIndex[i] = SR_UINT16_MAX;
                    out.nodeDepth[i] = 0u;
                    out.refNodeCS[i] = out.refNodeLocal[i];
                    out.refCSRot[i] = out.refLocalR[i].Normalized();
                }

                out.refCSPos[i] = out.refNodeCS[i].GetTranslate();
            }

            /// Collect bones present in the mesh with valid boneId and nodeIndex.
            out.orderedBones.clear();
            out.orderedBones.reserve(out.pMeshData->bones.size());

            for (const auto& [boneName, boneInfo] : out.pMeshData->bones) {
                if (!boneInfo.boneId.has_value() || !boneInfo.nodeIndex.has_value()) {
                    continue;
                }

                const uint16_t nodeIndex = boneInfo.nodeIndex.value();
                if (nodeIndex >= out.nodesCount) {
                    continue;
                }

                RigGraph::BoneOutInfo info;
                info.name = boneName;
                info.boneId = boneInfo.boneId.value();
                info.nodeIndex = nodeIndex;
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

        SR_NODISCARD static float_t ComputeChannelsDuration(const IKRetargetAnimation::Channels& channels) {
            float_t duration = 0.f;
            for (const auto& ch : channels) {
                for (const auto& key : ch.GetKeys()) {
                    duration = SR_MAX(duration, key.time);
                }
            }
            return duration;
        }

        struct HumanoidNodeMap final {
            std::unordered_map<HumanoidBoneType, uint16_t> nodeByType;
        };

        SR_NODISCARD HumanoidNodeMap BuildHumanoidNodeMap(const SkeletonRig& rig, const RigGraph& graph) {
            HumanoidNodeMap out;
            out.nodeByType.reserve(64);

            SR_UTILS_NS::EnumReflector::ForEach<HumanoidBoneType>([&](HumanoidBoneType type) {
                if (type == HumanoidBoneType::Unknown) {
                    return;
                }

                const SR_UTILS_NS::StringAtom key = SR_UTILS_NS::EnumReflector::ToStringAtom(type);
                auto&& pChain = rig.GetBoneChain(key);
                if (!pChain || pChain->bones.empty()) {
                    return;
                }

                /// Prefer the first bone that exists in this mesh scene structure.
                for (const auto& boneInfo : pChain->bones) {
                    const uint16_t nodeIndex = GetNodeIndexSafe(graph, boneInfo.name);
                    if (nodeIndex != SR_UINT16_MAX) {
                        out.nodeByType[type] = nodeIndex;
                        return;
                    }
                }
            });

            return out;
        }

        SR_NODISCARD uint16_t GetHumanoidNode(const HumanoidNodeMap& map, HumanoidBoneType type) {
            if (auto&& it = map.nodeByType.find(type); it != map.nodeByType.end()) {
                return it->second;
            }
            return SR_UINT16_MAX;
        }

        SR_NODISCARD SR_UTILS_NS::Vector<uint16_t> GetHumanoidCandidates(
            const SkeletonRig& rig,
            const RigGraph& graph,
            HumanoidBoneType type
        ) {
            SR_UTILS_NS::Vector<uint16_t> nodes;

            if (type == HumanoidBoneType::Unknown) {
                return nodes;
            }

            const SR_UTILS_NS::StringAtom key = SR_UTILS_NS::EnumReflector::ToStringAtom(type);
            auto&& pChain = rig.GetBoneChain(key);
            if (!pChain) {
                return nodes;
            }

            nodes.reserve(pChain->bones.size());
            for (const auto& boneInfo : pChain->bones) {
                const uint16_t nodeIndex = GetNodeIndexSafe(graph, boneInfo.name);
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

        SR_NODISCARD static uint16_t PickBestByMinDepth(const RigGraph& graph, const SR_UTILS_NS::Vector<uint16_t>& candidates) {
            if (candidates.empty()) {
                return SR_UINT16_MAX;
            }
            uint16_t best = candidates.front();
            uint32_t bestDepth = graph.nodeDepth[best];
            for (const uint16_t ni : candidates) {
                const uint32_t d = graph.nodeDepth[ni];
                if (d < bestDepth) {
                    bestDepth = d;
                    best = ni;
                }
            }
            return best;
        }

        static bool PickDirectTwoBoneChain(
            const RigGraph& graph,
            const SR_UTILS_NS::Vector<uint16_t>& rootCandidates,
            const SR_UTILS_NS::Vector<uint16_t>& midCandidates,
            const SR_UTILS_NS::Vector<uint16_t>& tipCandidates,
            uint16_t& outRoot,
            uint16_t& outMid,
            uint16_t& outTip
        ) {
            outRoot = SR_UINT16_MAX;
            outMid = SR_UINT16_MAX;
            outTip = SR_UINT16_MAX;

            if (rootCandidates.empty() || midCandidates.empty() || tipCandidates.empty()) {
                return false;
            }

            float_t bestScore = -1.f;
            for (const uint16_t mid : midCandidates) {
                if (mid == SR_UINT16_MAX || mid >= graph.nodesCount) {
                    continue;
                }
                const uint16_t root = graph.parentIndex[mid];
                if (root == SR_UINT16_MAX) {
                    continue;
                }
                if (std::find(rootCandidates.begin(), rootCandidates.end(), root) == rootCandidates.end()) {
                    continue;
                }

                for (const uint16_t tip : tipCandidates) {
                    if (tip == SR_UINT16_MAX || tip >= graph.nodesCount) {
                        continue;
                    }
                    if (graph.parentIndex[tip] != mid) {
                        continue;
                    }

                    /// Prefer longer segments (usually the main chain, not twist/end stubs).
                    const float_t l1 = (graph.refCSPos[mid] - graph.refCSPos[root]).Length();
                    const float_t l2 = (graph.refCSPos[tip] - graph.refCSPos[mid]).Length();
                    const float_t score = l1 + l2;
                    if (score > bestScore) {
                        bestScore = score;
                        outRoot = root;
                        outMid = mid;
                        outTip = tip;
                    }
                }
            }

            return outRoot != SR_UINT16_MAX && outMid != SR_UINT16_MAX && outTip != SR_UINT16_MAX;
        }

        SR_NODISCARD static SR_UTILS_NS::Vector<uint16_t> BuildParentPath(
            const RigGraph& graph,
            uint16_t root,
            uint16_t tip,
            uint32_t maxSteps = 256
        ) {
            SR_UTILS_NS::Vector<uint16_t> path;
            if (root == SR_UINT16_MAX || tip == SR_UINT16_MAX || root >= graph.nodesCount || tip >= graph.nodesCount) {
                return path;
            }

            uint16_t cur = tip;
            for (uint32_t step = 0; step < maxSteps; ++step) {
                path.emplace_back(cur);
                if (cur == root) {
                    break;
                }
                const uint16_t p = graph.parentIndex[cur];
                if (p == SR_UINT16_MAX || p == cur || p >= graph.nodesCount) {
                    break;
                }
                cur = p;
            }

            if (path.empty() || path.back() != root) {
                path.clear();
                return path;
            }

            std::reverse(path.begin(), path.end());
            return path;
        }

        static void BuildComponentSpacePose(
            const RigGraph& graph,
            const SR_UTILS_NS::Vector<SR_MATH_NS::FVector3>& localT,
            const SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion>& localR,
            const SR_UTILS_NS::Vector<SR_MATH_NS::FVector3>& localS,
            SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4>& outCS,
            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion>& outCSRot,
            SR_UTILS_NS::Vector<SR_MATH_NS::FVector3>& outCSPos
        ) {
            outCS.resize(graph.nodesCount);
            outCSRot.resize(graph.nodesCount);
            outCSPos.resize(graph.nodesCount);

            SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> localM;
            localM.resize(graph.nodesCount);

            for (uint16_t ni = 0; ni < graph.nodesCount; ++ni) {
                localM[ni] = SR_MATH_NS::Matrix4x4::CreateTRS(localT[ni], localR[ni], localS[ni]);

                const uint16_t parent = graph.parentIndex[ni];
                if (parent != SR_UINT16_MAX && parent < graph.nodesCount && parent < ni) {
                    outCS[ni] = outCS[parent] * localM[ni];
                    outCSRot[ni] = (outCSRot[parent] * localR[ni]).Normalized();
                }
                else {
                    outCS[ni] = localM[ni];
                    outCSRot[ni] = localR[ni].Normalized();
                }

                outCSPos[ni] = outCS[ni].GetTranslate();
            }
        }

        SR_NODISCARD static SR_MATH_NS::FVector3 SolvePelvisFromFeetGoals(
            const SR_MATH_NS::FVector3& hipsRefPos,
            const SR_MATH_NS::FVector3& hipsUpDir,
            const SR_MATH_NS::FVector3& goalLeftFoot,
            const SR_MATH_NS::FVector3& goalRightFoot,
            const SR_MATH_NS::FVector3& hipsToLeftUpperLegOffsetCS,
            const SR_MATH_NS::FVector3& hipsToRightUpperLegOffsetCS,
            float_t leftLegReach,
            float_t rightLegReach
        ) {
            /// We solve for pelvis position p (hips CS) such that:
            /// |(p + offsetUpper) - goalFoot| <= reach
            /// => |p - (goalFoot - offsetUpper)| <= reach
            const SR_MATH_NS::FVector3 cL = goalLeftFoot - hipsToLeftUpperLegOffsetCS;
            const SR_MATH_NS::FVector3 cR = goalRightFoot - hipsToRightUpperLegOffsetCS;

            const SR_MATH_NS::FVector3 diff = cR - cL;
            const float_t d = diff.Length();

            const float_t r1 = SR_MAX(leftLegReach, 0.001f);
            const float_t r2 = SR_MAX(rightLegReach, 0.001f);

            if (d <= 1e-6f) {
                /// Concentric spheres: pick closest point to ref on sphere r1 around cL.
                SR_MATH_NS::FVector3 dir = hipsRefPos - cL;
                const float_t len = dir.Length();
                if (len <= 1e-6f) {
                    dir = hipsUpDir.IsZero() ? SR_MATH_NS::FVector3::Up() : hipsUpDir;
                }
                else {
                    dir /= len;
                }
                return cL + dir * r1;
            }

            const SR_MATH_NS::FVector3 ex = diff / d;

            /// Sphere-sphere intersection circle basis on the line cL->cR.
            float_t a = (r1 * r1 - r2 * r2 + d * d) / (2.f * d);

            /// Clamp 'a' for non-intersecting spheres (gives closest points along the axis).
            a = SR_CLAMP(a, 0.f, d);

            const SR_MATH_NS::FVector3 p0 = cL + ex * a;
            float_t h2 = r1 * r1 - a * a;
            h2 = SR_MAX(h2, 0.f);
            const float_t h = std::sqrt(h2);

            /// Choose a stable perpendicular direction that tends to preserve "up" height.
            SR_MATH_NS::FVector3 up = hipsUpDir;
            if (up.IsZero()) {
                up = SR_MATH_NS::FVector3::Up();
            }

            SR_MATH_NS::FVector3 ey = up - ex * SR_MATH_NS::FVector3::Dot(up, ex);
            const float_t eyLen = ey.Length();
            if (eyLen <= 1e-6f) {
                ey = SR_MATH_NS::GetPerpendicularVector(ex);
            }
            else {
                ey /= eyLen;
            }

            const SR_MATH_NS::FVector3 pA = p0 + ey * h;
            const SR_MATH_NS::FVector3 pB = p0 - ey * h;

            return (pA - hipsRefPos).SqrMagnitude() < (pB - hipsRefPos).SqrMagnitude() ? pA : pB;
        }

        SR_NODISCARD static SR_MATH_NS::FVector3 SafeNormalize(const SR_MATH_NS::FVector3& v, const SR_MATH_NS::FVector3& fallback) {
            const float_t len = v.Length();
            if (len <= 1e-6f) {
                return fallback;
            }
            return v / len;
        }

        SR_NODISCARD static SR_MATH_NS::FVector3 GetLocalBoneDirSafe(
            const RigGraph& graph,
            uint16_t parentNode,
            uint16_t childNode
        ) {
            /// In this engine hierarchy, the child's local translation encodes the segment direction/length.
            if (childNode == SR_UINT16_MAX || parentNode == SR_UINT16_MAX || childNode >= graph.nodesCount || parentNode >= graph.nodesCount) {
                return SR_MATH_NS::FVector3::Forward();
            }

            const SR_MATH_NS::FVector3 t = graph.refLocalT[childNode];
            if (t.Length() > 1e-6f) {
                return t.Normalized();
            }

            /// Fallback: derive direction from reference component-space positions, expressed in parent CS frame.
            const SR_MATH_NS::FVector3 vCS = graph.refCSPos[childNode] - graph.refCSPos[parentNode];
            if (vCS.Length() <= 1e-6f) {
                return SR_MATH_NS::FVector3::Forward();
            }

            const SR_MATH_NS::Quaternion parentRefCSRot = graph.refCSRot[parentNode];
            return (parentRefCSRot.Inverse() * vCS).Normalized();
        }

        static bool SolveTwoBoneIK(
            const RigGraph& graph,
            uint16_t rootNode,
            uint16_t midNode,
            uint16_t tipNode,
            const SR_MATH_NS::FVector3& goalTipPosCS,
            const SR_MATH_NS::FVector3& bendNormalCS,
            SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion>& ioLocalR,
            const SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion>& csRot,
            const SR_UTILS_NS::Vector<SR_MATH_NS::FVector3>& csPos
        ) {
            if (rootNode == SR_UINT16_MAX || midNode == SR_UINT16_MAX || tipNode == SR_UINT16_MAX) {
                return false;
            }
            if (rootNode >= graph.nodesCount || midNode >= graph.nodesCount || tipNode >= graph.nodesCount) {
                return false;
            }

            if (graph.parentIndex[midNode] != rootNode || graph.parentIndex[tipNode] != midNode) {
                return false;
            }

            const SR_MATH_NS::FVector3 rootPos = csPos[rootNode];
            const SR_MATH_NS::FVector3 midPosRef = graph.refCSPos[midNode];
            const SR_MATH_NS::FVector3 rootPosRef = graph.refCSPos[rootNode];
            const SR_MATH_NS::FVector3 tipPosRef = graph.refCSPos[tipNode];

            const float_t l1 = (midPosRef - rootPosRef).Length();
            const float_t l2 = (tipPosRef - midPosRef).Length();
            if (l1 <= 1e-6f || l2 <= 1e-6f) {
                return false;
            }

            SR_MATH_NS::FVector3 rootToGoal = goalTipPosCS - rootPos;
            float_t d = rootToGoal.Length();

            /// Clamp unreachable targets for stability (fully extended / fully retracted).
            const float_t maxReach = l1 + l2 - 1e-4f;
            const float_t minReach = SR_MATH_NS::Abs(l1 - l2) + 1e-4f;
            d = SR_CLAMP(d, minReach, maxReach);
            const SR_MATH_NS::FVector3 ex = SafeNormalize(rootToGoal, SR_MATH_NS::FVector3::Forward());

            SR_MATH_NS::FVector3 n = bendNormalCS;
            if (n.Length() <= 1e-6f) {
                n = SR_MATH_NS::GetPerpendicularVector(ex);
            }
            n = n.Normalized();

            SR_MATH_NS::FVector3 ey = n - ex * SR_MATH_NS::FVector3::Dot(n, ex);
            if (ey.Length() <= 1e-6f) {
                ey = SR_MATH_NS::GetPerpendicularVector(ex);
            }
            ey = ey.Normalized();

            const float_t cosRootAngle = SR_CLAMP((l1 * l1 + d * d - l2 * l2) / (2.f * l1 * d), -1.f, 1.f);
            const float_t rootAngle = SR_ACOS(cosRootAngle);

            const float_t along = l1 * SR_COS(rootAngle);
            const float_t perp = l1 * SR_SIN(rootAngle);

            const SR_MATH_NS::FVector3 midPos = rootPos + ex * along + ey * perp;
            const SR_MATH_NS::FVector3 desiredRootDirCS = SafeNormalize(midPos - rootPos, ex);

            const SR_MATH_NS::Quaternion rootCSRotCurrent = csRot[rootNode];
            const SR_MATH_NS::FVector3 rootToMidLocalRef = GetLocalBoneDirSafe(graph, rootNode, midNode);
            const SR_MATH_NS::FVector3 rootForwardCS = rootCSRotCurrent * rootToMidLocalRef;
            const SR_MATH_NS::Quaternion qRoot = SR_MATH_NS::Quaternion::FromToRotation(rootForwardCS, desiredRootDirCS);
            const SR_MATH_NS::Quaternion rootCSRotNew = (qRoot * rootCSRotCurrent).Normalized();

            const uint16_t rootParent = graph.parentIndex[rootNode];
            const SR_MATH_NS::Quaternion parentCSRot = (rootParent != SR_UINT16_MAX && rootParent < graph.nodesCount) ? csRot[rootParent] : SR_MATH_NS::Quaternion::Identity();
            ioLocalR[rootNode] = (parentCSRot.Inverse() * rootCSRotNew).Normalized();

            /// Mid rotation
            const SR_MATH_NS::Quaternion midCSRotCurrent = (rootCSRotNew * ioLocalR[midNode]).Normalized();
            const SR_MATH_NS::FVector3 midToTipLocalRef = GetLocalBoneDirSafe(graph, midNode, tipNode);
            /// Use actual mid position implied by the new root rotation + local translation for stability.
            const SR_MATH_NS::FVector3 midPosActual = rootPos + (rootCSRotNew * graph.refLocalT[midNode]);
            const SR_MATH_NS::FVector3 desiredMidDirCS = SafeNormalize(goalTipPosCS - midPosActual, ex);
            const SR_MATH_NS::FVector3 midForwardCS = midCSRotCurrent * midToTipLocalRef;
            const SR_MATH_NS::Quaternion qMid = SR_MATH_NS::Quaternion::FromToRotation(midForwardCS, desiredMidDirCS);
            const SR_MATH_NS::Quaternion midCSRotNew = (qMid * midCSRotCurrent).Normalized();

            ioLocalR[midNode] = (rootCSRotNew.Inverse() * midCSRotNew).Normalized();

            return true;
        }
    }

    SR_NODISCARD bool IKRetargetAnimation::Retarget(
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

        /// Candidate lists (avoid picking twist bones by accident)
        const auto srcHipsCandidates = GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::Hips);
        const auto tgtHipsCandidates = GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::Hips);

        const uint16_t srcHipsNode = PickBestByMinDepth(srcGraph, srcHipsCandidates);
        const uint16_t tgtHipsNode = PickBestByMinDepth(tgtGraph, tgtHipsCandidates);
        if (srcHipsNode == SR_UINT16_MAX || tgtHipsNode == SR_UINT16_MAX) {
            return false;
        }

        const uint16_t srcHeadNode = PickBestByMinDepth(srcGraph, GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::Head));
        const uint16_t tgtHeadNode = PickBestByMinDepth(tgtGraph, GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::Head));

        const SR_MATH_NS::Quaternion hipsBasisRot = (tgtGraph.refCSRot[tgtHipsNode] * srcGraph.refCSRot[srcHipsNode].Inverse()).Normalized();

        float_t globalScale = 1.f;
        if (srcHeadNode != SR_UINT16_MAX && tgtHeadNode != SR_UINT16_MAX) {
            const float_t srcLen = (srcGraph.refCSPos[srcHeadNode] - srcGraph.refCSPos[srcHipsNode]).Length();
            const float_t tgtLen = (tgtGraph.refCSPos[tgtHeadNode] - tgtGraph.refCSPos[tgtHipsNode]).Length();
            if (srcLen > 1e-6f && tgtLen > 1e-6f) {
                globalScale = tgtLen / srcLen;
            }
        }

        /// Pick direct limb chains (root-mid-tip) on source/target.
        uint16_t srcLLegRoot = SR_UINT16_MAX, srcLLegMid = SR_UINT16_MAX, srcLLegTip = SR_UINT16_MAX;
        uint16_t tgtLLegRoot = SR_UINT16_MAX, tgtLLegMid = SR_UINT16_MAX, tgtLLegTip = SR_UINT16_MAX;
        uint16_t srcRLegRoot = SR_UINT16_MAX, srcRLegMid = SR_UINT16_MAX, srcRLegTip = SR_UINT16_MAX;
        uint16_t tgtRLegRoot = SR_UINT16_MAX, tgtRLegMid = SR_UINT16_MAX, tgtRLegTip = SR_UINT16_MAX;

        uint16_t srcLArmRoot = SR_UINT16_MAX, srcLArmMid = SR_UINT16_MAX, srcLArmTip = SR_UINT16_MAX;
        uint16_t tgtLArmRoot = SR_UINT16_MAX, tgtLArmMid = SR_UINT16_MAX, tgtLArmTip = SR_UINT16_MAX;
        uint16_t srcRArmRoot = SR_UINT16_MAX, srcRArmMid = SR_UINT16_MAX, srcRArmTip = SR_UINT16_MAX;
        uint16_t tgtRArmRoot = SR_UINT16_MAX, tgtRArmMid = SR_UINT16_MAX, tgtRArmTip = SR_UINT16_MAX;

        const bool hasLLeg = PickDirectTwoBoneChain(
            srcGraph,
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::LeftUpperLeg),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::LeftLowerLeg),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::LeftFoot),
            srcLLegRoot, srcLLegMid, srcLLegTip
        ) && PickDirectTwoBoneChain(
            tgtGraph,
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::LeftUpperLeg),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::LeftLowerLeg),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::LeftFoot),
            tgtLLegRoot, tgtLLegMid, tgtLLegTip
        );

        const bool hasRLeg = PickDirectTwoBoneChain(
            srcGraph,
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::RightUpperLeg),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::RightLowerLeg),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::RightFoot),
            srcRLegRoot, srcRLegMid, srcRLegTip
        ) && PickDirectTwoBoneChain(
            tgtGraph,
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::RightUpperLeg),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::RightLowerLeg),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::RightFoot),
            tgtRLegRoot, tgtRLegMid, tgtRLegTip
        );

        const bool hasLArm = PickDirectTwoBoneChain(
            srcGraph,
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::LeftUpperArm),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::LeftLowerArm),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::LeftHand),
            srcLArmRoot, srcLArmMid, srcLArmTip
        ) && PickDirectTwoBoneChain(
            tgtGraph,
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::LeftUpperArm),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::LeftLowerArm),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::LeftHand),
            tgtLArmRoot, tgtLArmMid, tgtLArmTip
        );

        const bool hasRArm = PickDirectTwoBoneChain(
            srcGraph,
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::RightUpperArm),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::RightLowerArm),
            GetHumanoidCandidates(sourceRig, srcGraph, HumanoidBoneType::RightHand),
            srcRArmRoot, srcRArmMid, srcRArmTip
        ) && PickDirectTwoBoneChain(
            tgtGraph,
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::RightUpperArm),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::RightLowerArm),
            GetHumanoidCandidates(targetRig, tgtGraph, HumanoidBoneType::RightHand),
            tgtRArmRoot, tgtRArmMid, tgtRArmTip
        );

        const auto ChainScaleFromRef = [&](uint16_t sRoot, uint16_t sTip, uint16_t tRoot, uint16_t tTip) -> float_t {
            if (sRoot == SR_UINT16_MAX || sTip == SR_UINT16_MAX || tRoot == SR_UINT16_MAX || tTip == SR_UINT16_MAX) {
                return globalScale;
            }
            const float_t srcLen = (srcGraph.refCSPos[sTip] - srcGraph.refCSPos[sRoot]).Length();
            const float_t tgtLen = (tgtGraph.refCSPos[tTip] - tgtGraph.refCSPos[tRoot]).Length();
            if (srcLen <= 1e-6f || tgtLen <= 1e-6f) {
                return globalScale;
            }
            return tgtLen / srcLen;
        };

        const float_t leftLegScale  = hasLLeg ? ChainScaleFromRef(srcLLegRoot, srcLLegTip, tgtLLegRoot, tgtLLegTip) : globalScale;
        const float_t rightLegScale = hasRLeg ? ChainScaleFromRef(srcRLegRoot, srcRLegTip, tgtRLegRoot, tgtRLegTip) : globalScale;
        const float_t leftArmScale  = hasLArm ? ChainScaleFromRef(srcLArmRoot, srcLArmTip, tgtLArmRoot, tgtLArmTip) : globalScale;
        const float_t rightArmScale = hasRArm ? ChainScaleFromRef(srcRArmRoot, srcRArmTip, tgtRArmRoot, tgtRArmTip) : globalScale;

        const float_t duration = ComputeChannelsDuration(sourceChannels);

        static constexpr float_t bakeFps = 60.f;
        static constexpr float_t bakeDt = 1.f / bakeFps;
        const uint32_t samplesCount = duration > 0.f ? (static_cast<uint32_t>(std::ceil(duration * bakeFps)) + 1u) : 1u;

        if (tgtGraph.orderedBones.empty()) {
            return false;
        }

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

        /// --- per-frame scratch (source component-space) ---
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> srcCS;
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> srcCSRot;
        SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> srcCSPos;

        /// --- per-frame scratch (target local pose we will bake) ---
        SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> tgtLocalT;
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> tgtLocalR;
        SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> tgtLocalS;
        tgtLocalT.resize(tgtGraph.nodesCount);
        tgtLocalR.resize(tgtGraph.nodesCount);
        tgtLocalS.resize(tgtGraph.nodesCount);

        /// Target component-space scratch (will be rebuilt after IK)
        SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> tgtCS;
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> tgtCSRot;
        SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> tgtCSPos;

        /// Quaternion continuity (per target bone)
        SR_UTILS_NS::Vector<SR_MATH_NS::Quaternion> prevRot;
        SR_UTILS_NS::Vector<bool> hasPrevRot;
        prevRot.resize(tgtGraph.orderedBones.size(), SR_MATH_NS::Quaternion::Identity());
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

            /// 2) Build source component-space pose (CS matrices + CS rotation + CS pos)
            BuildComponentSpacePose(srcGraph, srcLocalT, srcLocalR, srcLocalS, srcCS, srcCSRot, srcCSPos);

            /// 3) Start from target reference pose for this frame
            for (uint16_t ni = 0; ni < tgtGraph.nodesCount; ++ni) {
                tgtLocalT[ni] = tgtGraph.refLocalT[ni];
                tgtLocalR[ni] = tgtGraph.refLocalR[ni];
                tgtLocalS[ni] = tgtGraph.refLocalS[ni];
            }

            /// 4) Retarget hips rotation (component-space delta mapped into target frame)
            {
                const SR_MATH_NS::Quaternion srcDeltaHips = (srcCSRot[srcHipsNode] * srcGraph.refCSRot[srcHipsNode].Inverse()).Normalized();
                const SR_MATH_NS::Quaternion deltaTgt = (hipsBasisRot * srcDeltaHips * hipsBasisRot.Inverse()).Normalized();
                const SR_MATH_NS::Quaternion desiredTgtHipsCS = (deltaTgt * tgtGraph.refCSRot[tgtHipsNode]).Normalized();

                const uint16_t hipsParent = tgtGraph.parentIndex[tgtHipsNode];
                const SR_MATH_NS::Quaternion parentRefCSRot = (hipsParent != SR_UINT16_MAX && hipsParent < tgtGraph.nodesCount)
                    ? tgtGraph.refCSRot[hipsParent]
                    : SR_MATH_NS::Quaternion::Identity();

                tgtLocalR[tgtHipsNode] = (parentRefCSRot.Inverse() * desiredTgtHipsCS).Normalized();
            }

            /// Build target CS now (hips rotation is needed for correct goal-space mapping)
            BuildComponentSpacePose(tgtGraph, tgtLocalT, tgtLocalR, tgtLocalS, tgtCS, tgtCSRot, tgtCSPos);

            /// 5) Build goals in target space (absolute, anchored at CURRENT hips pose)
            const SR_MATH_NS::FVector3 srcHipsPos = srcCSPos[srcHipsNode];
            const SR_MATH_NS::FVector3 tgtHipsPos = tgtCSPos[tgtHipsNode];
            const SR_MATH_NS::Quaternion basisRotFrame = (tgtCSRot[tgtHipsNode] * srcCSRot[srcHipsNode].Inverse()).Normalized();

            auto&& MapPointToTargetSpaceFrame = [&](const SR_MATH_NS::FVector3& srcCSPoint, float_t scale) -> SR_MATH_NS::FVector3 {
                return tgtHipsPos + basisRotFrame * ((srcCSPoint - srcHipsPos) * scale);
            };

            SR_MATH_NS::FVector3 goalHeadPos = SR_MATH_NS::FVector3::Zero();
            bool hasGoalHead = false;
            if (srcHeadNode != SR_UINT16_MAX) {
                goalHeadPos = MapPointToTargetSpaceFrame(srcCSPos[srcHeadNode], globalScale);
                hasGoalHead = true;
            }

            SR_MATH_NS::FVector3 goalLeftFoot = SR_MATH_NS::FVector3::Zero();
            SR_MATH_NS::FVector3 goalRightFoot = SR_MATH_NS::FVector3::Zero();
            bool hasLeftFoot = false;
            bool hasRightFoot = false;

            SR_MATH_NS::FVector3 goalLeftHand = SR_MATH_NS::FVector3::Zero();
            SR_MATH_NS::FVector3 goalRightHand = SR_MATH_NS::FVector3::Zero();
            bool hasLeftHand = false;
            bool hasRightHand = false;

            if (hasLLeg) {
                goalLeftFoot = MapPointToTargetSpaceFrame(srcCSPos[srcLLegTip], leftLegScale);
                hasLeftFoot = true;
            }
            if (hasRLeg) {
                goalRightFoot = MapPointToTargetSpaceFrame(srcCSPos[srcRLegTip], rightLegScale);
                hasRightFoot = true;
            }

            if (hasLArm) {
                goalLeftHand = MapPointToTargetSpaceFrame(srcCSPos[srcLArmTip], leftArmScale);
                hasLeftHand = true;
            }
            if (hasRArm) {
                goalRightHand = MapPointToTargetSpaceFrame(srcCSPos[srcRArmTip], rightArmScale);
                hasRightHand = true;
            }

            /// 6) Solve pelvis translation from feet goals (if both DIRECT leg chains exist)
            if (hasLeftFoot && hasRightFoot && hasLLeg && hasRLeg)
            {
                const float_t leftLegReach = (tgtGraph.refCSPos[tgtLLegMid] - tgtGraph.refCSPos[tgtLLegRoot]).Length()
                                           + (tgtGraph.refCSPos[tgtLLegTip] - tgtGraph.refCSPos[tgtLLegMid]).Length();

                const float_t rightLegReach = (tgtGraph.refCSPos[tgtRLegMid] - tgtGraph.refCSPos[tgtRLegRoot]).Length()
                                            + (tgtGraph.refCSPos[tgtRLegTip] - tgtGraph.refCSPos[tgtRLegMid]).Length();

                const SR_MATH_NS::FVector3 hipsRefPos = tgtCSPos[tgtHipsNode];
                const SR_MATH_NS::FVector3 hipsUpDir = tgtCS[tgtHipsNode].Up();

                const SR_MATH_NS::FVector3 hipsToLUpper = tgtCSPos[tgtLLegRoot] - hipsRefPos;
                const SR_MATH_NS::FVector3 hipsToRUpper = tgtCSPos[tgtRLegRoot] - hipsRefPos;

                const SR_MATH_NS::FVector3 solvedHipsCSPos = SolvePelvisFromFeetGoals(
                    hipsRefPos,
                    hipsUpDir,
                    goalLeftFoot,
                    goalRightFoot,
                    hipsToLUpper,
                    hipsToRUpper,
                    leftLegReach,
                    rightLegReach
                );

                const uint16_t hipsParent = tgtGraph.parentIndex[tgtHipsNode];
                if (hipsParent != SR_UINT16_MAX && hipsParent < tgtGraph.nodesCount) {
                    const SR_MATH_NS::Matrix4x4 parentCS = tgtCS[hipsParent];
                    tgtLocalT[tgtHipsNode] = parentCS.Inverse().TransformPoint(solvedHipsCSPos).XYZ();
                }
                else {
                    tgtLocalT[tgtHipsNode] = solvedHipsCSPos;
                }
            }

            /// Build target CS after pelvis translation (IK passes will further modify rotations)
            BuildComponentSpacePose(tgtGraph, tgtLocalT, tgtLocalR, tgtLocalS, tgtCS, tgtCSRot, tgtCSPos);

            /// 6) Legs IK (two-bone) to match foot goals
            if (hasLeftFoot && hasLLeg) {
                SR_MATH_NS::FVector3 bend = SR_MATH_NS::FVector3::Zero();
                {
                    const SR_MATH_NS::FVector3 a = srcCSPos[srcLLegMid] - srcCSPos[srcLLegRoot];
                    const SR_MATH_NS::FVector3 b = srcCSPos[srcLLegTip] - srcCSPos[srcLLegMid];
                    bend = SR_MATH_NS::FVector3::Cross(a, b);
                }
                bend = basisRotFrame * bend;
                if (bend.Length() <= 1e-6f) {
                    bend = SR_MATH_NS::GetPerpendicularVector(SafeNormalize(goalLeftFoot - tgtCSPos[tgtLLegRoot], SR_MATH_NS::FVector3::Forward()));
                }

                SolveTwoBoneIK(tgtGraph, tgtLLegRoot, tgtLLegMid, tgtLLegTip, goalLeftFoot, bend, tgtLocalR, tgtCSRot, tgtCSPos);
            }

            if (hasRightFoot && hasRLeg) {
                SR_MATH_NS::FVector3 bend = SR_MATH_NS::FVector3::Zero();
                {
                    const SR_MATH_NS::FVector3 a = srcCSPos[srcRLegMid] - srcCSPos[srcRLegRoot];
                    const SR_MATH_NS::FVector3 b = srcCSPos[srcRLegTip] - srcCSPos[srcRLegMid];
                    bend = SR_MATH_NS::FVector3::Cross(a, b);
                }
                bend = basisRotFrame * bend;
                if (bend.Length() <= 1e-6f) {
                    bend = SR_MATH_NS::GetPerpendicularVector(SafeNormalize(goalRightFoot - tgtCSPos[tgtRLegRoot], SR_MATH_NS::FVector3::Forward()));
                }

                SolveTwoBoneIK(tgtGraph, tgtRLegRoot, tgtRLegMid, tgtRLegTip, goalRightFoot, bend, tgtLocalR, tgtCSRot, tgtCSPos);
            }

            /// Rebuild target CS after legs IK (spine/arms will use updated pelvis+legs pose)
            BuildComponentSpacePose(tgtGraph, tgtLocalT, tgtLocalR, tgtLocalS, tgtCS, tgtCSRot, tgtCSPos);

            /// 7) Spine/neck/head IK (FABRIK-like positions, then convert to rotations)
            if (hasGoalHead && tgtHeadNode != SR_UINT16_MAX) {
                /// Use the actual hierarchy path from hips to head for stability (works on self-retarget too).
                auto contiguous = BuildParentPath(tgtGraph, tgtHipsNode, tgtHeadNode);

                /// Skip hips itself to avoid twisting pelvis during spine solve.
                if (!contiguous.empty() && contiguous.front() == tgtHipsNode) {
                    contiguous.erase(contiguous.begin());
                }

                if (contiguous.size() >= 2 && contiguous.back() == tgtHeadNode) {
                    const uint32_t n = static_cast<uint32_t>(contiguous.size());

                    SR_UTILS_NS::Vector<float_t> segLen;
                    SR_UTILS_NS::Vector<SR_MATH_NS::FVector3> pos;
                    segLen.resize(n - 1);
                    pos.resize(n);

                    for (uint32_t i = 0; i < n; ++i) {
                        pos[i] = tgtCSPos[contiguous[i]];
                    }
                    for (uint32_t i = 0; i + 1 < n; ++i) {
                        segLen[i] = (tgtGraph.refCSPos[contiguous[i + 1]] - tgtGraph.refCSPos[contiguous[i]]).Length();
                        segLen[i] = SR_MAX(segLen[i], 0.001f);
                    }

                    const SR_MATH_NS::FVector3 rootFixed = pos.front();

                    static constexpr uint32_t kSpineIterations = 8;
                    for (uint32_t iter = 0; iter < kSpineIterations; ++iter) {
                        /// Forward: move head to goal, propagate to root
                        pos[n - 1] = goalHeadPos;
                        for (int32_t i = static_cast<int32_t>(n) - 2; i >= 0; --i) {
                            const SR_MATH_NS::FVector3 dir = SafeNormalize(pos[i] - pos[i + 1], SR_MATH_NS::FVector3::Forward());
                            pos[i] = pos[i + 1] + dir * segLen[i];
                        }

                        /// Backward: fix root, propagate to head
                        pos[0] = rootFixed;
                        for (uint32_t i = 1; i < n; ++i) {
                            const SR_MATH_NS::FVector3 dir = SafeNormalize(pos[i] - pos[i - 1], SR_MATH_NS::FVector3::Forward());
                            pos[i] = pos[i - 1] + dir * segLen[i - 1];
                        }
                    }

                    /// Convert FABRIK positions to rotations (top-down along the chain)
                    for (uint32_t i = 0; i + 1 < n; ++i) {
                        const uint16_t parentNode = contiguous[i];
                        const uint16_t childNode = contiguous[i + 1];

                        const SR_MATH_NS::FVector3 desiredDirCS = SafeNormalize(pos[i + 1] - pos[i], SR_MATH_NS::FVector3::Forward());

                        const SR_MATH_NS::Quaternion parentCSRotCurrent = tgtCSRot[parentNode];
                        const SR_MATH_NS::FVector3 parentToChildLocalRef = GetLocalBoneDirSafe(tgtGraph, parentNode, childNode);
                        const SR_MATH_NS::FVector3 parentForwardCS = parentCSRotCurrent * parentToChildLocalRef;

                        const SR_MATH_NS::Quaternion q = SR_MATH_NS::Quaternion::FromToRotation(parentForwardCS, desiredDirCS);
                        const SR_MATH_NS::Quaternion parentCSRotNew = (q * parentCSRotCurrent).Normalized();

                        const uint16_t p = tgtGraph.parentIndex[parentNode];
                        const SR_MATH_NS::Quaternion parentParentCSRot = (p != SR_UINT16_MAX && p < tgtGraph.nodesCount) ? tgtCSRot[p] : SR_MATH_NS::Quaternion::Identity();
                        tgtLocalR[parentNode] = (parentParentCSRot.Inverse() * parentCSRotNew).Normalized();

                        /// Update chain CS rotations/positions for subsequent joints
                        BuildComponentSpacePose(tgtGraph, tgtLocalT, tgtLocalR, tgtLocalS, tgtCS, tgtCSRot, tgtCSPos);
                    }
                }
            }

            /// Rebuild target CS after spine IK (arms will use updated shoulders/chest pose)
            BuildComponentSpacePose(tgtGraph, tgtLocalT, tgtLocalR, tgtLocalS, tgtCS, tgtCSRot, tgtCSPos);

            /// 8) Arms IK (two-bone) to match hand goals
            if (hasLeftHand && hasLArm) {
                SR_MATH_NS::FVector3 bend = SR_MATH_NS::FVector3::Zero();
                {
                    const SR_MATH_NS::FVector3 a = srcCSPos[srcLArmMid] - srcCSPos[srcLArmRoot];
                    const SR_MATH_NS::FVector3 b = srcCSPos[srcLArmTip] - srcCSPos[srcLArmMid];
                    bend = SR_MATH_NS::FVector3::Cross(a, b);
                }
                bend = basisRotFrame * bend;
                if (bend.Length() <= 1e-6f) {
                    bend = SR_MATH_NS::GetPerpendicularVector(SafeNormalize(goalLeftHand - tgtCSPos[tgtLArmRoot], SR_MATH_NS::FVector3::Forward()));
                }

                SolveTwoBoneIK(tgtGraph, tgtLArmRoot, tgtLArmMid, tgtLArmTip, goalLeftHand, bend, tgtLocalR, tgtCSRot, tgtCSPos);
            }

            if (hasRightHand && hasRArm) {
                SR_MATH_NS::FVector3 bend = SR_MATH_NS::FVector3::Zero();
                {
                    const SR_MATH_NS::FVector3 a = srcCSPos[srcRArmMid] - srcCSPos[srcRArmRoot];
                    const SR_MATH_NS::FVector3 b = srcCSPos[srcRArmTip] - srcCSPos[srcRArmMid];
                    bend = SR_MATH_NS::FVector3::Cross(a, b);
                }
                bend = basisRotFrame * bend;
                if (bend.Length() <= 1e-6f) {
                    bend = SR_MATH_NS::GetPerpendicularVector(SafeNormalize(goalRightHand - tgtCSPos[tgtRArmRoot], SR_MATH_NS::FVector3::Forward()));
                }

                SolveTwoBoneIK(tgtGraph, tgtRArmRoot, tgtRArmMid, tgtRArmTip, goalRightHand, bend, tgtLocalR, tgtCSRot, tgtCSPos);
            }

            /// Rebuild target CS after arms IK (bake will use the final local pose)
            BuildComponentSpacePose(tgtGraph, tgtLocalT, tgtLocalR, tgtLocalS, tgtCS, tgtCSRot, tgtCSPos);

            /// 9) Bake local pose for each target bone, emit keys
            for (uint32_t bi = 0; bi < tgtGraph.orderedBones.size(); ++bi) {
                const auto& bone = tgtGraph.orderedBones[bi];
                const uint16_t nodeIndex = bone.nodeIndex;
                if (nodeIndex == SR_UINT16_MAX || nodeIndex >= tgtGraph.nodesCount) {
                    continue;
                }

                SR_MATH_NS::FVector3 outT = tgtLocalT[nodeIndex];
                SR_MATH_NS::Quaternion outR = tgtLocalR[nodeIndex].Normalized();
                SR_MATH_NS::FVector3 outS = tgtLocalS[nodeIndex];

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