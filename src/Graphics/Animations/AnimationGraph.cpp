//
// Created by Monika on 05.05.2023.
//

#include <Graphics/Animations/AnimationGraph.h>
#include <Graphics/Animations/Animator.h>

#include <Utils/ECS/Transform.h>
#include <Utils/ECS/GameObject.h>

#include <Codegen/AnimationGraph.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationGraph::AnimationGraph()
        : Super()
    {
        m_nodes.emplace_back(SRNew<AnimationGraphNodeFinal>());
    }

    AnimationGraph::~AnimationGraph() {
        m_nodes.clear();
        SetAsset(nullptr);
    }

    uint64_t AnimationGraph::GetNodeIndex(const AnimationGraphNode* pNode) const {
        for (uint32_t i = 0; i < m_nodes.size(); ++i) {
            if (m_nodes[i] == pNode) {
                return i;
            }
        }

        SRHalt("Node not found!");
        return SR_ID_INVALID;
    }

    AnimationGraphNode* AnimationGraph::GetFinal() const {
        if (m_nodes.empty()) {
            SRHaltOnce("AnimationGraph::GetFinal() : no nodes in graph!");
            return nullptr;
        }
        if (m_nodes.front()->GetMeta() != AnimationGraphNodeFinal::GetMetaStatic()) {
            SRHaltOnce("AnimationGraph::GetFinal() : first node is not final!");
            return nullptr;
        }
        return const_cast<AnimationGraphNode*>(m_nodes.front().Get());
    }

    bool AnimationGraph::IsStateActive(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        for (auto&& pNode : m_nodes) {
            if (pNode->IsStateActive(name)) {
                return true;
            }
        }

        return false;
    }

    void AnimationGraph::Update(UpdateContext& context) {
        SR_TRACY_ZONE;

        Compile();

        if (!m_isCompiled) {
            SR_WARN("AnimationGraph::Update() : graph is not compiled!");
            return;
        }

        if (m_nodes.empty()) {
            return;
        }

        context.pGraph = this;

        auto&& pAnimationPose = GetFinal()->Update(context, AnimationLink(SR_ID_INVALID, SR_ID_INVALID));
        if (!pAnimationPose) {
            return;
        }

        Apply(pAnimationPose);
    }

    void AnimationGraph::Apply(AnimationPose* pPose) {
        SR_TRACY_ZONE;

        //const bool isRetargeted = Retarget(pPose);
        //std::vector<AnimationGameObjectData>& gameObjectsData = isRetargeted ? m_gameObjectsCache : pPose->GetGameObjects();
        std::vector<AnimationGameObjectData>& gameObjectsData = pPose->GetGameObjects();

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

            m_gameObjects[i]->GetTransform()->SetMatrix(
                data.translation,
                data.rotation,
                data.scaling
            );
        }
    }

    void AnimationGraph::Compile() {
        SR_TRACY_ZONE;

        if (m_isCompiled) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (!m_pAnimator) SR_UNLIKELY_ATTRIBUTE {
            SR_WARN("AnimationGraph::Compile() : animator is nullptr!");
            return;
        }

        m_gameObjects.clear();

        auto&& compileContext = CompileContext(m_gameObjects);

        if (auto&& pSkeleton = m_pAnimator->GetSkeleton().Get()) {
            compileContext.pSkeleton = m_pAnimator->GetSkeleton().Get();
            compileContext.pRig = pSkeleton->GetRig();
        }

        for (auto&& pNode : m_nodes) {
            pNode->Compile(compileContext);
        }

        SR_DEBUG_LOG(SR_FORMAT("AnimationGraph::Compile() : game objects count = {}", m_gameObjects.size()));

        m_isCompiled = true;
    }

    AnimationGraphNode* AnimationGraph::GetNode(uint64_t index) const {
        if (index < m_nodes.size()) {
            return const_cast<AnimationGraphNode*>(m_nodes.at(index).Get());
        }

        SRHaltOnce("Out of range!");

        return nullptr;
    }

    void AnimationGraph::SetAsset(AnimationGraphAsset* pAsset) {
        if (m_pAsset) {
            m_pAsset->RemoveUsePoint();
        }
        m_pAsset = pAsset;
        if (m_pAsset) {
            m_pAsset->AddUsePoint();
        }
    }

    void AnimationGraph::OnPostLoad() {
        Super::OnPostLoad();

        if (m_nodes.empty() || m_nodes.front()->GetMeta() != AnimationGraphNodeFinal::GetMetaStatic()) {
            SR_ERROR("AnimationGraph::OnPostLoad() : broken graph! Final node is nullptr! Trying to fix...");
            RemoveNodes(AnimationGraphNodeFinal::GetMetaStatic()->GetFactoryName());
            m_nodes.insert(m_nodes.begin(), SRNew<AnimationGraphNodeFinal>());
        }

        for (auto&& pNode : m_nodes) {
            pNode->SetGraph(this);
        }
    }

    void AnimationGraph::SetSimpleClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip) {
        SR_TRACY_ZONE;

        if (m_nodes.empty()) {
            m_nodes.emplace_back(SRNew<AnimationGraphNodeFinal>());
        }

        if (m_nodes.size() < 2) {
            m_nodes.emplace_back(SRNew<AnimationGraphNodeStateMachine>());
        }

        if (m_nodes[1]->GetMeta() != AnimationGraphNodeStateMachine::GetMetaStatic()) {
            m_nodes[1] = SRNew<AnimationGraphNodeStateMachine>();
        }

        m_nodes.resize(2);

        m_nodes[0]->SetGraph(this);
        m_nodes[1]->SetGraph(this);

        if (m_nodes[1].StaticCast<AnimationGraphNodeStateMachine>()->SetSimpleClip(pClip)) {
            m_isCompiled = false;
        }

        m_nodes[0]->ClearInputPins();
        m_nodes[0]->ClearOutputPins();

        m_nodes[1]->ClearInputPins();
        m_nodes[1]->ClearOutputPins();

        m_nodes[0]->AddInputPin(AnimationLink(1, 0));
    }

    void AnimationGraph::CloneTo(SR_UTILS_NS::SRClass& clone) const {
        Super::CloneTo(clone);
        for (auto&& pNode : static_cast<AnimationGraph&>(clone).m_nodes) {
            pNode->SetGraph(&static_cast<AnimationGraph&>(clone));
        }
        static_cast<AnimationGraph&>(clone).SetAsset(m_pAsset);
    }

    const SR_UTILS_NS::Path& AnimationGraph::GetPath() const noexcept {
        if (m_pAsset) {
            return m_pAsset->GetResourcePath();
        }
        SRHalt("AnimationGraph::GetPath() : asset is nullptr!");
        static SR_UTILS_NS::Path emptyPath;
        return emptyPath;
    }

    bool AnimationGraph::RemoveNode(uint64_t index) {
        if (index >= m_nodes.size()) {
            SRHalt("AnimationGraph::RemoveNode() : index out of range!");
            return true;
        }

        if (GetFinal() == m_nodes[index].Get()) {
            return false;
        }

        m_nodes.erase(m_nodes.begin() + index);
        m_isCompiled = false;
        return true;
    }

    bool AnimationGraph::RemoveNode(AnimationGraphNode* pNode) {
        if (!SRVerify(pNode)) {
            return false;
        }
        return RemoveNode(GetNodeIndex(pNode));
    }

    bool AnimationGraph::RemoveNodes(SR_UTILS_NS::StringAtom name) {
        SR_UTILS_NS::SmallVector<AnimationGraphNode*, 16> nodesToRemove;
        for (auto&& pNode : m_nodes) {
            if (pNode->GetMeta()->GetFactoryName() == name) {
                nodesToRemove.emplace_back(pNode.Get());
            }
        }
        for (auto&& pNode : nodesToRemove) {
            RemoveNode(pNode);
        }
        return !nodesToRemove.empty();
    }

    const AnimationGraph& AnimationGraphAsset::GetData() const noexcept {
        if (!m_data) {
            m_data = new AnimationGraph();
        }
        return *m_data;
    }

    AnimationGraph& AnimationGraphAsset::GetDataMutable() const noexcept {
        if (!m_data) {
            m_data = new AnimationGraph();
        }
        return *m_data;
    }

    AnimationGraphAsset::AnimationGraphAsset() {
        m_data = new AnimationGraph();
    }

    void AnimationGraphAsset::OnAssetLoaded() {
        Super::OnAssetLoaded();
    }

    AnimationGraphAsset::~AnimationGraphAsset() = default;
}