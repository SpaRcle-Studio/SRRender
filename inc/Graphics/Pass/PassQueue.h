//
// Created by Monika on 21.07.2023.
//

#ifndef SR_ENGINE_GRAPHICS_PASS_QUEUE_H
#define SR_ENGINE_GRAPHICS_PASS_QUEUE_H

#include <Graphics/macros.h>

#include <Utils/Serialization/Serializable.h>

namespace SR_GRAPH_NS {
    class BasePass;

    struct PassQueue : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        std::vector<BasePass*> passes;

        /// @property
        std::vector<SR_UTILS_NS::StringAtom> passNames;
    };

    using PassQueues = std::vector<PassQueue>;
}

#endif //SR_ENGINE_GRAPHICS_PASS_QUEUE_H
