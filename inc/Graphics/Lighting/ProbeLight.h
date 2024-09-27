//
// Created by Nikita on 13.12.2020.
//

#ifndef SR_ENGINE_PROBELIGHT_H
#define SR_ENGINE_PROBELIGHT_H

#include <Graphics/Lighting/ILightComponent.h>

namespace SR_GRAPH_NS {
    class ProbeLight : public ILightComponent {
    protected:
        float_t m_radius = 1.f;
        float_t m_distance = 10.f;

    };
}

#endif //SR_ENGINE_PROBELIGHT_H
