//
// Created by Monika on 13.12.2020.
//

#ifndef SR_ENGINE_ILIGHTCOMPONENT_H
#define SR_ENGINE_ILIGHTCOMPONENT_H

#include <Graphics/Lighting/LightType.h>

#include <Utils/ECS/Component.h>

namespace SR_GRAPH_NS {
    class RenderScene;

    /// @hidden @abstract @category(Render.Light)
    class ILightComponent : public SR_UTILS_NS::Component {
        using Super = SR_UTILS_NS::Component;
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::RenderScene>;
        SR_CLASS()
    public:
        SR_NODISCARD SR_FORCE_INLINE bool ExecuteInEditMode() const override { return true; }
        SR_NODISCARD bool IsUpdatable() const noexcept override { return false; }

        SR_NODISCARD virtual LightType GetLightType() const {
            SRHalt("Abstract method called!");
            return LightType::LightTypeMAX;
        }

        void OnAttached() override;
        void OnDestroy() override;

        void OnEnable() override;
        void OnDisable() override;

        void OnPostLoad() override;
        void OnMatrixDirty() override;

        void UpdateLightParams();

    private:
        void RegisterLight();
        void UnregisterLight();
        virtual void UpdateLightParamsImpl() { }

        SR_NODISCARD RenderScenePtr TryGetRenderScene() const;
        SR_NODISCARD RenderScenePtr GetRenderScene() const;

    protected:
        /// @property @onChanged(UpdateLightParams)
        float_t m_intensity = 1.f;
        /// @property @onChanged(UpdateLightParams)
        float_t m_temperature = 6500.f;
        /// @property @onChanged(UpdateLightParams)
        SR_MATH_NS::FVector3 m_filter = SR_MATH_NS::FVector3::One();
        /// @property @onChanged(UpdateLightParams)
        ShadowType m_shadowType = ShadowType::Soft;

    protected:
        bool m_isLightRegistered = false;
        mutable RenderScenePtr m_renderScene;

    };
}

#endif //SR_ENGINE_ILIGHTCOMPONENT_H
