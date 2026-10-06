//
// Created by Monika on 06.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_VOLUMETRIC_FOG_H
#define SR_ENGINE_GRAPHICS_VOLUMETRIC_FOG_H

#include <Utils/ECS/Component.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Math/Vector4.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderScene;

    /// Глобальный высотный туман с объемными лучами от солнца/луны.
    /// Рендерится только при наличии активного компонента на сцене.
    /// @displayName(Volumetric Fog) @category(Render)
    class VolumetricFog : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        SR_NODISCARD bool IsUpdatable() const noexcept override { return false; }

        void OnEnable() override;
        void OnDisable() override;
        void OnDestroy() override;

        void UseUniforms(SR_GTYPES_NS::Shader& shader) const;

    private:
        void RegisterFog();
        void UnregisterFog();

        SR_NODISCARD RenderScenePtr TryGetRenderScene() const;

    private:
        /// @property @group(Density)
        float_t m_density = 0.02f;
        /// @property @group(Density)
        float_t m_baseHeight = 0.f;
        /// @property @group(Density)
        float_t m_heightFalloff = 0.08f;
        /// @property @group(Density)
        float_t m_maxDistance = 300.f;
        /// @property @group(Density)
        float_t m_startDistance = 0.f;

        /// @property @group(Color)
        SR_MATH_NS::FColor m_albedo = SR_MATH_NS::FColor(0.9f, 0.92f, 0.95f, 1.f);
        /// @property @group(Color)
        float_t m_ambientStrength = 1.f;
        /// @property @group(Color)
        float_t m_skyColorInfluence = 0.7f;

        /// @property @group(Lighting)
        float_t m_anisotropy = 0.6f;
        /// @property @group(Lighting)
        float_t m_lightShaftsIntensity = 1.f;
        /// @property @group(Lighting)
        bool m_shadows = true;

        /// @property @group(Noise)
        float_t m_noiseStrength = 0.5f;
        /// @property @group(Noise)
        float_t m_noiseScale = 0.05f;
        /// @property @group(Noise)
        SR_MATH_NS::FVector2 m_windDirection = SR_MATH_NS::FVector2(1.f, 0.2f);
        /// @property @group(Noise)
        float_t m_windSpeed = 1.5f;

        /// @property @group(Quality)
        uint32_t m_steps = 24;

        bool m_isRegistered = false;
        mutable RenderScenePtr m_renderScene;

    };
}

#endif //SR_ENGINE_GRAPHICS_VOLUMETRIC_FOG_H
