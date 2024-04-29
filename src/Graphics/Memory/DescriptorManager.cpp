//
// Created by Monika on 27.04.2024.
//

#include <Graphics/Memory/DescriptorManager.h>

namespace SR_GRAPH_NS {
    DescriptorManager::VirtualDescriptorSet DescriptorManager::AllocateDescriptorSet(VirtualDescriptorSet reallocation) {
        SR_TRACY_ZONE;

        if (!m_pipeline) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::AllocateDescriptorSet() : pipeline is nullptr!");
            return SR_ID_INVALID;
        }

        auto&& pShader = m_pipeline->GetCurrentShader();
        if (!pShader) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::AllocateDescriptorSet() : shader is nullptr!");
            return SR_ID_INVALID;
        }

        auto&& descriptorSet = AllocateMemory(pShader);
        if (descriptorSet == SR_ID_INVALID && !m_allocationTypesCache.empty()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::AllocateDescriptorSet() : failed to allocate descriptor set!");
            return SR_ID_INVALID;
        }

        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();

        if (reallocation != SR_ID_INVALID) {
            auto&& descriptors = m_descriptorPool.At(reallocation);
            for (auto&& descriptor : descriptors) {
                if (descriptor.descriptorSet == SR_ID_INVALID) {
                    continue;
                }
                m_pipeline->FreeDescriptorSet(&descriptor.descriptorSet);
            }
            descriptors.clear();
            descriptors.emplace_back(pShaderHandle, descriptorSet);
            return reallocation;
        }

        return m_descriptorPool.Add({ DescriptorSetInfo{ pShaderHandle, descriptorSet } });
    }

    DescriptorManager::BindResult DescriptorManager::Bind(DescriptorManager::VirtualDescriptorSet virtualDescriptorSet) {
        SR_TRACY_ZONE;

        if (virtualDescriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::Bind() : descriptor set is invalid!");
            return BindResult::Failed;
        }

        auto&& info = m_descriptorPool.At(virtualDescriptorSet);

        auto&& pShader = m_pipeline->GetCurrentShader();
        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();

        std::optional<DescriptorSet> descriptorSet;

        BindResult result = BindResult::Success;

        for (auto&& [shaderHandle, descriptorSetInfo] : info) {
            if (shaderHandle == pShaderHandle) SR_LIKELY_ATTRIBUTE {
                descriptorSet = descriptorSetInfo;
                break;
            }
        }

        if (!descriptorSet.has_value()) SR_UNLIKELY_ATTRIBUTE {
            descriptorSet = AllocateMemory(pShader);

            if (descriptorSet == SR_ID_INVALID && !m_allocationTypesCache.empty()) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("DescriptorManager::Bind() : failed to allocate descriptor set!");
                return BindResult::Failed;
            }

            info.emplace_back(pShaderHandle, descriptorSet.value());
            result = BindResult::Duplicated;
        }

        if (descriptorSet.value() != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            if (!m_pipeline->BindDescriptorSet(descriptorSet.value())) {
                SR_ERROR("DescriptorManager::Bind() : failed to bind descriptor set!");
                return BindResult::Failed;
            }

            if (m_pipeline->GetCurrentBuildIteration() == 0) SR_LIKELY_ATTRIBUTE {
                pShader->AttachDescriptorSets();
                pShader->FlushConstants();
            }
        }

        return result;
    }

    DescriptorManager::DescriptorSet DescriptorManager::AllocateMemory(SR_GTYPES_NS::Shader* pShader) const {
        m_allocationTypesCache.clear();

        if (pShader->GetUBOBlockSize() > 0) SR_LIKELY_ATTRIBUTE {
            m_allocationTypesCache.emplace_back(DescriptorType::Uniform);
        }
        else if (pShader->GetSamplersCount() > 0) {
            m_allocationTypesCache.emplace_back(DescriptorType::CombinedImage);
        }

        if (pShader->HasSharedUBO()) {
            m_allocationTypesCache.emplace_back(DescriptorType::Uniform);
        }

        /// TODO: Implement storage buffer support
        /// if (pShader->GetStorageBuffersCount() > 0) {
        ///     m_allocationTypesCache.emplace_back(DescriptorType::Storage);
        /// }

        if (m_allocationTypesCache.empty()) SR_UNLIKELY_ATTRIBUTE {
            return SR_ID_INVALID;
        }

        const DescriptorSet descriptorSet = m_pipeline->AllocDescriptorSet(m_allocationTypesCache);
        if (descriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::AllocateMemory() : failed to allocate descriptor set!");
            return SR_ID_INVALID;
        }

        return descriptorSet;
    }

    void DescriptorManager::FreeDescriptorSet(DescriptorManager::VirtualDescriptorSet* pVirtualDescriptorSet) {
        SR_TRACY_ZONE;

        SRAssert(pVirtualDescriptorSet);

        if (*pVirtualDescriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::FreeDescriptorSet() : descriptor set is invalid!");
            return;
        }

        auto&& info = m_descriptorPool.RemoveByIndex(*pVirtualDescriptorSet);
        for (auto&& descriptor : info) {
            if (descriptor.descriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }
            m_pipeline->FreeDescriptorSet(&descriptor.descriptorSet);
        }

        *pVirtualDescriptorSet = SR_ID_INVALID;
    }
}