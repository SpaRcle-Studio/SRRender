//
// Created by Monika on 13.12.2020.
//

#ifndef SR_ENGINE_ILIGHTCOMPONENT_H
#define SR_ENGINE_ILIGHTCOMPONENT_H

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Lighting/LightType.h>

namespace SR_GRAPH_NS {
    class RenderScene;

    /// @hidden @abstract @category(Render.Light)
    class ILightComponent : public SR_GTYPES_NS::IRenderComponent {
        using Super = SR_GTYPES_NS::IRenderComponent;
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

        void OnMatrixDirty() override;

        void UpdateLightParams();

    private:
        void RegisterLight();
        void UnregisterLight();
        virtual void UpdateLightParamsImpl() { }

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


    };
}

#endif //SR_ENGINE_ILIGHTCOMPONENT_H
