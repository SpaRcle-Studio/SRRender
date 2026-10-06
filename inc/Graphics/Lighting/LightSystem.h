//
// Created by Monika on 22.05.2023.
//

#ifndef SR_ENGINE_LIGHTSYSTEM_H
#define SR_ENGINE_LIGHTSYSTEM_H

#include <Graphics/Lighting/LightType.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Types/SharedPtr.h>

#include <Enum/LightType.hpp>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class ILightComponent;

    class LightSystem : SR_UTILS_NS::NonCopyable {
        using Super = SR_UTILS_NS::NonCopyable;
    public:
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::RenderScene>;

        explicit LightSystem(RenderScenePtr pRenderScene);
        ~LightSystem() override;

        void Register(ILightComponent* pLightComponent);
        void Remove(ILightComponent* pLightComponent);
        void OnLightChanged(ILightComponent* pLightComponent);

        SR_NODISCARD const DirectionalLightParams& GetDirectionalLightParams() const noexcept;
        SR_NODISCARD const DirectionalLightParams& GetSecondaryDirectionalLightParams() const noexcept;
        SR_NODISCARD bool HasSecondaryDirectionalLight() const noexcept { return m_hasSecondaryDirectionalLight; }

        /// параметры неба: второй источник, диски светил, звезды
        void UseSkyUniforms(SR_GTYPES_NS::Shader& shader) const;

    private:
        void UpdateDirectionalLights();

    public:
        RenderScenePtr m_renderScene;

        std::array<SR_UTILS_NS::Set<ILightComponent*>, SR_UTILS_NS::EnumTraits<LightType>::NumItems> m_lights;

    private:
        DirectionalLightParams m_directionalLightParams;
        DirectionalLightParams m_secondaryDirectionalLightParams;
        bool m_hasSecondaryDirectionalLight = false;

    };
}

#endif //SR_ENGINE_LIGHTSYSTEM_H
