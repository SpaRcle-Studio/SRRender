//
// Created by Monika on 19.01.2025.
//

#include <Graphics/UI/UICanvasComponent.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Camera.h>

#include <Utils/World/Scene.h>

#include <Codegen/UICanvasComponent.generated.hpp>

namespace SR_GRAPH_UI_NS {
    void UICanvasComponent::Prepare(SR_UTILS_NS::UI::UIModifierContext& context) const {
        SR_TRACY_ZONE;

        auto&& pRenderScene = GetScene()->GetDataStorage().GetPointer<SR_GRAPH_NS::RenderScene>();
        if (!pRenderScene) {
            return;
        }

        SR_MATH_NS::UVector2 screenSize;

        auto&& pCamera = pRenderScene->GetMainCamera();
        if (!pCamera) {
            screenSize = pCamera->GetSize();
        }
        else if (auto&& pWindow = pRenderScene->GetWindow()) {
            screenSize = pWindow->GetSize();
        }

        context.contentSize.SetPixels(screenSize.Cast<float_t>());
    }
}
