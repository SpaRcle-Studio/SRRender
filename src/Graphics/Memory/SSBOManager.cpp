//
// Created by Monika on 24.04.2024.
//

#include <Utils/Profile/TracyContext.h>

#include <Graphics/Memory/SSBOManager.h>
#include <Graphics/Memory/ShaderProgramManager.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SSBOManager::VirtualSSBO SSBOManager::AllocateSSBO(VirtualSSBO virtualSSBO, uint32_t size, SSBOUsage usage) {
        SR_TRACY_ZONE;

        SSBOManager::SSBO* pSSBO = nullptr;

        if (virtualSSBO != SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            auto&& ssbo = m_ssboPool.At(virtualSSBO);
            FreeMemory(&ssbo);
        }

        if (!AllocateMemory(pSSBO, size, usage)) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::AllocateSSBO() : failed to allocate memory!");
            return SR_ID_INVALID;
        }

        return virtualSSBO;
    }

    bool SSBOManager::FreeSSBO(VirtualSSBO* pSSBO) {
        SR_TRACY_ZONE;
        auto&& ssbo = m_ssboPool.RemoveByIndex(*pSSBO);
        *pSSBO = SR_ID_INVALID;
        return FreeMemory(&ssbo);
    }

    SSBOManager::BindResult SSBOManager::BindSSBO(VirtualSSBO virtualSSBO) noexcept {
        SR_TRACY_ZONE;

        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::BindSSBO() : shader handle is nullptr!");
            return BindResult::Failed;
        }

        auto&& ssbo = m_ssboPool.At(virtualSSBO);
        m_pipeline->BindSSBO(ssbo);

        return BindResult::Success;
    }

    bool SSBOManager::AllocateMemory(SSBOManager::SSBO* pSSBO, uint32_t size, SSBOUsage usage) const {
        SR_TRACY_ZONE;

        SRAssert2(*pSSBO == SR_ID_INVALID, "SSBOManager::AllocateMemory() : SSBO already allocated!");

        auto&& pShaderHandle = m_pipeline->GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::AllocateMemory() : shader handle is nullptr!");
            return false;
        }

        *pSSBO = m_pipeline->AllocateSSBO(size, usage);
        if (*pSSBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::AllocateMemory() : failed to allocate SSBO!");
            return false;
        }

        return true;
    }

    bool SSBOManager::FreeMemory(SSBOManager::SSBO* pSSBO) const {
        SR_TRACY_ZONE;
        m_pipeline->FreeSSBO(pSSBO);
        return true;
    }
}

