//
// Created by Monika on 18.05.2025.
//

#include <Graphics/UI/UIViewportNode.h>

#include <Utils/Events/Broadcaster.h>

#include <Codegen/UIViewportNode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    UIViewportNode::UIViewportNode()
        : Super()
    {
        //m_onEngineUpdate = SR_UTILS_NS::Broadcaster::Instance().Subscribe(SR_UTILS_NS::Events::EVENT_ON_ENGINE_UPDATE_ID, [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
        //    uint64_t priority = 0;
        //    Prepare(priority);
        //    Layout(SR_MATH_NS::FRect());
        //    Compile();
        //});

        m_keyDown = SR_UTILS_NS::Input::Instance().Subscribe("Down", [&](const SR_UTILS_NS::SubscriptionMessage& msg) {
            if (static_cast<SR_UTILS_NS::KeyCode>(msg.GetInt("KeyCode"_atom)) == SR_UTILS_NS::KeyCode::J) {
                m_doRecalcLayout = true;
            }
        });
    }

    void UIViewportNode::Layout(const SR_MATH_NS::FRect&) {
        SR_TRACY_ZONE;

        auto&& pMainCamera = GetCamera();
        if (!pMainCamera) {
            return;
        }

        auto&& viewportSize = pMainCamera->GetSize().CastToFloat();
        if (viewportSize.HasZero() || viewportSize.HasNegative()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (m_manualLayoutRecalc && !m_doRecalcLayout) {
            return;
        }

        m_doRecalcLayout = false;

        CalculateLayout();

        GetLayout().width = viewportSize.x;
        GetLayout().height = viewportSize.y;

        SetViewportSize(viewportSize);

        Super::Layout(SR_MATH_NS::FRect(0.f, 0.f, viewportSize));

        /// Расставляем детей по размеру родителя, все размеры и прочие расстановки будут делать уже сами дети
        for (SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            if (auto&& pNode = dynamic_cast<UINode*>(pChild.Get())) {
                pNode->Layout(SR_MATH_NS::FRect());
            }
        }
    }

    void UIViewportNode::CalculateLayout() {
        SR_TRACY_ZONE;
        YGNodeCalculateLayout(GetYGNode(), m_viewportSize.x, m_viewportSize.y, YGDirectionLTR);
    }
}