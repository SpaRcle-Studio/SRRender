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

    public:
        void UpdateLightParamsImpl() override;

        SR_NODISCARD DirectionalLightParams GetParams() const;

    private:
        /// @property @onChanged(UpdateLightParams)
        bool m_interactsWithSky = true;
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        SR_MATH_NS::FColor m_sunsetSky = SR_MATH_NS::FColor(0.06f, 0.0f, 0.0f);
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        SR_MATH_NS::FColor m_daySkyColor = SR_MATH_NS::FColor(0.514f, 0.734f, 0.997f);
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        SR_MATH_NS::FColor m_groundSky = SR_MATH_NS::FColor(0.7f, 0.6f, 0.5f);
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        float_t m_saturationMin = 0.5f;
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        float_t m_saturationMax = 1.0f;
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        float_t m_shadowMin = 0.6f;
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        float_t m_shadowMax = 0.9f;
        /// @property @onChanged(UpdateLightParams) @group(Sky)
        float_t m_skyHeightOffset = 0.0f;

    private:
        DirectionalLightParams m_params;

    };
}

#endif //SR_ENGINE_DIRECTIONALLIGHT_H
