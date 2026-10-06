//
// Created by Monika on 06.10.2026.
//

#include <Graphics/Pass/VolumetricFogPass.h>
#include <Graphics/Fog/VolumetricFog.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/VolumetricFogPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool VolumetricFogPass::Init() {
        SetShader("Engine/Shaders/Fog/volumetric-fog.srsl");

        if (!m_shadowPassName.Empty()) {
            if (auto&& pMaterialData = GetMaterial()->GetMaterialData()) {
                pMaterialData->AddShaderDefine("SR_FOG_SHADOWS");
            }
        }

        return Super::Init();
    }

    bool VolumetricFogPass::Render() {
        /// без компонента буфер остается очищенным в (0, 0, 0, 1) - тумана нет
        if (!GetRenderScene()->GetVolumetricFog()) {
            return false;
        }
        return Super::Render();
    }

    void VolumetricFogPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        if (auto&& pFog = GetRenderScene()->GetVolumetricFog()) {
            pFog->UseUniforms(shader);
        }

        if (!m_shadowPassName.Empty()) {
            if (auto&& pShadowPass = GetTechnique()->FindPass(m_shadowPassName)) {
                pShadowPass->UseUniformsFromAnotherPass(shader);
            }
        }
    }
}
