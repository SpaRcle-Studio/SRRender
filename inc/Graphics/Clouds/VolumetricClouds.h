//
// Created by Monika on 06.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_VOLUMETRIC_CLOUDS_H
#define SR_ENGINE_GRAPHICS_VOLUMETRIC_CLOUDS_H

#include <Utils/ECS/Component.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Math/Vector3.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderScene;

    /// Объемные облака. Рендерятся только при наличии активного компонента на сцене.
    /// @displayName(Volumetric Clouds) @category(Render)
    class VolumetricClouds : public SR_UTILS_NS::Component {
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
        void RegisterClouds();
        void UnregisterClouds();

        SR_NODISCARD RenderScenePtr TryGetRenderScene() const;

    private:
        /// @property @group(Shape)
        float_t m_coverage = 0.45f;
        /// @property @group(Shape)
        float_t m_density = 0.04f;
        /// @property @group(Shape)
        float_t m_cloudType = 0.5f;
        /// @property @group(Shape)
        float_t m_shapeScale = 1.f;
        /// @property @group(Shape)
        float_t m_detailScale = 1.f;
        /// @property @group(Shape)
        float_t m_detailStrength = 0.35f;

        /// @property @group(Layer)
        float_t m_bottomHeight = 1500.f;
        /// @property @group(Layer)
        float_t m_thickness = 2500.f;
        /// @property @group(Layer)
        float_t m_maxDistance = 50000.f;

        /// @property @group(Wind)
        SR_MATH_NS::FVector2 m_windDirection = SR_MATH_NS::FVector2(1.f, 0.3f);
        /// @property @group(Wind)
        float_t m_windSpeed = 12.f;

        /// @property @group(Lighting)
        float_t m_lightAbsorption = 1.f;
        /// @property @group(Lighting)
        float_t m_ambientStrength = 1.f;
        /// @property @group(Lighting)
        float_t m_silverLining = 0.6f;
        /// @property @group(Lighting)
        float_t m_powderStrength = 0.6f;

        /// @property @group(Quality)
        uint32_t m_primarySteps = 48;
        /// @property @group(Quality)
        uint32_t m_lightSteps = 5;

        bool m_isRegistered = false;
        mutable RenderScenePtr m_renderScene;

    };
}

#endif //SR_ENGINE_GRAPHICS_VOLUMETRIC_CLOUDS_H
