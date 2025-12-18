//
// Created by Monika on 14.07.2022.
//

#ifndef SR_ENGINE_GRAPHICS_WIDGET_PASS_H
#define SR_ENGINE_GRAPHICS_WIDGET_PASS_H

#include <Graphics/Pass/BasePass.h>

namespace SR_GRAPH_NS {
    class WidgetPass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
    public:
        bool Prepare() override;
        bool Overlay() override;

    };
}


#endif //SR_ENGINE_GRAPHICS_WIDGET_PASS_H
