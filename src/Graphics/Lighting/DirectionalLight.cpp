//
// Created by Nikita on 13.12.2020.
//

#include <Graphics/Lighting/DirectionalLight.h>

#include <Utils/ECS/Transform.h>
#include <Utils/Math/Curve.h>

#include <Codegen/DirectionalLight.generated.hpp>

namespace SR_GRAPH_NS {
    const DirectionalLightParams& DirectionalLight::GetParams() const {
        return m_params;
    }

    void DirectionalLight::SetSkyColors(const SR_MATH_NS::FColor& sunsetSky, const SR_MATH_NS::FColor& daySky, const SR_MATH_NS::FColor& groundSky) {
        m_sunsetSky = sunsetSky;
        m_daySkyColor = daySky;
        m_groundSky = groundSky;
        UpdateLightParams();
    }

    void DirectionalLight::SetCelestialBody(CelestialBodyShading shading, float_t angularDiameter, float_t diskIntensity, const SR_MATH_NS::FColor& tint) {
        m_shading = shading;
        m_angularDiameter = angularDiameter;
        m_diskIntensity = diskIntensity;
        m_diskTint = tint;
        UpdateLightParams();
    }

    void DirectionalLight::UpdateLightParamsImpl() {
        // ------------------------------------------------------------
        // Direction
        // ------------------------------------------------------------
        m_params.direction = GetTransform()->Forward().Normalize();

        const float_t sunHeight = SR_MATH_NS::Clamp(
            SR_MATH_NS::Dot(-m_params.direction, SR_MATH_NS::FVector3::Up()),
            0.0f,
            1.0f
        ) + m_skyHeightOffset;

        // ------------------------------------------------------------
        // Base sun color (Kelvin + Filter)
        // ------------------------------------------------------------
        const SR_MATH_NS::FColor tempColor = SR_MATH_NS::KelvinToRGB(m_temperature);
        SR_MATH_NS::FColor sunBaseColor = tempColor * m_filter;

        // ------------------------------------------------------------
        // Atmospheric absorption
        // ------------------------------------------------------------
        const float redLoss   = 1.0f;
        const float greenLoss = SR_MATH_NS::Mix(0.7f, 1.0f, sunHeight);
        const float blueLoss  = SR_MATH_NS::Mix(0.2f, 1.0f, sunHeight);

        const SR_MATH_NS::FColor atmosphereAbsorb(
            redLoss,
            greenLoss,
            blueLoss
        );

        SR_MATH_NS::FColor sunColor = sunBaseColor * atmosphereAbsorb;

        // ------------------------------------------------------------
        // Desaturate sun on sunset
        // ------------------------------------------------------------
        const float saturation = SR_MATH_NS::Mix(m_saturationMin, m_saturationMax, sunHeight);
        const float luminance = sunColor.x * 0.2126f + sunColor.y * 0.7152f + sunColor.z * 0.0722f;

        const SR_MATH_NS::FColor gray(luminance);

        sunColor = SR_MATH_NS::Mix(gray, sunColor, saturation);

        if (m_interactsWithSky) {
            // Чем ниже солнце — тем БЕЛЕЕ direct light
            const float skyInfluence = SR_MATH_NS::Curve::SmoothStep(0.0f, 0.35f, sunHeight);
            sunColor = SR_MATH_NS::Mix(
                SR_MATH_NS::FColor(1.0f),
                sunColor,
                skyInfluence
            );
        }

        m_params.lightColor = sunColor;

        // ------------------------------------------------------------
        // Intensity (SOFTER curve, Unity-like)
        // ------------------------------------------------------------
        /// сумерки: свет под горизонтом плавно гаснет (0..-10 градусов), а не обрывается
        const float_t rawHeight = SR_MATH_NS::Dot(-m_params.direction, SR_MATH_NS::FVector3::Up());
        const float_t twilight = SR_MATH_NS::Curve::SmoothStep(-0.17f, 0.02f, rawHeight);

        /// у горизонта прямой свет не обрывается в ноль, а затухает вместе с сумерками
        const float visibility = SR_MATH_NS::Max(SR_MATH_NS::Pow(SR_MATH_NS::Max(sunHeight, 0.f), 0.6f), 0.15f * twilight) * twilight;
        m_params.intensity = m_intensity * visibility;
        /// вклад в окружение (небо/ambient) - для плавного смешивания солнца и луны в LightSystem
        m_params.environmentWeight = m_intensity * twilight;
        m_params.skyColor = SR_MATH_NS::Mix(m_sunsetSky, m_daySkyColor, sunHeight);

        if (m_interactsWithSky) {
            // Усиление горизонта
            const float horizon = 1.0f - sunHeight;
            const float horizonBoost = SR_MATH_NS::Curve::SmoothStep(0.0f, 0.5f, horizon);

            m_params.skyColor *= SR_MATH_NS::Mix(1.0f, 1.3f, horizonBoost);
        }

        //m_params.skyColor = SR_MATH_NS::Mix(
        //    SR_MATH_NS::FVector3(1.2f, 0.45f, 0.25f),
        //    SR_MATH_NS::FVector3(0.45f, 0.6f, 0.9f),
        //    sunHeight
        //);

        // ------------------------------------------------------------
        // Ground bounce
        // ------------------------------------------------------------

        m_params.groundColor = m_params.skyColor * m_groundSky;

        // ------------------------------------------------------------
        // Ambient
        // ------------------------------------------------------------
        if (m_interactsWithSky) {
            // Ambient доминирует (как в Unity)
            m_params.ambientIntensity = SR_MATH_NS::Mix(1.4f, 1.0f, sunHeight);
        }
        else {
            m_params.ambientIntensity = SR_MATH_NS::Mix(1.2f, 1.0f, sunHeight);
        }

        m_params.shadowStrength = SR_MATH_NS::Mix(m_shadowMin, m_shadowMax, sunHeight);

        // ------------------------------------------------------------
        // Celestial body (диск на небе)
        // ------------------------------------------------------------
        m_params.shading = m_shading;
        m_params.angularDiameter = m_angularDiameter;
        m_params.diskIntensity = m_diskIntensity;
        m_params.diskColor = sunBaseColor * m_diskTint;
        m_params.skyIlluminance = sunBaseColor * m_intensity;
    }
}
