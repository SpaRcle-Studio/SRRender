//
// Created by Monika on 17.05.2025.
//

#include <Graphics/UI/UINode.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Camera.h>

#include <Utils/World/Scene.h>

#ifdef SR_RENDER_USE_YOGA
    #include <yoga/Yoga.h>
#endif

#include <Codegen/UINode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    UINode::UINode()
        : Super()
        , m_finalRect(SR_MATH_NS::FRect(0.f, 0.f, 1.f, 1.f))
    {
        m_implNode = YGNodeNew();
        YGNodeSetContext(m_implNode, this);
    }

    UINode::~UINode() {
        if (m_implNode) {
            YGNodeFree(m_implNode);
            m_implNode = nullptr;
        }
    }

    SR_UTILS_NS::ECSNodeType UINode::GetNodeType() const noexcept {
        return SR_UTILS_NS::ECSNodeType::UINode;
    }

    const SR_MATH_NS::Matrix4x4& UINode::GetMatrix() const noexcept {
        auto&& viewportSize = SR_MATH_NS::FVector3(GetViewportSize(), 1.f);
        if (viewportSize.HasZero()) {
            static const SR_MATH_NS::Matrix4x4 identity = SR_MATH_NS::Matrix4x4::Identity();
            return identity;
        }

        SR_MATH_NS::FVector3 translation = m_finalRect.XY0().InverseAxis(SR_MATH_NS::Axis::Y);
        //translation -= SR_MATH_NS::FVector3(viewportSize.x / 2.f, 0, 0);
        //translation.y += m_finalRect.h / 2.f;

        //translation.x -= viewportSize.x / 2.f;

        const float_t left  = (m_finalRect.x / viewportSize.x) * 2.0f - 1.0f;
        const float_t right = ((m_finalRect.x + m_finalRect.w) / viewportSize.y) * 2.0f - 1.0f;

        const float_t ndcX = (left + right) / 2.0f;
        //float ndcY = (top + bottom) / 2.0f;

        const float_t scaleX = (right - left) / 2.0f;
        //float scaleY = (top - bottom) / 2.0f;  // top > bottom

        m_matrix = SR_MATH_NS::Matrix4x4(
            SR_MATH_NS::FVector3(ndcX, 0.f, 0.f),
            SR_MATH_NS::Quaternion::Identity(),
            SR_MATH_NS::FVector3(scaleX, 1.f, 1.f)
        );

        //m_matrix = SR_MATH_NS::Matrix4x4(
        //    SR_MATH_NS::FVector3(translation / viewportSize) - SR_MATH_NS::FVector3(0.5f, 0.f, 0.f),
        //    SR_MATH_NS::Quaternion::Identity(),
        //    SR_MATH_NS::FVector3(m_finalRect.WH1() / viewportSize)
        //);

        return m_matrix;
    }

    void UINode::SetViewportSize(const SR_MATH_NS::FVector2& size) noexcept {
        m_viewportSize = size;

        for (auto&& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node) {
                continue;
            }

            if (auto&& pNode = dynamic_cast<UINode*>(pChild.Get())) {
                pNode->SetViewportSize(size);
            }
        }
    }

    SR_GTYPES_NS::Camera* UINode::GetCamera() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene->GetMainCamera().Get();
        }
        return nullptr;
    }

    void UINode::Compile() {
        OnMatrixDirty();

        for (auto&& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node) {
                continue;
            }

            if (auto&& pNode = dynamic_cast<UINode*>(pChild.Get())) {
                pNode->Compile();
            }
        }
    }

    RenderScene* UINode::TryGetRenderScene() const  {
        if (m_renderScene) {
            return m_renderScene;
        }

        auto&& pScene = GetScene();
        if (!pScene) {
            return m_renderScene;
        }

        m_renderScene = pScene->GetDataStorage().GetPointer<RenderScene>();

        return m_renderScene;
    }

    RenderScene* UINode::GetRenderScene() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene;
        }

        SRHalt("Invalid render scene!");

        return nullptr;
    }

    void UINode::OnParentChanged(const SR_UTILS_NS::SceneObject::Ptr& pOldParent) {
        Super::OnParentChanged(pOldParent);

        if (auto&& pUINode = pOldParent.DynamicCast<UINode>()) {
            YGNodeRemoveChild(pUINode->GetYGNode(), m_implNode);
        }

        if (auto&& pUINode = GetParent().DynamicCast<UINode>()) {
            SRAssert2(YGNodeGetParent(m_implNode) == nullptr, "UINode::OnParentChanged() : parent is not null!");
            YGNodeInsertChild(pUINode->GetYGNode(), m_implNode, YGNodeGetChildCount(pUINode->GetYGNode())); /// TODO: children order
        }
    }

    SR_MATH_NS::FVector4 UINode::GetNDCVector() const noexcept {
        const float_t left   = (m_finalRect.x / m_viewportSize.x) * 2.0f - 1.0f;
        const float_t right  = ((m_finalRect.x + m_finalRect.w) / m_viewportSize.x) * 2.0f - 1.0f;
        const float_t top    = 1.0f - (m_finalRect.y / m_viewportSize.y) * 2.0f;
        const float_t bottom = 1.0f - ((m_finalRect.y + m_finalRect.h) / m_viewportSize.y) * 2.0f;
        return SR_MATH_NS::FVector4(left, right, top, bottom);
    }

    void UINode::Prepare(uint64_t& priority) {
        ++priority;

        if (m_priority != priority) {
            m_priority = priority;
            OnPriorityChanged();
        }
    }
}