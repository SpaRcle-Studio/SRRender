//
// Created by Monika on 27.01.2026.
//

#include <Graphics/UI/RectMask2D.h>

#include <Utils/ECS/TransformRect.h>
#include <Utils/ECS/SceneObject.h>
#include <Utils/World/Scene.h>

#include <Codegen/RectMask2D.generated.hpp>

namespace SR_GRAPH_NS::UI {
    void RectMask2D::OnEnable() {
        UpdateClipping(true);
        Super::OnEnable();
    }

    void RectMask2D::OnDisable() {
        UpdateClipping(false);
        Super::OnDisable();
    }

    void RectMask2D::OnDetached() {
        UpdateClipping(false);
        Super::OnDetached();
    }

    void RectMask2D::OnMatrixDirty() {
        UpdateClipping(true);
        Super::OnMatrixDirty();
    }

    SR_GRAPH_NS::UI::Canvas* RectMask2D::FindCanvas() {
        if (auto&& pCanvas = m_canvas.Get()) {
            return pCanvas.Get();
        }

        SR_UTILS_NS::SceneObject::Ptr pParent = GetSceneObject()->GetParent();
        while (pParent) {
            if (auto&& pCanvas = pParent->GetComponent<SR_GRAPH_NS::UI::Canvas>()) {
                m_canvas.SetEntityId(pCanvas->GetEntityId());
                return pCanvas.Get();
            }
            pParent = pParent->GetParent();
        }

        return nullptr;
    }

    void RectMask2D::UpdateClipping(bool enable) {
        SR_TRACY_ZONE;

        if (auto&& pTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            SR_UTILS_NS::UI::MaskInfo maskInfo;
            maskInfo.hasMask = enable && IsActive();
            maskInfo.scissor = true;
            maskInfo.rect = pTransform->GetLayoutRect().CastToInt();

            if (auto&& pCanvas = FindCanvas()) {
                maskInfo.referenceSize = pCanvas->GetSize().CastToInt();
            }

            if (m_maskInfo == maskInfo) {
                return;
            }

            if (!m_renderScene) {
                m_renderScene = GetScene()->GetDataStorage().GetValue<SR_HTYPES_NS::SharedPtr<RenderScene>>();
            }

            if (m_renderScene) {
                m_renderScene->SetDirty();
            }

            pTransform->SetMaskInfo(maskInfo);
        }
    }
}
