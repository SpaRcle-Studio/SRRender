//
// Created by Monika on 22.05.2023.
//

#ifndef SR_ENGINE_LIGHTSYSTEM_H
#define SR_ENGINE_LIGHTSYSTEM_H

#include <Graphics/Lighting/LightType.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Types/SharedPtr.h>

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
        void OnLightTransformChanged(ILightComponent* pLightComponent);

        SR_NODISCARD SR_MATH_NS::FVector3 GetDirectionalLightDirection() const noexcept;

    public:
        RenderScenePtr m_renderScene;

        std::array<std::set<ILightComponent*>, SR_UTILS_NS::EnumTraits<LightType>::NumItems> m_lights;

    private:
        SR_MATH_NS::FVector3 m_directionalLightDir;

    };
}

#endif //SR_ENGINE_LIGHTSYSTEM_H
