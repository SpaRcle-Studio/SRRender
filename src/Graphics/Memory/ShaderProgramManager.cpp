//
// Created by Monika on 11.07.2022.
//

#include <Utils/Common/Numeric.h>

#include <Graphics/Memory/ShaderProgramManager.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <EvoVulkan/Tools/VulkanDebug.h>

namespace SR_GRAPH_NS::Memory {
    ShaderProgramManager::ShaderProgramManager()
        : SR_UTILS_NS::Singleton<ShaderProgramManager>()
    {
        m_programs.reserve(1024);
    }

    void ShaderProgramManager::OnSingletonDestroy() {
        Singleton::OnSingletonDestroy();
    }

    ShaderProgramManager::VirtualProgram ShaderProgramManager::Allocate(const SRShaderCreateInfo& createInfo) {
        SR_TRACY_ZONE;

        VirtualProgramInfo virtualProgramInfo;
        virtualProgramInfo.m_createInfo = createInfo;

        if (auto&& shaderProgramInfo = AllocateShaderProgram(createInfo); shaderProgramInfo.Valid()) SR_LIKELY_ATTRIBUTE {
            virtualProgramInfo.SetProgramInfo(GetCurrentIdentifier(), shaderProgramInfo);
        }
        else {
            SR_ERROR("ShaderProgramManager::Allocate() : failed to allocate shader program!");
            return SR_ID_INVALID;
        }

        VirtualProgram virtualProgram;
        if (!m_unusedPrograms.empty()) SR_LIKELY_ATTRIBUTE {
            virtualProgram = m_unusedPrograms.back();
            m_unusedPrograms.pop_back();
            m_programs[virtualProgram] = std::move(virtualProgramInfo);
        }
        else {
            virtualProgram = static_cast<VirtualProgram>(m_programs.size());
            m_programs.emplace_back(std::move(virtualProgramInfo));
        }

        return virtualProgram;
    }

    ShaderProgramManager::VirtualProgram ShaderProgramManager::ReAllocate(VirtualProgram program, const SRShaderCreateInfo& createInfo) {
        SR_TRACY_ZONE;

        if (program == SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            return Allocate(createInfo);
        }

        if (program >= static_cast<VirtualProgram>(m_programs.size())) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::ReAllocate() : virtual program not found!");
            return SR_ID_INVALID;
        }

        auto&& virtualProgramInfo = m_programs[program];

        if (!virtualProgramInfo.Valid()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::ReAllocate() : virtual program is invalid!");
            return SR_ID_INVALID;
        }

        /// EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

        /// очишаем старые шейдерные программы
        for (auto&& [fbo /** unused */, shaderProgramInfo] : virtualProgramInfo.m_data) {
            m_pipeline->FreeShader(&shaderProgramInfo.id);
        }
        virtualProgramInfo.m_data.clear();

        /// обновляем данные
        virtualProgramInfo.m_createInfo = SRShaderCreateInfo(createInfo);

        if (auto&& shaderProgramInfo = AllocateShaderProgram(createInfo); shaderProgramInfo.Valid()) {
            virtualProgramInfo.SetProgramInfo(GetCurrentIdentifier(), shaderProgramInfo);
            /// EVK_POP_LOG_LEVEL();
        }
        else {
            SR_ERROR("ShaderProgramManager::ReAllocate() : failed to allocate shader program!");
            /// EVK_POP_LOG_LEVEL();
            return SR_ID_INVALID;
        }

        return program;
    }

    ShaderBindResult ShaderProgramManager::BindProgram(VirtualProgram virtualProgram) noexcept {
        if (virtualProgram >= static_cast<VirtualProgram>(m_programs.size())) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::BindProgram() : invalid virtual program range!");
            return ShaderBindResult::Failed;
        }

        ShaderBindResult result = ShaderBindResult::Success;

        auto&& virtualProgramInfo = m_programs[virtualProgram];
        if (!virtualProgramInfo.Valid()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::BindProgram() : invalid virtual program!");
            return ShaderBindResult::Failed;
        }

        auto&& identifier = GetCurrentIdentifier();

        auto&& pProgramInfo = virtualProgramInfo.GetProgramInfo(identifier);
        if (!pProgramInfo) SR_UNLIKELY_ATTRIBUTE {
            if (auto&& shaderProgramInfo = AllocateShaderProgram(virtualProgramInfo.m_createInfo); shaderProgramInfo.Valid()) {
                pProgramInfo = virtualProgramInfo.SetProgramInfo(identifier, shaderProgramInfo);
                result = ShaderBindResult::Duplicated;
            }
            else {
                SR_ERROR("ShaderProgramManager::BindProgram() : failed to allocate shader program!");
                return ShaderBindResult::Failed;
            }
        }

        auto&& bindResult = BindShaderProgram(*pProgramInfo, virtualProgramInfo.m_createInfo);

        if (bindResult == ShaderBindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("ShaderProgramManager::BindProgram() : failed to bind shader program!");
            return ShaderBindResult::Failed;
        }

        if (result == ShaderBindResult::Duplicated) SR_UNLIKELY_ATTRIBUTE {
            return result;
        }

        return bindResult;
    }

    bool ShaderProgramManager::FreeProgram(ShaderProgramManager::VirtualProgram program) {
        return FreeProgram(&program);
    }

    bool ShaderProgramManager::FreeProgram(VirtualProgram *program) {
        SR_TRACY_ZONE;

        if (!SRVerifyFalse(!program)) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        if (static_cast<uint64_t>(*program) >= m_programs.size()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::FreeProgram() : invalid virtual program range!");
            return false;
        }

        auto&& virtualProgramInfo = m_programs[*program];
        if (!virtualProgramInfo.Valid()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::FreeProgram() : invalid virtual program!");
            return false;
        }

        for (auto&& [fbo /** unused */, shaderProgramInfo] : virtualProgramInfo.m_data) {
            m_pipeline->FreeShader(&shaderProgramInfo.id);
        }

        m_programs[*program] = VirtualProgramInfo();
        m_unusedPrograms.emplace_back(*program);
        *program = SR_ID_INVALID;

        return true;
    }

    bool ShaderProgramManager::IsAvailable(ShaderProgramManager::VirtualProgram virtualProgram) const noexcept {
        SR_TRACY_ZONE;

        if (static_cast<uint64_t>(virtualProgram) >= m_programs.size()) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        auto&& virtualProgramInfo = m_programs[virtualProgram];
        if (!virtualProgramInfo.Valid()) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        return virtualProgramInfo.HasProgram(GetCurrentIdentifier());
    }

    ShaderProgramManager::ShaderProgram ShaderProgramManager::GetProgram(VirtualProgram virtualProgram) const noexcept {
        SR_TRACY_ZONE;

        if (static_cast<uint64_t>(virtualProgram) >= m_programs.size()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::GetProgram() : invalid virtual program range!");
            return SR_ID_INVALID;
        }

        auto&& virtualProgramInfo = m_programs[virtualProgram];
        if (!virtualProgramInfo.Valid()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::GetProgram() : invalid virtual program!");
            return SR_ID_INVALID;
        }

        if (auto&& pInfo = virtualProgramInfo.GetProgramInfo(GetCurrentIdentifier())) SR_LIKELY_ATTRIBUTE {
            return pInfo->id;
        }

        SRHalt("ShaderProgramManager::GetProgram() : framebuffer not found!");
        return SR_ID_INVALID;
    }

    VirtualProgramInfo::ShaderProgramInfo ShaderProgramManager::AllocateShaderProgram(const SRShaderCreateInfo &createInfo) const {
        SR_TRACY_ZONE;

        SRAssert(m_pipeline);

        /// Выделяем новую шейдерную программу
        auto&& frameBufferId = m_pipeline->GetCurrentFrameBufferId();

        auto&& shaderProgram = m_pipeline->AllocateShaderProgram(createInfo, frameBufferId);
        if (shaderProgram == SR_ID_INVALID) {
            SR_ERROR("ShaderProgramManager::AllocateShaderProgram() : failed to allocate shader program!");
            return VirtualProgramInfo::ShaderProgramInfo(); /// NOLINT
        }

        VirtualProgramInfo::ShaderProgramInfo shaderProgramInfo;
        shaderProgramInfo.id = shaderProgram;

        if (auto&& pFrameBuffer = m_pipeline->GetCurrentFrameBuffer()) {
            shaderProgramInfo.samples = pFrameBuffer->GetSamplesCount();
            shaderProgramInfo.depth = pFrameBuffer->IsDepthEnabled();
        }
        else {
            shaderProgramInfo.samples = m_pipeline->GetSamplesCount();
            shaderProgramInfo.depth = createInfo.blendEnabled;
        }

        return shaderProgramInfo;
    }

    ShaderBindResult ShaderProgramManager::BindShaderProgram(VirtualProgramInfo::ShaderProgramInfo &shaderProgramInfo, const SRShaderCreateInfo& createInfo) { /// NOLINT
        if (auto&& pFrameBuffer = m_pipeline->GetCurrentFrameBuffer())
        {
            if (pFrameBuffer->IsDepthEnabled() != shaderProgramInfo.depth || pFrameBuffer->GetSamplesCount() != shaderProgramInfo.samples)
            {
                SR_LOG("ShaderProgramManager::BindShaderProgram() : the frame buffer parameters have been changed, the shader has been recreated...");

                m_pipeline->FreeShader(&shaderProgramInfo.id);

                if ((shaderProgramInfo = AllocateShaderProgram(createInfo)).Valid()) {
                    if (BindShaderProgram(shaderProgramInfo, createInfo) == ShaderBindResult::Success) {
                        return ShaderBindResult::ReAllocated;
                    }
                    else {
                        SR_ERROR("ShaderProgramManager::BindShaderProgram() : unexcepted result!");
                        return ShaderBindResult::Failed;
                    }
                }
                else {
                    SR_ERROR("ShaderProgramManager::BindShaderProgram() : failed to allocate shader program!");
                    return ShaderBindResult::Failed;
                }
            }

            m_pipeline->UseShader(static_cast<ShaderProgram>(shaderProgramInfo.id));
        }
        else {
            m_pipeline->UseShader(static_cast<ShaderProgram>(shaderProgramInfo.id));
        }

        return ShaderBindResult::Success;
    }

    VirtualProgramInfo::Identifier ShaderProgramManager::GetCurrentIdentifier() const {
        return reinterpret_cast<VirtualProgramInfo::Identifier>(m_pipeline->GetCurrentFBOHandle());
    }

    const VirtualProgramInfo* ShaderProgramManager::GetInfo(ShaderProgramManager::VirtualProgram virtualProgram) const noexcept {
        if (static_cast<uint64_t>(virtualProgram) >= m_programs.size()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::GetProgram() : invalid virtual program range!");
            return nullptr;
        }

        auto&& virtualProgramInfo = m_programs[virtualProgram];
        if (!virtualProgramInfo.Valid()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("ShaderProgramManager::GetProgram() : invalid virtual program!");
            return nullptr;
        }

        return &virtualProgramInfo;
    }

    bool ShaderProgramManager::HasProgram(ShaderProgramManager::VirtualProgram virtualProgram) const noexcept {
        if (static_cast<uint64_t>(virtualProgram) >= m_programs.size()) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        return m_programs[virtualProgram].Valid();
    }

    void ShaderProgramManager::CollectUnusedShaders() {
        SR_TRACY_ZONE;

        if (m_programs.size() - m_unusedPrograms.size() == 0) {
            return;
        }

        auto&& handles = m_pipeline->GetFBOHandles();

        uint32_t count = 0;

        /// не удаляем сами виртуальные программы, они могут еще пригодиться
        for (auto&& virtualProgram : m_programs) {
            if (virtualProgram.m_data.empty()) {
                continue;
            }

            for (auto pIt = virtualProgram.m_data.begin(); pIt != virtualProgram.m_data.end(); ) {
                auto&& [identifier, program] = *pIt;

                if (handles.count(reinterpret_cast<void*>(identifier)) == 0) {
                    m_pipeline->FreeShader(&program.id);
                    pIt = virtualProgram.m_data.erase(pIt);
                    ++count;
                }
                else {
                    ++pIt;
                }
            }
        }

        if (count > 0) {
            SR_LOG("ShaderProgramManager::CollectUnusedShaders() : collected {} unused shaders.", count);
        }
    }
}

