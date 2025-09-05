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
        void UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh) override;
        void UseSharedUniforms(SR_GTYPES_NS::Shader* pShader) override;
        void UseConstants(SR_GTYPES_NS::Shader* pShader) override;
        void UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader* pShader) override;

    protected:
        bool CheckCamera();
        void UpdateCascades();

    protected:
        SR_MATH_NS::FVector3 m_directionalLightPosition;
        SR_MATH_NS::FVector3 m_cameraPosition;
        SR_MATH_NS::Quaternion m_cameraRotation;
        SR_MATH_NS::UVector2 m_screenSize;

        /// @property
        float_t m_near = 0.1f;
        /// @property
        float_t m_far = 100.f;
        /// @property
        float_t m_cascadeSplitLambda = 0.95f;
        /// @property
        bool m_usePerspective = false;

        std::vector<SR_MATH_NS::Matrix4x4> m_cascadeMatrices;
        std::vector<float_t> m_cascadeSplitDepths;

    };
}

#endif //SR_ENGINE_CASCADED_SHADOW_MAP_PASS_H
