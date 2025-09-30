//
// Created by Monika on 06.06.2023.
//

#ifndef SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H
#define SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H

#include <Graphics/Pass/MeshDrawerPass.h>

#include <Utils/Math/Matrix4x4.h>

namespace SR_GRAPH_NS {
    class CascadedShadowMapPass : public MeshDrawerPass {
        SR_CLASS()
        using Super = MeshDrawerPass;
    public:
        SR_NODISCARD const std::vector<SR_MATH_NS::Matrix4x4>& GetCascadeMatrices() const { return m_cascadeMatrices; }
        SR_NODISCARD const std::vector<float_t>& GetSplitDepths() const { return m_cascadeSplitDepths; }

    public:
        bool Init() override;
        bool Render() override;
        void Update() override;
        void Prepare() override;
        void PostUpdate() override;
        void UseSharedUniforms(SR_GTYPES_NS::Shader* pShader) override;
        void UseConstants(SR_GTYPES_NS::Shader* pShader) override;
        void UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader* pShader) override;

        SR_NODISCARD RenderQueuePtr AllocateRenderQueue(uint32_t index) override;

    protected:
        bool CheckCamera();
        void UpdateCascades();

        void UpdateShaderDefines(SR_SRSL_NS::ShaderMacrosParams& defines) const override;

        SR_NODISCARD const Frustum& GetFrustum(uint32_t renderLayer) const override;

    protected:
        SR_MATH_NS::FVector3 m_directionalLightDirection;
        SR_MATH_NS::FVector3 m_cameraPosition;
        SR_MATH_NS::UVector2 m_screenSize;
        SR_MATH_NS::Quaternion m_cameraRotation;

        /// @property
        float_t m_near = 0.1f;
        /// @property
        float_t m_far = 100.f;
        /// @property
        float_t m_cascadeSplitLambda = 0.95f;
        /// @property
        bool m_instancing = false;
        /// @property
        uint32_t m_lightFrustumCount = 2;
        /// @property
        uint32_t m_cascadeCount = 4;

        int32_t m_drawCascadeIndex = -1;
        std::vector<SR_MATH_NS::Matrix4x4> m_cascadeMatrices;
        std::vector<Frustum> m_lightFrustums;
        std::vector<float_t> m_cascadeSplitDepths;
        std::vector<float_t> m_cascadeRadii;

    };
}

#endif //SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H
