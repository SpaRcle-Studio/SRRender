//
// Created by Monika on 11.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
#define SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H

#include <Graphics/Types/IRenderComponent.h>

#include <Utils/ECS/Component.h>

namespace SR_GRAPH_NS {
    /// @category(Render.Particles)
    class ParticleEmitter : public SR_GTYPES_NS::IRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:

    };
}

#endif //SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
