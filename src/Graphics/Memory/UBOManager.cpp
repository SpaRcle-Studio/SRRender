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
        data.ubo = ubo;
        data.pShaderHandle = pShaderHandle;
        data.uboSize = uboSize;

        if (virtualUbo == SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            return m_uboPool.Add(std::move(virtualUboInfo));
        }

        auto&& info = m_uboPool.At(virtualUbo);
        for (auto&& dataToFree : info.data) {
            if (dataToFree.uboSize <= 0) {
                continue;
            }
            m_pipeline->FreeUBO(&dataToFree.ubo);
        }
        info = std::move(virtualUboInfo);
        return virtualUbo;
    }

    bool UBOManager::FreeUBO(UBOManager::VirtualUBO* virtualUbo) {
        SR_TRACY_ZONE;

        SRAssert(virtualUbo != nullptr);

        auto&& info = m_uboPool.RemoveByIndex(*virtualUbo);
        for (auto&& data : info.data) {
            if (data.uboSize <= 0) {
                SRAssert(data.ubo == SR_ID_INVALID);
                continue;
            }
            m_pipeline->FreeUBO(&data.ubo);
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
        auto&& uboSize = m_pipeline->GetCurrentShader()->GetUBOBlockSize();
        return BindUBO(virtualUbo, uboSize);
    }

    UBOManager::BindResult UBOManager::BindUBO(VirtualUBO virtualUbo, uint32_t uboSize) noexcept {
        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHaltOnce("Current shader is nullptr!");
            return BindResult::Failed;
        }

        auto&& info = m_uboPool.At(virtualUbo);
        BindResult result = BindResult::Success;

        UBO ubo = SR_ID_INVALID;
        bool isFound = false;

        for (auto&& data : info.data) {
            if (data.pShaderHandle == pShaderHandle || info.shared) SR_LIKELY_ATTRIBUTE {
                ubo = data.ubo;
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
            data.ubo = ubo;
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
            SRHaltOnce("Current shader is nullptr!");
            return BindResult::Failed;
        }

        auto&& info = m_uboPool.At(virtualUbo);

        for (auto&& data : info.data) {
            if (data.pShaderHandle == pShaderHandle || info.shared) SR_LIKELY_ATTRIBUTE {
                /// SR_ID_INVALID is allowed
                m_pipeline->BindUBO(data.ubo);
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

        auto&& info = m_uboPool.At(virtualUbo);
        for (auto&& data : info.data) {
            if (data.pShaderHandle == pShaderHandle || info.shared) SR_LIKELY_ATTRIBUTE {
                return data.ubo;
            }
        }

        return SR_ID_INVALID;
    }

    void UBOManager::CollectUnused() {
        SR_TRACY_ZONE;

        if (m_uboPool.IsEmpty()) {
            return;
        }

        auto&& handles = m_pipeline->GetShaderHandles();

        uint32_t count = 0;

        m_uboPool.ForEach([&](VirtualUBO, VirtualUBOInfo& virtualUboInfo) {
            for (auto pIt = virtualUboInfo.data.begin(); pIt != virtualUboInfo.data.end(); ) {
                VirtualUBOInfo::Data& data = *pIt;

                if (handles.count(data.pShaderHandle) == 0) {
                    if (data.uboSize > 0) {
                        m_pipeline->FreeUBO(&data.ubo);
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
}
