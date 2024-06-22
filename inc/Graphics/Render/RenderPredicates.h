//
// Created by Monika on 03.06.2024.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_PREDICATES_H
#define SR_ENGINE_GRAPHICS_RENDER_PREDICATES_H

#include <Utils/Types/StringAtom.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class LayerFilterPredicate {
    public:
        virtual ~LayerFilterPredicate() = default;
        SR_NODISCARD virtual bool IsLayerAllowed(SR_UTILS_NS::StringAtom layer) const = 0;
    };

    class PriorityFilterPredicate {
    public:
        virtual ~PriorityFilterPredicate() = default;
        SR_NODISCARD virtual bool IsPriorityAllowed(int64_t priority) const = 0;
    };

    class ShaderReplacePredicate {
    public:
        virtual ~ShaderReplacePredicate() = default;
        SR_NODISCARD virtual SR_GRAPH_NS::ShaderUseInfo ReplaceShader(SR_GTYPES_NS::Shader* pShader) const = 0;
    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_PREDICATES_H
