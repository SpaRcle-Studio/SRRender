//
// Created by Monika on 01.08.2022.
//

#include <Graphics/UI/Canvas.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Types/Camera.h>

#include <Utils/World/Scene.h>
#include <Utils/World/SceneUpdater.h>
#include <Utils/ECS/TransformRect.h>
#include <Utils/ECS/GameObject.h>

#include <Codegen/Canvas.generated.hpp>

namespace SR_GRAPH_UI_NS {
    void Canvas::OnAttached() {
        if (auto&& pScene = GetScene()) {
            m_renderScene = dynamic_cast<RenderScene*>(pScene->GetModule("Render"));
        }

        SR_UTILS_NS::UIComponent::OnAttached();
    }

    SR_MATH_NS::FRect Canvas::LayoutToCanvasRect(const SR_MATH_NS::FRect& layoutRect) const {
        return SR_MATH_NS::FRect(layoutRect.xy - GetSize().CastToFloat() / 2.f, layoutRect.Size());
    }

    SR_NODISCARD SR_MATH_NS::FVector2 Canvas::ScreenToCanvasSpace(const SR_MATH_NS::FVector2& screenPosition) const {
        auto&& pWindow = GetWindow();
        if (!pWindow) {
            return SR_MATH_NS::FVector2::Zero();
        }

        SR_MATH_NS::FVector2 clientPos = pWindow->ScreenToClient(screenPosition.CastToInt()).CastToFloat();

        return ClientToCanvasSpace(clientPos);
    }

    SR_MATH_NS::FVector2 Canvas::ClientToCanvasSpace(const SR_MATH_NS::FVector2& clientPos) const {
        const SR_MATH_NS::FRect viewportRect = GetViewportRect();

        const SR_MATH_NS::FVector2 viewportSize = viewportRect.Size();
        const SR_MATH_NS::FVector2 canvasSize = GetSize().CastToFloat();

        if (viewportSize.x <= 0.f || viewportSize.y <= 0.f) {
            return SR_MATH_NS::FVector2::Zero();
        }

        const float_t screenFactor = m_scaleFactor > 0.f ? m_scaleFactor : 1.f;

        SR_MATH_NS::FVector2 normalized = (clientPos - viewportRect.Min()) / viewportSize;
        SR_MATH_NS::FVector2 uiPos = normalized * canvasSize / screenFactor;
        uiPos.y = (canvasSize.y / screenFactor) - uiPos.y;

        //const SR_MATH_NS::FVector2 multiplier = viewportSize.CastToFloat() / canvasSize;
        //return uiPos * multiplier;

        return uiPos - (GetSize().CastToFloat() / 2.f) / GetScaleFactor();

        //return uiPos;
    }

    SR_GTYPES_NS::Camera* Canvas::GetCamera() const noexcept {
        return const_cast<SR_GTYPES_NS::Camera*>(m_camera.Get());
    }

    bool Canvas::ContainsScreenPoint(const SR_UTILS_NS::TransformRect& rect, const SR_MATH_NS::FVector2& screenPosition) const {
        SR_UTILS_NS::PointerEvent event;
        event.position = screenPosition;
        return ContainsPointer(rect, event);
    }

    SR_MATH_NS::FVector2 Canvas::PointerToCanvasSpace(const SR_UTILS_NS::PointerEvent& event) const {
        PreparePointerQuery(event);
        return m_pointerCanvasPosition;
    }

    void Canvas::PreparePointerQuery(const SR_UTILS_NS::PointerEvent& event) const {
        auto&& pScene = TryGetScene();
        const uint64_t revision = pScene ? pScene->GetSceneUpdater()->GetUIInputRevision() : 0;
        if (pScene && m_pointerQueryValid && m_pointerRevision == revision
            && m_pointerSpace == event.space && m_pointerPosition == event.position) {
            return;
        }
        SR_TRACY_ZONE;
        m_pointerRevision = revision;
        m_pointerSpace = event.space;
        m_pointerPosition = event.position;
        m_pointerQueryValid = true;
        auto client = event.position;
        if (event.space == SR_UTILS_NS::PointerSpace::Screen) {
            auto&& pWindow = GetWindow();
            if (!pWindow) {
                m_pointerInViewport = false;
                m_pointerCanvasPosition = SR_MATH_NS::FVector2::Zero();
                return;
            }
            client = pWindow->ScreenToClient(event.position.CastToInt()).CastToFloat();
        }
        m_pointerInViewport = m_viewportRect.Width() > 0.f && m_viewportRect.Height() > 0.f && m_viewportRect.Contains(client);
        m_pointerCanvasPosition = ClientToCanvasSpace(client);
    }

    bool Canvas::ContainsPointerViewport(const SR_UTILS_NS::PointerEvent& event) const {
        PreparePointerQuery(event);
        return m_pointerInViewport;
    }

    bool Canvas::ContainsPointer(const SR_UTILS_NS::TransformRect& rect, const SR_UTILS_NS::PointerEvent& event) const {
        return ContainsPointerViewport(event) && !rect.GetGlobalScale().HasZero()
            && rect.GetLayoutRect().Contains(CanvasToLayoutPoint(rect, m_pointerCanvasPosition));
    }

    SR_MATH_NS::FVector2 Canvas::CanvasToLayoutPoint(const SR_UTILS_NS::TransformRect& rect, const SR_MATH_NS::FVector2& canvasPosition) const {
        const auto point = canvasPosition * m_scaleFactor + m_size.CastToFloat() / 2.f;
        const auto local = rect.GetMatrix().Inverse().TransformPoint({ point.x, point.y, 0.f });
        return SR_MATH_NS::FVector2(local.x, local.y);
    }

    Window* Canvas::GetWindow() const noexcept {
        if (m_renderScene) {
            return m_renderScene->GetWindow().Get();
        }
        return nullptr;
    }

    void Canvas::PrepareUIInput() {
        SR_TRACY_ZONE;
        auto&& pWindow = GetWindow();
        const SR_MATH_NS::IVector2 clientOrigin = pWindow
            ? pWindow->ScreenToClient(SR_MATH_NS::IVector2()) : SR_MATH_NS::IVector2();
        if (m_inputWindowAvailable != bool(pWindow) || m_inputClientOrigin != clientOrigin) {
            m_inputWindowAvailable = bool(pWindow);
            m_inputClientOrigin = clientOrigin;
            InvalidateUIInput();
        }
        if (m_renderScene) {
            SR_MATH_NS::UVector2 windowSize;

            if (auto&& pCamera = m_renderScene->GetMainCamera()) {
                m_camera = pCamera;
                windowSize = pCamera->GetSize();
                SetViewportRect(pCamera->GetViewportRect());
            }
            else {
                m_camera = nullptr;
                windowSize = m_renderScene->GetSurfaceSize();
                SetViewportRect(SR_MATH_NS::FRect(SR_MATH_NS::FVector2(), windowSize.CastToFloat()));
            }

            auto&& pTransform = GetTransform();

            if (!m_previewViewportSize.HasZero()) {
                windowSize = m_previewViewportSize;
            }
            m_dirty |= windowSize != m_size;
            m_size = windowSize;
            if (auto&& pCanvasScaler = GetGameObject()->GetComponent<CanvasScaler>(); pCanvasScaler && pCanvasScaler->IsActive()) {
                m_hadScaler = true;
                const float_t newScaleFactor = pCanvasScaler->CalculateScaleFactor(*this);
                m_dirty |= !SR_MATH_NS::IsEquals(m_scaleFactor, newScaleFactor);
                m_scaleFactor = newScaleFactor;

                const float_t newPixelsPerUnit = pCanvasScaler->CalculateReferencePixelsPerUnit();
                m_dirty |= !SR_MATH_NS::IsEquals(m_referencePixelsPerUnit, newPixelsPerUnit);
                m_referencePixelsPerUnit = newPixelsPerUnit;

                m_scaleFactor = SR_MAX(0.01f, m_scaleFactor);
            }
            else if (m_hadScaler) {
                m_hadScaler = false;
                m_scaleFactor = 1.f;
                m_referencePixelsPerUnit = 100.f;
                m_dirty = true;
            }

            if (m_dirty) {
                InvalidateUIInput();
            }
            if (m_dirty && pTransform && pTransform->GetMeta()->IsSameOrInherited(SR_UTILS_NS::TransformRect::GetClassStaticName())) {
                m_size = windowSize;
                m_dirty = false;

                SR_UTILS_NS::RectAnchors anchors;
                anchors.min = 0.f;
                anchors.max = 0.f;

                static_cast<SR_UTILS_NS::TransformRect*>(pTransform)->SetSize(m_size.Cast<float_t>() / m_scaleFactor);
                static_cast<SR_UTILS_NS::TransformRect*>(pTransform)->SetAnchors(anchors);
                static_cast<SR_UTILS_NS::TransformRect*>(pTransform)->SetCanvasSize(m_size.CastToFloat());

                pTransform->SetScale(m_scaleFactor);
                pTransform->SetTranslation(SR_MATH_NS::FVector3(m_size.Cast<float_t>() / 2.f, 0.f));

                pTransform->UpdateTree();
            }
        }

        SR_UTILS_NS::UIComponent::PrepareUIInput();
    }

    void Canvas::SetViewportRect(const SR_MATH_NS::FRect& rect) {
        if (m_viewportRect != rect) {
            m_viewportRect = rect;
            InvalidateUIInput();
        }
    }

    void Canvas::SetScaleFactor(float_t scaleFactor) {
        if (!SR_MATH_NS::IsEquals(m_scaleFactor, scaleFactor)) {
            m_scaleFactor = scaleFactor;
            m_dirty = true;
            InvalidateUIInput();
        }
    }

    SR_GRAPH_NS::UI::Canvas* IFindCanvasOwner::FindCanvas(const SR_UTILS_NS::SceneObject* pSO) {
        auto&& pScene = pSO ? pSO->GetScenePtr() : nullptr;
        const uint64_t revision = pScene ? pScene->GetSceneUpdater()->GetUIInputTargetsRevision() : 0;
        if (pScene && m_pCanvasSearchRoot == pSO && m_pCanvasSearchScene == pScene
            && m_canvasSearchRevision == revision) {
            if (m_canvas.GetEntityId() == SR_UTILS_NS::SR_INVALID_ENTITY_ID) {
                return nullptr;
            }
            if (auto&& pCanvas = m_canvas.Get(); pCanvas && pCanvas->IsComponentValid()) {
                return pCanvas.Get();
            }
        }
        SR_TRACY_ZONE;
        m_pCanvasSearchRoot = pSO;
        m_pCanvasSearchScene = pScene;
        m_canvasSearchRevision = revision;
        auto&& pCurrentStart = const_cast<SR_UTILS_NS::SceneObject*>(pSO);
        SR_UTILS_NS::SceneObject* pCurrent = pCurrentStart;
        while (pCurrent) {
            if (auto pCanvas = pCurrent->GetComponent<SR_GRAPH_NS::UI::Canvas>()) {
                m_canvas.SetEntityId(pCanvas->GetEntityId());
                return pCanvas.Get();
            }
            pCurrent = const_cast<SR_UTILS_NS::SceneObject*>(pCurrent->GetParent().Get());
        }
        m_canvas.SetEntityId(SR_UTILS_NS::SR_INVALID_ENTITY_ID);
        return nullptr;
    }

    float_t CanvasScaler::CalculateScaleFactor(const Canvas& canvas) const {
        return CalculateScaleFactorForViewport(canvas.GetSize().CastToFloat(),
            canvas.GetWindow() ? canvas.GetWindow()->GetScreenDPI() : 0.f);
    }

    void CanvasScaler::OnPostLoad() {
        Super::OnPostLoad();
        SetScaleDirty();
    }

    void CanvasScaler::SetScaleDirty() {
        m_scaleDirty = true;
        SetLayoutDirty();
    }

    float_t CanvasScaler::CalculateScaleFactorForViewport(const SR_MATH_NS::FVector2& screenSize, float_t dpi) const {
        SR_TRACY_ZONE;

        if (!m_scaleDirty && m_cachedScreenSize == screenSize && m_cachedDPI == dpi) {
            return m_cachedScaleFactor;
        }
        m_cachedScreenSize = screenSize;
        m_cachedDPI = dpi;
        m_scaleDirty = false;

        float_t scale = 1.f;
        switch (m_scaleMode) {
            case CanvasScaleMode::ConstantPixelSize:
                scale = m_scaleFactor;
                break;
            case CanvasScaleMode::ScaleWithScreenSize: {
                if (screenSize.HasZero() || m_referenceResolution.HasZero()) {
                    break;
                }

                const float_t scaleX = screenSize.x / m_referenceResolution.CastToFloat().x;
                const float_t scaleY = screenSize.y / m_referenceResolution.CastToFloat().y;
                switch (m_screenMatchMode) {
                    case CanvasScreenMatchMode::MatchWidthOrHeight: {
                        const float_t logWidth  = std::log(scaleX) / std::log(2.f);
                        const float_t logHeight = std::log(scaleY) / std::log(2.f);
                        const float_t logWeighted = SR_MATH_NS::Lerp(logWidth, logHeight, m_match);
                        scale = std::pow(2.f, logWeighted);
                        break;
                    }

                    case CanvasScreenMatchMode::Expand: {
                        scale = std::min(scaleX, scaleY);
                        break;
                    }

                    case CanvasScreenMatchMode::Shrink: {
                        scale = std::max(scaleX, scaleY);
                        break;
                    }
                    default:
                        SRHalt("CanvasScaler::Update() : unknown screen match mode!");
                        break;
                }
                break;
            }
            case CanvasScaleMode::ConstantPhysicalSize: {
                if (dpi <= 0.f) {
                    dpi = m_fallbackScreenDPI;
                }
                scale = dpi / SR_MATH_NS::GetPhysicalUnitScale(m_physicalUnit);
                break;
            }
            default:
                SRHalt("CanvasScaler::Update() : unknown scale mode!");
                break;
        }

        m_cachedScaleFactor = SR_MAX(0.01f, scale);
        return m_cachedScaleFactor;
    }

    float_t CanvasScaler::CalculateReferencePixelsPerUnit() const {
        if (m_scaleMode == CanvasScaleMode::ConstantPhysicalSize) {
            return m_referencePixelsPerUnit * SR_MATH_NS::GetPhysicalUnitScale(m_physicalUnit)
                / SR_MAX(1.f, m_defaultSpriteDPI);
        }
        return m_referencePixelsPerUnit;
    }
}
