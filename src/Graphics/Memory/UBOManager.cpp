//
// Created by Monika on 10.06.2022.
//

#include <Utils/Common/Features.h>

#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS::Memory {
    UBOManager::UBOManager()
        : Super()
    {
        m_uboPool.Reserve(4096);
    }

    UBOManager::~UBOManager() {
        m_pipeline.Reset();
    }

    UBOManager::VirtualUBO UBOManager::AllocateUBO(VirtualUBO virtualUbo) {
        auto&& pShader = m_pipeline->GetCurrentShader();
        if (!pShader) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("UBOManager::AllocateUBO() : shader is nullptr!");
            return SR_ID_INVALID;
        }
        return AllocateUBO(virtualUbo, pShader->GetUBOBlockSize(), false);
    }

    UBOManager::VirtualUBO UBOManager::AllocateUBO(VirtualUBO virtualUbo, uint32_t uboSize) {
        return AllocateUBO(virtualUbo, uboSize, false);
    }

    UBOManager::VirtualUBO UBOManager::AllocateUBO(VirtualUBO virtualUbo, uint32_t uboSize, bool shared) {
        SR_TRACY_ZONE;

        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        const uint8_t frameIndex = m_multiFrameMode ? m_pipeline->GetCurrentFrameIndex() : 0;
        const uint8_t maxFramesInFlight = m_pipeline->GetSwapchainImagesCount();

        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("UBOManager::AllocateUBO() : shader program do not set!");
            return SR_ID_INVALID;
        }

        UBO ubo = SR_ID_INVALID;

        if (uboSize > 0) SR_LIKELY_ATTRIBUTE {
            if (!AllocMemory(&ubo, uboSize)) SR_UNLIKELY_ATTRIBUTE {
                SR_ERROR("UBOManager::AllocateUBO() : failed to allocate memory!");
                return SR_ID_INVALID;
            }
        }

        VirtualUBOInfo virtualUboInfo;
        virtualUboInfo.shared = shared;

        VirtualUBOInfo::Data& data = virtualUboInfo.data.emplace_back();
        std::ranges::fill(data.ubos, SR_ID_INVALID);
        data.ubos[frameIndex] = ubo;
        data.pShaderHandle = pShaderHandle;
        data.uboSize = uboSize;

        if (virtualUbo == SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            return m_uboPool.Add(std::move(virtualUboInfo));
        }

        if (!m_uboPool.IsAlive(virtualUbo)) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("UBOManager::AllocateUBO() : virtual UBO is not alive!");
            return SR_ID_INVALID;
        }

        auto&& info = m_uboPool.AtUnchecked(virtualUbo);
        for (auto&& dataToFree : info.data) {
            if (dataToFree.uboSize <= 0) {
                continue;
            }

            for (uint8_t i = 0; i < maxFramesInFlight; ++i) {
                if (dataToFree.ubos[i] != SR_ID_INVALID) {
                    m_pipeline->FreeUBO(&dataToFree.ubos[i]);
                }
            }
        }
        info = std::move(virtualUboInfo);
        return virtualUbo;
    }

    bool UBOManager::FreeUBO(UBOManager::VirtualUBO* virtualUbo) {
        SR_TRACY_ZONE;

        SRAssert(virtualUbo != nullptr);

        const uint8_t maxFramesInFlight = m_pipeline->GetSwapchainImagesCount();

        auto&& info = m_uboPool.RemoveByIndex(*virtualUbo);
        for (auto&& data : info.data) {
            if (data.uboSize <= 0) {
                continue;
            }

            for (uint8_t i = 0; i < maxFramesInFlight; ++i) {
                if (data.ubos[i] != SR_ID_INVALID) {
                    m_pipeline->FreeUBO(&data.ubos[i]);
                }
            }
        }

        *virtualUbo = SR_ID_INVALID;

        return true;
    }

    bool UBOManager::AllocMemory(UBO *ubo, uint32_t uboSize) {
        SR_TRACY_ZONE;

        if (*ubo = m_pipeline->AllocateUBO(uboSize); *ubo < 0) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("UBOManager::AllocMemory() : failed to allocate uniform buffer object!");
            return false;
        }

        return true;
    }

    UBOManager::BindResult UBOManager::BindUBO(VirtualUBO virtualUbo) noexcept {
        if (!m_pipeline->GetCurrentShader()) SR_UNLIKELY_ATTRIBUTE {
            SRHaltOnce("Current shader is nullptr!");
            return BindResult::Failed;
        }
        auto&& uboSize = m_pipeline->GetCurrentShader()->GetUBOBlockSize();
        return BindUBO(virtualUbo, uboSize);
    }

    UBOManager::BindResult UBOManager::BindUBO(VirtualUBO virtualUbo, uint32_t uboSize) noexcept {
        SR_TRACY_ZONE;

        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHaltOnce("Current shader is nullptr!");
            return BindResult::Failed;
        }

        const uint8_t frameIndex = m_multiFrameMode ? m_pipeline->GetCurrentFrameIndex() : 0;

        auto&& info = m_uboPool.At(virtualUbo);
        BindResult result = BindResult::Success;

        UBO ubo = SR_ID_INVALID;
        bool isFound = false;

        for (auto&& data : info.data) {
            if (data.pShaderHandle == pShaderHandle || info.shared) SR_LIKELY_ATTRIBUTE {
                if (data.ubos[frameIndex] == SR_ID_INVALID && data.uboSize > 0) SR_UNLIKELY_ATTRIBUTE {
                    if (!AllocMemory(&data.ubos[frameIndex], data.uboSize)) SR_UNLIKELY_ATTRIBUTE {
                        SR_ERROR("UBOManager::BindUBO() : failed to allocate memory!");
                        return BindResult::Failed;
                    }
                    result = BindResult::Duplicated;
                }

                ubo = data.ubos[frameIndex];
                isFound = true;
                break;
            }
        }

        if (!isFound) SR_UNLIKELY_ATTRIBUTE {
            SRAssert2(!info.shared, "Something went wrong! UBO not found in shared mode!");

            if (uboSize > 0) SR_LIKELY_ATTRIBUTE {
                if (!AllocMemory(&ubo, uboSize)) SR_UNLIKELY_ATTRIBUTE {
                    SR_ERROR("UBOManager::BindUBO() : failed to allocate memory!");
                    return BindResult::Failed;
                }
            }

            VirtualUBOInfo::Data& data = info.data.emplace_back();
            std::ranges::fill(data.ubos, SR_ID_INVALID);
            data.ubos[frameIndex] = ubo;
            data.pShaderHandle = pShaderHandle;
            data.uboSize = uboSize;

            result = BindResult::Duplicated;
        }

        /// SR_ID_INVALID is allowed
        m_pipeline->BindUBO(ubo);

        return result;
    }

    UBOManager::BindResult UBOManager::BindNoDublicateUBO(VirtualUBO virtualUbo) noexcept {
        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            return BindResult::Failed;
        }

        const uint8_t frameIndex = m_multiFrameMode ? m_pipeline->GetCurrentFrameIndex() : 0;

        auto&& info = m_uboPool.At(virtualUbo);

        for (auto&& data : info.data) {
            if (data.pShaderHandle == pShaderHandle || info.shared) SR_LIKELY_ATTRIBUTE {
                /// SR_ID_INVALID is allowed
                m_pipeline->BindUBO(data.ubos[frameIndex]);
                return BindResult::Success;
            }
        }

        return BindResult::Failed;
    }

    void UBOManager::SetPipeline(UBOManager::PipelinePtr pPipeline) {
        m_pipeline = std::move(pPipeline);
    }

    UBOManager::UBO UBOManager::GetUBO(UBOManager::VirtualUBO virtualUbo) const noexcept {
        SR_TRACY_ZONE;

        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        const uint8_t frameIndex = m_multiFrameMode ? m_pipeline->GetCurrentFrameIndex() : 0;

        auto&& info = m_uboPool.At(virtualUbo);
        for (auto&& data : info.data) {
            if (data.pShaderHandle == pShaderHandle || info.shared) SR_LIKELY_ATTRIBUTE {
                return data.ubos[frameIndex];
            }
        }

        return SR_ID_INVALID;
    }

    void UBOManager::CollectUnused() {
        SR_TRACY_ZONE;

        if (m_uboPool.IsEmpty()) {
            return;
        }

        m_pipeline->GetShaderHandles(m_handles);

        uint32_t count = 0;

        m_uboPool.ForEach([&](VirtualUBO, VirtualUBOInfo& virtualUboInfo) {
            for (auto pIt = virtualUboInfo.data.begin(); pIt != virtualUboInfo.data.end(); ) {
                VirtualUBOInfo::Data& data = *pIt;

                if (!std::ranges::binary_search(m_handles, data.pShaderHandle)) SR_UNLIKELY_ATTRIBUTE {
                    if (data.uboSize > 0) {
                        const uint8_t maxFramesInFlight = m_pipeline->GetSwapchainImagesCount();
                        for (uint8_t i = 0; i < maxFramesInFlight; ++i) {
                            if (data.ubos[i] != SR_ID_INVALID) {
                                m_pipeline->FreeUBO(&data.ubos[i]);
                            }
                        }
                    }
                    pIt = virtualUboInfo.data.erase(pIt);
                    ++count;
                }
                else {
                    ++pIt;
                }
            }
        });

        if (count > 0) {
            SR_LOG("UBOManager::CollectUnused() : collected {} unused UBO.", count);
        }
    }

    void UBOManager::InitSingleton() {
        Super::InitSingleton();
        m_multiFrameMode = SR_UTILS_NS::Features::Instance().Enabled("MultiFrameResources", true);
    }
}
