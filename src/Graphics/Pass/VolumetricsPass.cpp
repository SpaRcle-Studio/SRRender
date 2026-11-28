//
// Created by Monika on 28.11.2026.
//

#include <Graphics/Pass/VolumetricsPass.h>
#include <Graphics/Clouds/VolumetricClouds.h>
#include <Graphics/Fog/VolumetricFog.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/VolumetricsPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool VolumetricsPass::Init() {
        SetShader("Engine/Shaders/Volumetrics/volumetrics.srsl");

        if (!m_shadowPassName.Empty()) {
            if (auto&& pMaterialData = GetMaterial()->GetMaterialData()) {
                pMaterialData->AddShaderDefine("SR_FOG_SHADOWS");
            }
        }

        return Super::Init();
    }

    bool VolumetricsPass::Render() {
        /// без компонентов буфер остается очищенным в (0, 0, 0, 1) - эффектов нет
        if (!GetRenderScene()->GetVolumetricClouds() && !GetRenderScene()->GetVolumetricFog()) {
            return false;
        }
        return Super::Render();
    }

    void VolumetricsPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        static const SR_UTILS_NS::StringAtom flagsName = "volumetricsFlags";

        auto&& pClouds = GetRenderScene()->GetVolumetricClouds();
        auto&& pFog = GetRenderScene()->GetVolumetricFog();

        if (pClouds) {
            pClouds->UseUniforms(shader);
        }

        if (pFog) {
            pFog->UseUniforms(shader);
        }

        /// x - облака, y - туман и атмосферная перспектива. Ветвление по uniform: компонент можно
        /// выключить без перекомпиляции шейдера
        shader.SetVec4(flagsName, SR_MATH_NS::FVector4(pClouds ? 1.f : 0.f, pFog ? 1.f : 0.f, 0.f, 0.f));

        if (pFog && !m_shadowPassName.Empty()) {
            if (auto&& pShadowPass = GetTechnique()->FindPass(m_shadowPassName)) {
                pShadowPass->UseUniformsFromAnotherPass(shader);
            }
        }
    }
}
