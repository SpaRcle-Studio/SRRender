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

        SR_NODISCARD const DirectionalLightParams& GetParams() const;

        void SetSkyColors(const SR_MATH_NS::FColor& sunsetSky, const SR_MATH_NS::FColor& daySky, const SR_MATH_NS::FColor& groundSky);
        void SetCelestialBody(CelestialBodyShading shading, float_t angularDiameter, float_t diskIntensity, const SR_MATH_NS::FColor& tint);
        void SetIntensity(float_t intensity) { m_intensity = intensity; UpdateLightParams(); }
        void SetTemperature(float_t temperature) { m_temperature = temperature; UpdateLightParams(); }
        void SetDiskIntensity(float_t diskIntensity) { m_diskIntensity = diskIntensity; UpdateLightParams(); }

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

        /// @property @onChanged(UpdateLightParams) @group(CelestialBody)
        CelestialBodyShading m_shading = CelestialBodyShading::Emission;
        /// @property @onChanged(UpdateLightParams) @group(CelestialBody)
        float_t m_angularDiameter = 0.53f;
        /// @property @onChanged(UpdateLightParams) @group(CelestialBody)
        float_t m_diskIntensity = 1.f;
        /// @property @onChanged(UpdateLightParams) @group(CelestialBody)
        SR_MATH_NS::FColor m_diskTint = SR_MATH_NS::FColor(1.f);

    private:
        DirectionalLightParams m_params;

    };
}

#endif //SR_ENGINE_DIRECTIONALLIGHT_H
