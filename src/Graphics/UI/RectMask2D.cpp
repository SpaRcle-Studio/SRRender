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

    void RectMask2D::UpdateClipping(bool enable) {
        SR_TRACY_ZONE;

        if (auto&& pTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            SR_UTILS_NS::UI::MaskInfo maskInfo;
            maskInfo.hasMask = enable && IsActive();
            maskInfo.scissor = true;

            const SR_MATH_NS::FRect transformedRect = pTransform->GetTransformedLayoutRect();
            const int32_t minX = static_cast<int32_t>(std::floor(transformedRect.Left()));
            const int32_t minY = static_cast<int32_t>(std::floor(transformedRect.Bottom()));
            const int32_t maxX = static_cast<int32_t>(std::ceil(transformedRect.Right()));
            const int32_t maxY = static_cast<int32_t>(std::ceil(transformedRect.Top()));
            maskInfo.rect = SR_MATH_NS::IRect(minX, minY, maxX - minX, maxY - minY);

            if (auto&& pCanvas = FindCanvas(GetSceneObject().Get())) {
                maskInfo.referenceSize = pCanvas->GetSize().CastToInt();
            }

            /// flip Y axis
            maskInfo.rect.y = maskInfo.referenceSize.y - (maskInfo.rect.y + maskInfo.rect.h);

            if (m_maskInfo == maskInfo) {
                return;
            }

            if (!m_renderScene) {
                m_renderScene = dynamic_cast<RenderScene*>(GetScene()->GetModule("Render"));
            }

            if (m_renderScene) {
                m_renderScene->SetDirty();
            }

            m_maskInfo = maskInfo;
            pTransform->SetMaskInfo(maskInfo);
        }
    }
}
