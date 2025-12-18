//
// Created by Monika on 01.08.2022.
//

#include <Graphics/UI/Canvas.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Camera.h>

#include <Utils/World/Scene.h>
#include <Utils/ECS/TransformRect.h>
#include <Utils/ECS/ComponentManager.h>

#include <Codegen/Canvas.generated.hpp>

namespace SR_GRAPH_UI_NS {
    void Canvas::OnAttached() {
        if (auto&& pScene = GetScene()) {
            m_renderScene = pScene->GetDataStorage().GetValue<RenderScenePtr>();
        }

        Super::OnAttached();
    }

    void Canvas::Update(float_t dt) {
        if (m_renderScene.RecursiveLockIfValid()) {
            SR_MATH_NS::UVector2 windowSize;

            if (auto&& pCamera = m_renderScene->GetMainCamera()) {
                windowSize = pCamera->GetSize();
            }
            else {
                windowSize = m_renderScene->GetSurfaceSize();
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

            m_renderScene.Unlock();
        }

        Super::Update(dt);
    }
}