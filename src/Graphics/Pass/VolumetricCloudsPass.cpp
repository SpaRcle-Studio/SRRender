//
// Created by Monika on 06.10.2026.
//

#include <Graphics/Pass/VolumetricCloudsPass.h>
#include <Graphics/Clouds/VolumetricClouds.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/VolumetricCloudsPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool VolumetricCloudsPass::Init() {
        SetShader("Engine/Shaders/Clouds/volumetric-clouds.srsl");
        return Super::Init();
    }

    bool VolumetricCloudsPass::Render() {
        /// без компонента буфер остается очищенным в (0, 0, 0, 1) - облаков нет
        if (!GetRenderScene()->GetVolumetricClouds()) {
            return false;
        }
        return Super::Render();
    }

    void VolumetricCloudsPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        if (auto&& pClouds = GetRenderScene()->GetVolumetricClouds()) {
            pClouds->UseUniforms(shader);
        }
    }
}
