//
// Created by Monika on 31.07.2025.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_STAGES_SETTINGS_H
#define SR_ENGINE_GRAPHICS_RENDER_STAGES_SETTINGS_H

#include <Graphics/Pipeline/IShaderProgram.h>

#include <Utils/Resources/Asset.h>

namespace SR_GRAPH_NS {
    class RenderStagesSettings : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RenderStagesSettings>;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_STAGES_SETTINGS_H
