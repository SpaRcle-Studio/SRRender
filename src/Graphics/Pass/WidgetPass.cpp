//
// Created by Monika on 23.07.2022.
//

#include <Graphics/Pass/WidgetPass.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderScene.h>

#include <Codegen/WidgetPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool WidgetPass::Prepare() {
        Super::Prepare();
        GetPipeline()->PrepareOverlay(OverlayType::ImGui);
        return true;
    }

    bool WidgetPass::Overlay() {
        auto&& pPipeline = GetPipeline();
        auto&& widgetManagers = GetRenderScene()->GetWidgetManagers();

        if (widgetManagers.empty()) {
            return false;
        }

        if (pPipeline->BeginDrawOverlay(OverlayType::ImGui)) {
            /// Во время отрисовки виджета он может быть удален
            for (uint16_t i = 0; ; ++i) {
                if (i >= widgetManagers.size()) {
                    break;
                }

                widgetManagers[i]->Draw();
            }

            pPipeline->EndDrawOverlay(OverlayType::ImGui);

            return true;
        }

        return false;
    }
}