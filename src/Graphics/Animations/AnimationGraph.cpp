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
        AddNode<AnimationGraphNodeFinal>();
    }

    AnimationGraph::~AnimationGraph() {
        for (auto&& pNode : m_nodes) {
            delete pNode;
        }
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

        for (auto&& pGameObject : m_gameObjects) {
            auto&& pTransform = pGameObject->GetTransform();
            if (!pTransform) {
                continue;
            }

            AnimationGameObjectData& data = m_sourceGameObjectsData.emplace_back();
            data.translation = pTransform->GetTranslation();
            data.rotation = pTransform->GetQuaternion();
            data.scaling = pTransform->GetScale();
        }

        m_testGameObjectsData.resize(m_sourceGameObjectsData.size());

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