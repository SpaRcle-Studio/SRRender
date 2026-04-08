//
// Created by Monika on 06.06.2023.
//

#ifndef SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H
#define SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H

#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Utils/Frustum.h>

#include <Utils/Math/Matrix4x4.h>

namespace SR_GRAPH_NS {
    class CascadedShadowMapPass : public MeshDrawerPass {
        SR_CLASS()
        using Super = MeshDrawerPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<CascadedShadowMapPass>;

    public:
        SR_NODISCARD const std::vector<SR_MATH_NS::Matrix4x4>& GetCascadeMatrices() const { return m_cascadeMatrices; }
        SR_NODISCARD const std::vector<float_t>& GetSplitDepths() const { return m_cascadeSplitDepths; }

    public:
        bool Init() override;
        bool Render() override;
        void Update() override;
        bool Prepare() override;
        void PostUpdate() override;
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;
        void UseConstants(SR_GTYPES_NS::Shader& shader) override;
        void UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader& shader) override;

        void SetCascadeCount(uint32_t count) { m_cascadeCount = count; }
        void SetInstancing(bool enabled) { m_instancing = enabled; }
        void SetOneMeterUnit(float_t unit) { m_oneMeterUnit = unit; }
        void SetMaxShadowDistance(float_t distance) { m_maxShadowDistance = distance; }
        void SetSplitDepths(float_t split1, float_t split2, float_t split3) { m_split1 = split1; m_split2 = split2; m_split3 = split3; }
        void SetLightFrustumCount(uint32_t count) { m_lightFrustumCount = count; }

        SR_NODISCARD RenderQueuePtr AllocateRenderQueue(uint32_t index) override;

    protected:
        SR_NODISCARD SR_GTYPES_NS::Camera* CheckCamera();
        void UpdateCascades(SR_GTYPES_NS::Camera* pCamera);
        void UpdateCascadesUnityStyle(SR_GTYPES_NS::Camera* pCamera);

        void OnCameraParamsChanged() override;
        void UpdateShaderDefines(SR_SRSL_NS::ShaderParams& params) const override;

        SR_NODISCARD const Frustum& GetFrustum(uint32_t renderLayer, bool& isAvailable) const override;
        SR_NODISCARD float_t GetCascadedMapResolution() const;

    protected:
        SR_MATH_NS::FVector3 m_directionalLightDirection;
        SR_MATH_NS::FVector3 m_cameraPosition;
        SR_MATH_NS::UVector2 m_screenSize;
        SR_MATH_NS::Quaternion m_cameraRotation;
        bool m_cameraDirty = true;

        /// @property
        float_t m_near = 0.f;
        /// @property
        float_t m_far = 0.f;
        /// @property
        bool m_instancing = false;
        /// @property
        uint32_t m_lightFrustumCount = 2;
        /// @property
        uint32_t m_cascadeCount = 4;
        /// @property
        float_t m_oneMeterUnit = 1.f;
        /// @property
        float_t m_maxShadowDistance = 500.f;
        /// @property
        float_t m_split1 = 25.f;
        /// @property
        float_t m_split2 = 75.f;
        /// @property
        float_t m_split3 = 150.f;

        struct LastCascadeData {
            float_t radius = 1.f;
            float_t extentX = 1.f;
            float_t extentY = 1.f;
            SR_MATH_NS::FVector3 center;
            SR_MATH_NS::FVector3 minLS;
            SR_MATH_NS::FVector3 maxLS;
        };

        std::vector<LastCascadeData> m_lastCascadeData;
        int32_t m_drawCascadeIndex = -1;
        std::vector<SR_MATH_NS::Matrix4x4> m_cascadeMatrices;
        std::vector<Frustum> m_lightFrustums;
        std::vector<float_t> m_cascadeRadii;
        std::vector<float_t> m_cascadeSplitDepths;

    };
}

#endif //SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H
