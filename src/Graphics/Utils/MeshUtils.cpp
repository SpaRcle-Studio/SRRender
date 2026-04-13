//
// Created by Monika on 20.03.2023.
//

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>

namespace SR_GRAPH_NS {
    void DrawRenderObject(SR_GTYPES_NS::IRenderComponent* pObject,
        uint32_t indices,
        int32_t& ubo, int32_t& descriptor,
        bool& dirtyMaterial, bool& hasErrors
    ) {
        auto&& pPipeline = pObject->GetPipeline();
        if (dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            ubo = pPipeline->GetUBOManager().AllocateUBO(ubo);
            if (ubo == SR_INVALID_UBO) SR_UNLIKELY_ATTRIBUTE {
                hasErrors = true;
                return;
            }

            descriptor = pPipeline->GetDescriptorManager().AllocateDescriptorSet(descriptor);
        }

        SRAssert(ubo != SR_INVALID_UBO);

        pPipeline->GetUBOManager().BindUBO(ubo);

        const auto result = pPipeline->GetDescriptorManager().Bind(descriptor);

        if (result == DescriptorManager::BindResult::Duplicated || dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            pObject->UseSamplers(*pPipeline->GetCurrentShader());
            pObject->UseSSBO();
            pObject->MarkUniformsDirty();
            pPipeline->GetDescriptorManager().Flush();
        }
        pPipeline->GetCurrentShader()->FlushConstants();

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            if (pObject->GetIBO().has_value()) {
                pPipeline->DrawIndices(indices);
            }
            else {
                pPipeline->Draw(indices);
            }
        }

        dirtyMaterial = false;
    }
}
