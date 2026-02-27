//
// Created by Monika on 27.04.2024.
//

#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Shader.h>

#include <Utils/Common/Features.h>

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
        const uint8_t frameIndex = m_multiFrameMode ? m_pipeline->GetCurrentImageIndex() : 0;

        if (reallocation != SR_ID_INVALID) {
            auto&& descriptors = m_descriptorPool.At(reallocation);
            for (auto&& descriptor : descriptors) {
                const uint8_t maxFramesInFlight = m_pipeline->GetSwapchainImagesCount();
                for (uint8_t i = 0; i < maxFramesInFlight; ++i) {
                    if (descriptor.descriptorSets[i] != SR_ID_INVALID) {
                        m_pipeline->FreeDescriptorSet(&descriptor.descriptorSets[i]);
                    }
                }
            }

            descriptors.clear();

            DescriptorSetInfo& info = descriptors.emplace_back();
            std::ranges::fill(info.descriptorSets, SR_ID_INVALID);
            info.pShaderHandle = pShaderHandle;
            info.descriptorSets[frameIndex] = descriptorSet;

            return reallocation;
        }

        DescriptorSetInfo info;
        std::ranges::fill(info.descriptorSets, SR_ID_INVALID);
        info.pShaderHandle = pShaderHandle;
        info.descriptorSets[frameIndex] = descriptorSet;

        std::vector<DescriptorSetInfo> descriptorSetInfos;
        descriptorSetInfos.reserve(256);
        descriptorSetInfos.emplace_back(info);

        return m_descriptorPool.Add(std::move(descriptorSetInfos));
    }

    DescriptorManager::BindResult DescriptorManager::Bind(DescriptorManager::VirtualDescriptorSet virtualDescriptorSet) {
        SR_TRACY_ZONE;

        if (virtualDescriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("DescriptorManager::Bind() : descriptor set is invalid!");
            return BindResult::Failed;
        }

        auto&& info = m_descriptorPool.At(virtualDescriptorSet);
        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        const uint8_t frameIndex = m_multiFrameMode ? m_pipeline->GetCurrentImageIndex() : 0;

        DescriptorSet descriptorSet = SR_ID_INVALID;
        bool hasDescriptorSet = false;

        BindResult result = BindResult::Success;

        DescriptorSetInfo* pElement = info.data();
        const DescriptorSetInfo* pEnd = pElement + info.size();
        for (; pElement != pEnd; ++pElement) {
            if (pElement->pShaderHandle == pShaderHandle) SR_LIKELY_ATTRIBUTE {
                if (pElement->descriptorSets[frameIndex] == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                    pElement->descriptorSets[frameIndex] = AllocateMemory(m_pipeline->GetCurrentShader());

                    if (pElement->descriptorSets[frameIndex] == SR_ID_INVALID && !m_allocationTypesCache.empty()) SR_UNLIKELY_ATTRIBUTE {
                        SRHalt("DescriptorManager::Bind() : failed to allocate descriptor set!");
                        return BindResult::Failed;
                    }
                    result = BindResult::Duplicated;
                }
                descriptorSet = pElement->descriptorSets[frameIndex];
                hasDescriptorSet = true;
                break;
            }
        }

        if (!hasDescriptorSet) SR_UNLIKELY_ATTRIBUTE {
            descriptorSet = AllocateMemory(m_pipeline->GetCurrentShader());

            if (descriptorSet == SR_ID_INVALID && !m_allocationTypesCache.empty()) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("DescriptorManager::Bind() : failed to allocate descriptor set!");
                return BindResult::Failed;
            }

            DescriptorSetInfo& descriptorSetInfo = info.emplace_back();
            std::ranges::fill(descriptorSetInfo.descriptorSets, SR_ID_INVALID);
            descriptorSetInfo.pShaderHandle = pShaderHandle;
            descriptorSetInfo.descriptorSets[frameIndex] = descriptorSet;

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
        SR_TRACY_ZONE;

        m_allocationTypesCache.clear();

        if (pShader->GetUBOBlockSize() > 0) SR_LIKELY_ATTRIBUTE {
            m_allocationTypesCache.emplace_back(DescriptorType::Uniform);
        }
        if (pShader->GetSamplersCount() > 0) {
            m_allocationTypesCache.emplace_back(DescriptorType::CombinedImage);
        }
        if (pShader->HasSSBOBindings()) {
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

        const uint8_t maxFramesInFlight = m_pipeline->GetSwapchainImagesCount();

        auto&& info = m_descriptorPool.RemoveByIndex(*pVirtualDescriptorSet);
        for (auto&& descriptor : info) {
            for (uint8_t i = 0; i < maxFramesInFlight; ++i) {
                if (descriptor.descriptorSets[i] != SR_ID_INVALID) {
                    m_pipeline->FreeDescriptorSet(&descriptor.descriptorSets[i]);
                }
            }
        }

        *pVirtualDescriptorSet = SR_ID_INVALID;
        return true;
    }

    void DescriptorManager::CollectUnused() {
        SR_TRACY_ZONE;

        if (m_descriptorPool.IsEmpty()) {
            return;
        }

        const uint8_t maxFramesInFlight = m_pipeline->GetSwapchainImagesCount();

        m_pipeline->GetShaderHandles(m_handles);

        uint32_t count = 0;

        m_descriptorPool.ForEach([&](VirtualDescriptorSet, std::vector<DescriptorSetInfo>& descriptorSetInfos) {
            for (auto pIt = descriptorSetInfos.begin(); pIt != descriptorSetInfos.end(); ) {
                DescriptorSetInfo& data = *pIt;

                if (!std::ranges::binary_search(m_handles, data.pShaderHandle)) SR_UNLIKELY_ATTRIBUTE {
                    for (uint8_t i = 0; i < maxFramesInFlight; ++i) {
                        if (data.descriptorSets[i] != SR_ID_INVALID) {
                            m_pipeline->FreeDescriptorSet(&data.descriptorSets[i]);
                        }
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

    bool DescriptorManager::TryFreeDescriptorSet(DescriptorManager::VirtualDescriptorSet *pVirtualDescriptorSet) {
        SR_TRACY_ZONE;
        if (!pVirtualDescriptorSet || *pVirtualDescriptorSet == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }
        return FreeDescriptorSet(pVirtualDescriptorSet);
    }
}