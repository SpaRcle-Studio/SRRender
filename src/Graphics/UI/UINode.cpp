//
// Created by Monika on 17.05.2025.
//

#include <Graphics/UI/UINode.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Camera.h>

#include <Utils/World/Scene.h>

#include <Codegen/UINode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    UINode::UINode()
        : Super()
        , m_finalRect(SR_MATH_NS::FRect(0.f, 0.f, 1.f, 1.f))
    { }

    SR_UTILS_NS::ECSNodeType UINode::GetNodeType() const noexcept {
        return SR_UTILS_NS::ECSNodeType::UINode;
    }

    const SR_MATH_NS::Matrix4x4& UINode::GetMatrix() const noexcept {
        m_matrix = SR_MATH_NS::Matrix4x4(
            SR_MATH_NS::FVector3(m_finalRect.XY0()),
            SR_MATH_NS::Quaternion::Identity(),
            SR_MATH_NS::FVector3(m_finalRect.WH1())
        );

        return m_matrix;
    }

    SR_GTYPES_NS::Camera* UINode::GetCamera() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene->GetMainCamera().Get();
        }
        return nullptr;
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
}