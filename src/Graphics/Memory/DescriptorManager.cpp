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
        const uint8_t frameIndex = m_pipeline->GetCurrentFrameIndex();

        if (reallocation != SR_ID_INVALID) {
            auto&& descriptors = m_descriptorPool.At(reallocation);
            for (auto&& descriptor : descriptors) {
                if (descriptor.descriptorSet == SR_ID_INVALID) {
                    continue;
                }
                m_pipeline->FreeDescriptorSet(&descriptor.descriptorSet);
            }

            descriptors.clear();

            DescriptorSetInfo& info = descriptors.emplace_back();
            info.pShaderHandle = pShaderHandle;
            info.frameIndex = frameIndex;
            info.descriptorSet = descriptorSet;

            return reallocation;
        }

        return m_descriptorPool.Add({ DescriptorSetInfo{ pShaderHandle, frameIndex, descriptorSet } });
    }

    DescriptorManager::BindResult DescriptorManager::Bind(DescriptorManager::VirtualDescriptorSet virtualDescriptorSet) {
        if (virtualDescriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::Bind() : descriptor set is invalid!");
            return BindResult::Failed;
        }

        auto&& info = m_descriptorPool.At(virtualDescriptorSet);
        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        const uint8_t frameIndex = m_pipeline->GetCurrentFrameIndex();

        DescriptorSet descriptorSet;
        bool hasDescriptorSet = false;

        const DescriptorSetInfo* pElement = info.data();
        const DescriptorSetInfo* pEnd = pElement + info.size();
        for (; pElement != pEnd; ++pElement) {
            if (m_multiFrameMode && pElement->frameIndex != frameIndex) {
                continue;
            }

            if (pElement->pShaderHandle == pShaderHandle) SR_LIKELY_ATTRIBUTE {
                descriptorSet = pElement->descriptorSet;
                hasDescriptorSet = true;
                break;
            }
        }

        BindResult result = BindResult::Success;

        if (!hasDescriptorSet) SR_UNLIKELY_ATTRIBUTE {
            descriptorSet = AllocateMemory(m_pipeline->GetCurrentShader());

            if (descriptorSet == SR_ID_INVALID && !m_allocationTypesCache.empty()) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("DescriptorManager::Bind() : failed to allocate descriptor set!");
                return BindResult::Failed;
            }

            DescriptorSetInfo& descriptorSetInfo = info.emplace_back();
            descriptorSetInfo.pShaderHandle = pShaderHandle;
            descriptorSetInfo.frameIndex = frameIndex;
            descriptorSetInfo.descriptorSet = descriptorSet;

            result = BindResult::Duplicated;
        }

        if (descriptorSet != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            if (!m_pipeline->BindDescriptorSet(descriptorSet)) {
                SR_ERROR("DescriptorManager::Bind() : failed to bind descriptor set!");
                return BindResult::Failed;
            }
        }

        return result;
    }

    void DescriptorManager::Flush() {
        auto&& pShader = m_pipeline->GetCurrentShader();
        pShader->AttachDescriptorSets();
    }

    DescriptorManager::DescriptorSet DescriptorManager::AllocateMemory(SR_GTYPES_NS::Shader* pShader) const {
        m_allocationTypesCache.clear();

        if (pShader->GetUBOBlockSize() > 0) SR_LIKELY_ATTRIBUTE {
            m_allocationTypesCache.emplace_back(DescriptorType::Uniform);
        }
        else if (pShader->GetSamplersCount() > 0) {
            m_allocationTypesCache.emplace_back(DescriptorType::CombinedImage);
        }
        else if (pShader->HasSSBOBindings()) {
            m_allocationTypesCache.emplace_back(DescriptorType::Storage);
        }

        if (pShader->HasSharedUBO()) {
            m_allocationTypesCache.emplace_back(DescriptorType::Uniform);
        }

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

    bool DescriptorManager::FreeDescriptorSet(DescriptorManager::VirtualDescriptorSet* pVirtualDescriptorSet) {
        SR_TRACY_ZONE;

        SRAssert(pVirtualDescriptorSet);

        if (*pVirtualDescriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::FreeDescriptorSet() : descriptor set is invalid!");
            return false;
        }

        auto&& info = m_descriptorPool.RemoveByIndex(*pVirtualDescriptorSet);
        for (auto&& descriptor : info) {
            if (descriptor.descriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }
            m_pipeline->FreeDescriptorSet(&descriptor.descriptorSet);
        }

        *pVirtualDescriptorSet = SR_ID_INVALID;
        return true;
    }

    void DescriptorManager::CollectUnused() {
        SR_TRACY_ZONE;

        if (m_descriptorPool.IsEmpty()) {
            return;
        }

        m_pipeline->GetShaderHandles(m_handles);

        uint32_t count = 0;

        m_descriptorPool.ForEach([&](VirtualDescriptorSet, std::vector<DescriptorSetInfo>& descriptorSetInfos) {
            for (auto pIt = descriptorSetInfos.begin(); pIt != descriptorSetInfos.end(); ) {
                DescriptorSetInfo& data = *pIt;

                if (!std::ranges::binary_search(m_handles, data.pShaderHandle)) SR_UNLIKELY_ATTRIBUTE {
                    if (data.descriptorSet != SR_ID_INVALID) {
                        m_pipeline->FreeDescriptorSet(&data.descriptorSet);
                    }
                    pIt = descriptorSetInfos.erase(pIt);
                    ++count;
                }
                else {
                    ++pIt;
                }
            }
        });

        if (count > 0) {
            SR_LOG("DescriptorManager::CollectUnused() : collected {} unused descriptors.", count);
        }
    }

    void DescriptorManager::InitSingleton() {
        Super::InitSingleton();
        m_multiFrameMode = SR_UTILS_NS::Features::Instance().Enabled("MultiFrameResources", true);
    }
}