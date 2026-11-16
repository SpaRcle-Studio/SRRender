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
        SR_UTILS_NS::UIComponent::OnEnable();
    }

    void RectMask2D::OnDisable() {
        UpdateClipping(false);
        SR_UTILS_NS::UIComponent::OnDisable();
    }

    void RectMask2D::OnDetached() {
        UpdateClipping(false);
        SR_UTILS_NS::UIComponent::OnDetached();
    }

    void RectMask2D::OnMatrixDirty() {
        SR_UTILS_NS::UIComponent::OnMatrixDirty();
    }

    void RectMask2D::AfterLayout() {
        SR_UTILS_NS::UIComponent::AfterLayout();
        UpdateClipping(true);
    }

    void RectMask2D::UpdateClipping(bool enable) {
        SR_TRACY_ZONE;

        if (auto&& pTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            SR_UTILS_NS::UI::MaskInfo maskInfo;
            maskInfo.hasMask = enable && IsActive();
            maskInfo.scissor = true;

            if (maskInfo.hasMask) {
                const SR_MATH_NS::FRect transformedRect = pTransform->GetTransformedLayoutRect();
                const int32_t minX = static_cast<int32_t>(std::ceil(transformedRect.Left() - 0.5f));
                const int32_t minY = static_cast<int32_t>(std::ceil(transformedRect.Bottom() - 0.5f));
                const int32_t maxX = static_cast<int32_t>(std::ceil(transformedRect.Right() - 0.5f));
                const int32_t maxY = static_cast<int32_t>(std::ceil(transformedRect.Top() - 0.5f));
                maskInfo.rect = SR_MATH_NS::IRect(minX, minY, maxX - minX, maxY - minY);

                if (auto&& pCanvas = FindCanvas(GetSceneObject().Get())) {
                    maskInfo.referenceSize = pCanvas->GetSize().CastToInt();
                }
                maskInfo.rect.y = maskInfo.referenceSize.y - (maskInfo.rect.y + maskInfo.rect.h);
            }

            if (m_maskInfo == maskInfo) {
                return;
            }

            if (!m_renderScene) {
                if (auto&& pScene = TryGetScene()) {
                    m_renderScene = dynamic_cast<RenderScene*>(pScene->GetModule("Render"));
                }
            }

            if (m_renderScene) {
                m_renderScene->SetDirty();
            }

            m_maskInfo = maskInfo;
            pTransform->SetMaskInfo(maskInfo);
        }
    }
}
