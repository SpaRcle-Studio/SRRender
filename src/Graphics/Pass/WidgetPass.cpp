//
// Created by Monika on 23.07.2022.
//

#include <Graphics/Pass/WidgetPass.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(WidgetPass)

    void WidgetPass::Prepare()
    {
        Super::Prepare();

        if (auto&& pPipeline = GetContext()->GetPipeline()) {
            pPipeline->PrepareOverlay(OverlayType::ImGui);
        }
    }

    bool WidgetPass::Overlay() {
        auto&& pPipeline = GetContext()->GetPipeline();

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