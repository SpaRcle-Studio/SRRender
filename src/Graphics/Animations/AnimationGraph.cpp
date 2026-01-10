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
    { }

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
            SRHalt("AnimationGraph::GetFinal() : no nodes in graph!");
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

        auto&& gameObjectsData = pPose->GetGameObjects();
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

        compileContext.pSkeleton = m_pAnimator->GetSkeleton().Get();

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

        SRHalt("Out of range!");

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
        for (auto&& pNode : m_nodes) {
            pNode->SetGraph(this);
        }
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

    const AnimationGraph& AnimationGraphAsset::GetData() const noexcept {
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

    AnimationGraphAsset::~AnimationGraphAsset() {
        m_data.AutoFree();
    }
}