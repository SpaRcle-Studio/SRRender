//
// Created by Monika on 24.04.2024.
//

#include <Utils/Profile/TracyContext.h>

#include <Graphics/Memory/SSBOManager.h>
#include <Graphics/Memory/ShaderProgramManager.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SSBOManager::VirtualSSBO SSBOManager::AllocateSSBO(uint32_t size, SSBOUsage usage) {
        SR_TRACY_ZONE;

        VirtualSSBOInfo ssbo;

        if (!AllocateMemory(ssbo, size, usage, true)) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::AllocateSSBO() : failed to allocate memory!");
            return SR_ID_INVALID;
        }

        return m_ssboPool.Add(std::move(ssbo));
    }

    SSBOManager::VirtualSSBO SSBOManager::ReAllocateSSBO(VirtualSSBO virtualSSBO, uint32_t size, SSBOUsage usage) {
        SR_TRACY_ZONE;

        auto&& ssbo = m_ssboPool.At(virtualSSBO);
        FreeMemory(ssbo);

        if (!AllocateMemory(ssbo, size, usage, true)) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::AllocateSSBO() : failed to allocate memory!");
            return SR_ID_INVALID;
        }

        return virtualSSBO;
    }

    bool SSBOManager::FreeSSBO(VirtualSSBO* pSSBO) {
        SR_TRACY_ZONE;
        auto&& ssbo = m_ssboPool.Remove(*pSSBO);
        *pSSBO = SR_ID_INVALID;
        return FreeMemory(ssbo);
    }

    SSBOManager::BindResult SSBOManager::BindSSBO(VirtualSSBO ssbo) noexcept {
        SR_TRACY_ZONE;

        auto&& [pShaderHandle, shaderProgram] = GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::BindSSBO() : shader handle is nullptr!");
            return BindResult::Failed;
        }

        auto&& ssboInfo = m_ssboPool.At(ssbo);

        m_pipeline->BindSSBO(ssboInfo.SSBO);

        for (auto&& [shader, descriptor] : ssboInfo.descriptors) {
            if (shader == pShaderHandle) SR_LIKELY_ATTRIBUTE {
                m_pipeline->BindDescriptorSet(descriptor);
                return BindResult::Success;
            }
        }

        if (!AllocateMemory(ssboInfo, 0, SSBOUsage::Unknown, false)) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::BindSSBO() : failed to allocate memory!");
            return BindResult::Failed;
        }

        return BindResult::Duplicated;
    }

    std::pair<void*, int32_t> SSBOManager::GetCurrentShaderHandle() const {
        SR_TRACY_ZONE;

        auto&& pShader = m_pipeline->GetCurrentShader();
        if (!pShader) SR_UNLIKELY_ATTRIBUTE {
            return { };
        }

        auto&& shaderProgram = pShader->GetId();
        if (shaderProgram == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            return { };
        }

        return std::make_pair(
            reinterpret_cast<void*>(SR_GRAPH_NS::Memory::ShaderProgramManager::Instance().GetProgram(shaderProgram)),
            shaderProgram
        );
    }

    bool SSBOManager::AllocateMemory(SSBOManager::VirtualSSBOInfo& memory, uint32_t size, SSBOUsage usage, bool allocSSBO) const {
        SR_TRACY_ZONE;

        auto&& [pShaderHandle, shaderProgram] = GetCurrentShaderHandle();
        if (!pShaderHandle) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("SSBOManager::AllocateMemory() : shader handle is nullptr!");
            return false;
        }

        auto&& shaderIdStash = m_pipeline->GetCurrentShaderId();

        {
            m_pipeline->SetCurrentShaderId(shaderProgram);

            if (allocSSBO) SR_LIKELY_ATTRIBUTE {
                SRAssert2(memory.SSBO == SR_ID_INVALID, "SSBOManager::AllocateMemory() : SSBO already allocated!");
                memory.SSBO = m_pipeline->AllocateSSBO(size, usage);
                if (memory.SSBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                    SRHalt("SSBOManager::AllocateMemory() : failed to allocate SSBO!");
                    goto failed;
                }
            }

            static std::vector<DescriptorType> type = {DescriptorType::Storage};
            auto&& descriptor = m_pipeline->AllocDescriptorSet(type);
            if (descriptor == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("SSBOManager::AllocateMemory() : failed to allocate descriptor set!");
                goto failed;
            }
            memory.descriptors.emplace_back(pShaderHandle, descriptor);
        }

        m_pipeline->SetCurrentShaderId(shaderIdStash);
        return true;

    failed:
        m_pipeline->SetCurrentShaderId(shaderIdStash);
        return false;
    }

    bool SSBOManager::FreeMemory(SSBOManager::VirtualSSBOInfo& memory) const {
        SR_TRACY_ZONE;
        m_pipeline->FreeSSBO(&memory.SSBO);
        for (auto&& [shader, descriptor] : memory.descriptors) {
            m_pipeline->FreeDescriptorSet(&descriptor);
        }
        memory.descriptors.clear();
        return true;
    }
}

