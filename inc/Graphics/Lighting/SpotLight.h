//
// Created by Nikita on 13.12.2020.
//

#ifndef SR_ENGINE_SPOTLIGHT_H
#define SR_ENGINE_SPOTLIGHT_H

#include <Graphics/Lighting/ILightComponent.h>

namespace SR_GRAPH_NS {
    class SpotLight : public ILightComponent {
        SR_CLASS()
    public:
        SR_NODISCARD LightType GetLightType() const override { return LightType::Spot; };

    protected:
        /// @property
        float_t m_radius = 1.f;
        /// @property
        float_t m_distance = 10.f;

    };
}

#endif //SR_ENGINE_SPOTLIGHT_H
