//
// Created by Nikita on 13.12.2020.
//

#ifndef SR_ENGINE_DIRECTIONALLIGHT_H
#define SR_ENGINE_DIRECTIONALLIGHT_H

#include <Graphics/Lighting/ILightComponent.h>

namespace SR_GRAPH_NS {
    class DirectionalLight : public ILightComponent {
        using Super = ILightComponent;
        SR_CLASS()
    public:
        SR_NODISCARD LightType GetLightType() const override { return LightType::Directional; };

    };
}

#endif //SR_ENGINE_DIRECTIONALLIGHT_H
