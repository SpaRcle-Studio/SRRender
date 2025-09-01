//
// Created by Monika on 21.07.2023.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_QUEUE_H
#define SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_QUEUE_H

#include <Graphics/macros.h>

#include <Utils/Serialization/Serializable.h>

namespace SR_GRAPH_NS {
    class BasePass;

    struct RenderTechniqueQueue : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        std::vector<SR_UTILS_NS::StringAtom> frameBuffers;
    };

    using RenderTechniqueQueues = std::vector<RenderTechniqueQueue>;
}

#endif //SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_QUEUE_H
