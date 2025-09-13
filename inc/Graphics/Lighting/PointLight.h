//
// Created by Nikita on 13.12.2020.
//

#ifndef SR_ENGINE_POINTLIGHT_H
#define SR_ENGINE_POINTLIGHT_H

#include <Graphics/Lighting/ILightComponent.h>

namespace SR_GRAPH_NS {
    class PointLight : public ILightComponent {
        SR_CLASS()
    public:
        SR_NODISCARD LightType GetLightType() const override { return LightType::Point; };

    protected:
        /// @property
        float_t m_radius = 1.f;

    };
}

#endif //SR_ENGINE_POINTLIGHT_H
