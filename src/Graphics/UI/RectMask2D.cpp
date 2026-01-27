//
// Created by Monika on 27.01.2026.
//

#include <Graphics/UI/RectMask2D.h>

#include <Utils/ECS/TransformRect.h>
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

    void RectMask2D::UpdateClipping(bool enable) {
        SR_TRACY_ZONE;

        if (auto&& pTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            SR_UTILS_NS::UI::MaskInfo maskInfo;
            maskInfo.hasMask = enable && IsActive();
            maskInfo.scissor = true;
            maskInfo.rect = pTransform->GetLayoutRect().ToInt();

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
