//
// Created by Monika on 06.07.2025.
//

#include <Graphics/Memory/SSBO.h>

namespace SR_GRAPH_NS {
    SSBOInstance::SSBOInstance() = default;

    SSBOInstance::~SSBOInstance() {
        DeAllocate();
    }

    SSBOInstance::Ptr SSBOInstance::Create(
        uint64_t size,
        SSBOUsage usage,
        SR_UTILS_NS::StringAtom name,
        SSBOFlags flags
    ) {
        SR_TRACY_ZONE;
        SSBOInstance::Ptr pSSBO = std::unique_ptr<SSBOInstance>(new SSBOInstance());

        pSSBO->m_flags = flags;

        pSSBO->SetName(name);
        pSSBO->SetSizeAndUsage(size, usage);

        return pSSBO;
    }

    bool SSBOInstance::ReAllocate() {
        SR_TRACY_ZONE;

        auto&& pPipeline = GetPipeline();
        if (!pPipeline) {
            SR_ERROR("SSBOInstance::ReAllocate() : pPipeline is nullptr!");
            return false;
        }

        if (m_SSBO != SR_ID_INVALID) {
            pPipeline->FreeSSBO(&m_SSBO);
        }

        if (m_size == 0) {
            SR_ERROR("SSBOInstance::ReAllocate() : SSBO size is zero!");
            return false;
        }

        if (m_usage == SSBOUsage::Unknown) {
            SR_ERROR("SSBOInstance::ReAllocate() : SSBO usage is unknown!");
            return false;
        }

        m_SSBO = pPipeline->AllocateSSBO(m_size + GetCounterSize(), m_usage);

        return true;
    }

    Pipeline::Ptr SSBOInstance::GetPipeline() const noexcept {
        if (m_pipeline) {
            return m_pipeline;
        }

        SR_TRACY_ZONE;

        SRAssert2(SR_THIS_THREAD, "SSBOInstance::GetPipeline() : SR_THIS_THREAD is nullptr!");

        auto&& pRenderContext = SR_THIS_THREAD->GetContext()->GetValue<SR_GRAPH_NS::RenderContext::Ptr>();
        SRAssert2(pRenderContext, "SSBOInstance::GetPipeline() : pRenderContext is nullptr!");

        m_pipeline = pRenderContext->GetPipeline();
        SRAssert2(m_pipeline, "SSBOInstance::GetPipeline() : m_pipeline is nullptr!");

        return m_pipeline;
    }

    void SSBOInstance::DeAllocate() {
        if (m_mappedData) {
            SRHalt("SSBOInstance::DeAllocate() : SSBO is mapped! Unmap it before deallocating!");
        }

        if (m_SSBO != SR_ID_INVALID) {
            if (auto&& pPipeline = GetPipeline()) {
                pPipeline->FreeSSBO(&m_SSBO);
            }
            else {
                SRHalt("SSBOInstance::DeAllocate() : pPipeline is nullptr!");
            }
        }
    }

    void SSBOInstance::Resize(uint64_t size) {
        m_size = size;
        ReAllocate();
    }

    void SSBOInstance::SetUsage(SSBOUsage usage) {
        m_usage = usage;
        ReAllocate();
    }

    void SSBOInstance::SetSizeAndUsage(uint64_t size, SSBOUsage usage) {
        m_size = size;
        m_usage = usage;
        ReAllocate();
    }

    void SSBOInstance::SetName(SR_UTILS_NS::StringAtom name) {
        m_name = name;
    }

    bool SSBOInstance::Bind(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        auto&& pCurrentShader = GetPipeline()->GetCurrentShader();
        if (!pCurrentShader) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("SSBOInstance::Bind() : no active shader!");
            return false;
        }

        if (m_SSBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("SSBOInstance::Bind() : SSBO is not allocated!");
            return false;
        }

        if (!name.empty()) {
            pCurrentShader->BindSSBO(name, m_SSBO);
            return true;
        }

        if (!m_name.empty()) {
            pCurrentShader->BindSSBO(m_name, m_SSBO);
            return true;
        }

        SR_ERROR("SSBOInstance::Bind() : name is empty!");
        return false;
    }

    uint64_t SSBOInstance::GetCounterSize() const {
        if (HasCounter()) {
            if (IsStructured()) {
                return sizeof(uint32_t) * 4;
            }
            else {
                return sizeof(uint32_t);
            }
        }

        return 0;
    }

    bool SSBOInstance::HasCounter() const {
        return SR_MATH_NS::IsMaskIncludedSubMask(m_flags, SSBOFlags::Counter);
    }

    bool SSBOInstance::IsStructured() const {
        return SR_MATH_NS::IsMaskIncludedSubMask(m_flags, SSBOFlags::Structured);
    }

    uint32_t SSBOInstance::GetCounter() const {
        SR_TRACY_ZONE;

        if (!HasCounter()) {
            SR_ERROR("SSBOInstance::GetCounter() : SSBO does not have a counter!");
            return 0;
        }

        bool isNeedUnmap = m_mappedData == nullptr;

        if (isNeedUnmap) {
            if (!Map()) {
                SR_ERROR("SSBOInstance::GetCounter() : failed to map SSBO!");
                return 0;
            }
        }

        const uint32_t count = *reinterpret_cast<uint32_t*>(m_mappedData);

        if (isNeedUnmap) {
            UnMap();
        }

        return count;
    }

    void* SSBOInstance::Map() const {
        SR_TRACY_ZONE;

        if (m_mappedData) {
            SR_WARN("SSBOInstance::Map() : SSBO is already mapped!");
            return m_mappedData;
        }

        if (m_SSBO == SR_ID_INVALID) {
            SR_ERROR("SSBOInstance::Map() : SSBO is not allocated!");
            return nullptr;
        }

        if (auto&& pPipeline = GetPipeline()) {
            if (!pPipeline->MapSSBO(m_SSBO, &m_mappedData) || !m_mappedData) {
                SR_ERROR("SSBOInstance::Map() : failed to map SSBO!");
                return nullptr;
            }
            return m_mappedData;
        }

        SR_ERROR("SSBOInstance::Map() : pPipeline is nullptr!");
        return nullptr;
    }

    void SSBOInstance::UnMap() const {
        SR_TRACY_ZONE;

        if (!m_mappedData) {
            SR_WARN("SSBOInstance::UnMap() : SSBO is not mapped!");
            return;
        }

        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->UnMapSSBO(m_SSBO);
            m_mappedData = nullptr;
        }
        else {
            SR_ERROR("SSBOInstance::UnMap() : pPipeline is nullptr!");
        }
    }

    void* SSBOInstance::MapData() const {
        SR_TRACY_ZONE;

        void* pData = Map();
        if (!pData) {
            SR_ERROR("SSBOInstance::MapData() : failed to map SSBO!");
            return nullptr;
        }

        if (HasCounter()) {
            return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(pData) + GetCounterSize());
        }

        return pData;
    }

    void SSBOInstance::ResetCounter(uint32_t value) {
        SR_TRACY_ZONE;

        if (!HasCounter()) {
            SR_ERROR("SSBOInstance::ResetCounter() : SSBO does not have a counter!");
            return;
        }

        bool isNeedUnmap = m_mappedData == nullptr;

        if (!m_mappedData) {
            if (!Map()) {
                SR_ERROR("SSBOInstance::ResetCounter() : failed to map SSBO!");
                return;
            }
        }

        *reinterpret_cast<uint32_t*>(m_mappedData) = value;

        if (isNeedUnmap) {
            Flush(0, sizeof(uint32_t));
            UnMap();
        }
    }

    void SSBOInstance::Flush(uint32_t offset, uint32_t size) {
        SR_TRACY_ZONE;

        if (!m_mappedData) {
            SR_ERROR("SSBOInstance::Flush() : SSBO is not mapped!");
            return;
        }

        if (m_SSBO == SR_ID_INVALID) {
            SR_ERROR("SSBOInstance::Flush() : SSBO is not allocated!");
            return;
        }

        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->FlushSSBO(m_SSBO, offset, size);
        }
        else {
            SR_ERROR("SSBOInstance::Flush() : pPipeline is nullptr!");
        }
    }

    void SSBOInstance::FlushCounter() {
        SR_TRACY_ZONE;

        if (!HasCounter()) {
            SR_ERROR("SSBOInstance::FlushCounter() : SSBO does not have a counter!");
            return;
        }

        Flush(0, sizeof(uint32_t));
    }

    void SSBOInstance::Memset(int32_t value, uint64_t offset, uint64_t size) {
        SR_TRACY_ZONE;

        if (size == SR_UINT32_MAX) {
            size = m_size;
        }

        if (size + offset > m_size) {
            SR_ERROR("SSBOInstance::Memset() : size + offset exceeds SSBO size!");
            return;
        }

        bool isNeedUnmap = m_mappedData == nullptr;

        if (!m_mappedData) {
            if (!Map()) {
                SR_ERROR("SSBOInstance::ResetCounter() : failed to map SSBO!");
                return;
            }
        }

        const uint64_t counterSize = GetCounterSize();
        std::memset(static_cast<uint8_t*>(m_mappedData) + offset + counterSize, value, size);

        if (isNeedUnmap) {
            Flush(offset + counterSize, size);
            UnMap();
        }
    }

    void SSBOInstance::UpdateSSBO(const void* pData, uint64_t size) {
        SR_TRACY_ZONE;

        if (!pData) {
            SR_ERROR("SSBOInstance::UpdateSSBO() : pData is nullptr!");
            return;
        }

        if (size == SR_UINT64_MAX) {
            size = m_size;
        }
        else if (size > m_size) {
            SR_ERROR("SSBOInstance::UpdateSSBO() : size exceeds SSBO size!");
            return;
        }

        bool isNeedUnmap = m_mappedData == nullptr;

        if (!m_mappedData) {
            if (!Map()) {
                SR_ERROR("SSBOInstance::UpdateSSBO() : failed to map SSBO!");
                return;
            }
        }

        const uint32_t counterSize = GetCounterSize();
        std::memcpy(static_cast<uint8_t*>(m_mappedData) + counterSize, pData, size);

        if (isNeedUnmap) {
            Flush(counterSize, size);
            UnMap();
        }
    }

    void* SSBOInstance::GetMappedData() const {
        if (!m_mappedData) {
            SRHalt("SSBOInstance::GetMappedData() : SSBO is not mapped!");
            return nullptr;
        }
        return (char*)m_mappedData + GetCounterSize();
    }

    int32_t SSBOInstance::GetSSBO() const noexcept {
        if (m_SSBO == SR_ID_INVALID) {
            SRHalt("SSBOInstance::GetSSBO() : SSBO is not allocated!");
            return SR_ID_INVALID;
        }
        return m_SSBO;
    }
}
