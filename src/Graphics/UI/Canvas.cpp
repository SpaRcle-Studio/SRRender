//
// Created by Monika on 01.08.2022.
//

#include <Graphics/UI/Canvas.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Types/Camera.h>

#include <Utils/World/Scene.h>
#include <Utils/ECS/TransformRect.h>
#include <Utils/ECS/ComponentManager.h>
#include <Utils/ECS/SceneObject.h>

#include <Codegen/Canvas.generated.hpp>

namespace SR_GRAPH_UI_NS {
    void Canvas::OnAttached() {
        if (auto&& pScene = GetScene()) {
            m_renderScene = pScene->GetDataStorage().GetValue<RenderScenePtr>();
        }

        Super::OnAttached();
    }

    SR_GTYPES_NS::Camera* Canvas::GetCamera() const noexcept {
        return const_cast<SR_GTYPES_NS::Camera*>(m_camera.Get());
    }

    Window* Canvas::GetWindow() const noexcept {
        if (m_renderScene) {
            return m_renderScene->GetWindow().Get();
        }
        return nullptr;
    }

    void Canvas::Update(float_t dt) {
        if (m_renderScene) {
            SR_MATH_NS::UVector2 windowSize;

            if (auto&& pCamera = m_renderScene->GetMainCamera()) {
                windowSize = pCamera->GetSize();
                m_viewportRect = pCamera->GetViewportRect();
            }
            else {
                windowSize = m_renderScene->GetSurfaceSize();
                m_viewportRect = SR_MATH_NS::FRect(SR_MATH_NS::FVector2(), windowSize.CastToFloat());
            }

            auto&& pTransform = GetTransform();

            if (windowSize != m_size && pTransform && pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                m_size = windowSize;

                pTransform->SetScale(SR_MATH_NS::FVector3::One());

                SR_UTILS_NS::RectAnchors anchors;
                anchors.min = 0.f;
                anchors.max = 0.f;

                static_cast<SR_UTILS_NS::TransformRect*>(pTransform)->SetSize(m_size.Cast<float_t>());
                static_cast<SR_UTILS_NS::TransformRect*>(pTransform)->SetAnchors(anchors);
                static_cast<SR_UTILS_NS::TransformRect*>(pTransform)->SetCanvasSize(m_size.CastToFloat());

                pTransform->SetTranslation(SR_MATH_NS::FVector3(m_size.Cast<float_t>() / 2.f, 0.f));
            }
        }

        Super::Update(dt);
    }

    SR_GRAPH_NS::UI::Canvas* IFindCanvasOwner::FindCanvas(const SR_UTILS_NS::SceneObject* pSO) {
        SR_TRACY_ZONE;

        if (auto&& pCanvas = m_canvas.Get()) {
            return pCanvas.Get();
        }

        SR_UTILS_NS::SceneObject::Ptr pParent = pSO->GetParent();
        while (pParent) {
            if (auto&& pCanvas = pParent->GetComponent<SR_GRAPH_NS::UI::Canvas>()) {
                m_canvas.SetEntityId(pCanvas->GetEntityId());
                return pCanvas.Get();
            }
            pParent = pParent->GetParent();
        }

        return nullptr;
    }
}