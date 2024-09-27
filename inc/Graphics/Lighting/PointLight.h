//
// Created by Nikita on 13.12.2020.
//

#ifndef SR_ENGINE_POINTLIGHT_H
#define SR_ENGINE_POINTLIGHT_H

#include <Graphics/Lighting/ILightComponent.h>

namespace SR_GRAPH_NS {
    class PointLight : public ILightComponent {
    public:

    protected:
        float_t m_radius = 1.f;

    };
}

#endif //SR_ENGINE_POINTLIGHT_H
