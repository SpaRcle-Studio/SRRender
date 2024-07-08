//
// Created by Monika on 05.05.2023.
//

#include <Graphics/Animations/AnimationGraph.h>
#include <Graphics/Animations/Animator.h>

#include <Utils/ECS/Transform.h>
#include <Utils/ECS/GameObject.h>

namespace SR_ANIMATIONS_NS {
    AnimationGraph::AnimationGraph(Animator* pAnimator)
        : Super()
        , m_pAnimator(pAnimator)
    {
        SRAssert2(m_pAnimator, "Invalid animator!");
        CreateNode<AnimationGraphNodeFinal>();
    }

    AnimationGraph::~AnimationGraph() {
        for (auto&& pNode : m_nodes) {
            delete pNode;
        }
    }

    AnimationGraph* AnimationGraph::Load(Animator* pAnimator, const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        if (!path.Exists(SR_UTILS_NS::Path::Type::File)) {
            SR_ERROR("AnimationGraph::Load() : animation graph \"{}\" isn't exists!", path.ToStringRef());
            return nullptr;
        }

        auto&& xmlDocument = SR_XML_NS::Document::Load(path);
        if (!xmlDocument) {
            SR_ERROR("AnimationGraph::Load() : failed to open xml document \"{}\"!", path.ToStringRef());
            return nullptr;
        }

        auto&& rootXml = xmlDocument.Root().GetNode("Animator");
        if (!rootXml) {
            SR_ERROR("AnimationGraph::Load() : failed to find root node \"Animator\" in xml document \"{}\"!", path.ToStringRef());
            return nullptr;
        }

        auto&& pAnimationGraph = new AnimationGraph(pAnimator);
        pAnimationGraph->m_path = path;

        std::vector<uint32_t> errorNodes;

        for (auto&& nodeXml : rootXml.GetNodes("Node")) {
            if (auto&& pNode = AnimationGraphNode::Load(nodeXml)) {
                pAnimationGraph->AddNode(pNode);
            }
            else {
                errorNodes.emplace_back(pAnimationGraph->GetNodesCount());
                SR_ERROR("AnimationGraph::Load() : failed to load node \"{}\"!", nodeXml.GetAttribute("Type").ToString());
            }
        }

        auto&& fixNodeIndex = [&errorNodes](uint32_t index) {
            for (auto&& errorIndex : errorNodes) {
                if (errorIndex > index) {
                    --index;
                }
            }
            return index;
        };

        for (auto&& transitionXml : rootXml.GetNodes("Connection")) {
            auto&& fromNodeIndex = fixNodeIndex(transitionXml.GetAttribute("FromNode").ToUInt());
            auto&& toNodeIndex = fixNodeIndex(transitionXml.GetAttribute("ToNode").ToUInt());
            auto&& pFromNode = pAnimationGraph->GetNode(fromNodeIndex);
            auto&& pToNode = pAnimationGraph->GetNode(toNodeIndex);

            if (!pFromNode || !pToNode) {
                SR_ERROR("AnimationGraph::Load() : failed to find node! From: {}, to: {}",
                    pFromNode ? "ok" : "fail", pToNode ? "ok" : "fail");
                continue;
            }

            auto&& fromPinIndex = transitionXml.GetAttribute("FromPin").ToUInt();
            auto&& toPinIndex = transitionXml.GetAttribute("ToPin").ToUInt();

            if (fromPinIndex >= pFromNode->GetOutputCount() || toPinIndex >= pToNode->GetInputCount()) {
                SR_ERROR("AnimationGraph::Load() : pins out of range! From: {}, to: {}",
                    fromPinIndex >= pFromNode->GetOutputCount() ? "ok" : "fail",
                    toPinIndex >= pToNode->GetInputCount() ? "ok" : "fail");
                continue;
            }

            pFromNode->ConnectTo(pToNode, fromPinIndex, toPinIndex);
        }

        return pAnimationGraph;
    }

    uint64_t AnimationGraph::GetNodeIndex(const AnimationGraphNode* pNode) const {
        if (auto&& pIt = m_indices.find(const_cast<AnimationGraphNode*>(pNode)); pIt != m_indices.end()) {
            return pIt->second;
        }

        SRHalt("Node not found!");

        return SR_ID_INVALID;
    }

    AnimationGraphNode* AnimationGraph::GetFinal() const {
        return m_nodes.front();
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

        if (m_isCompiled || !m_pAnimator) SR_UNLIKELY_ATTRIBUTE {
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
            return m_nodes.at(index);
        }

        SRHalt("Out of range!");

        return nullptr;
    }
}