//
// Created by Monika on 11.07.2022.
//

#include <Graphics/Memory/ShaderUBOBlock.h>

namespace SR_GRAPH_NS::Memory {
    ShaderUBOBlock::~ShaderUBOBlock() {
        DeInit();
    }

    void ShaderUBOBlock::Append(uint64_t hashId, uint64_t size, uint64_t alignedSize, bool hidden) {
        SRAssert2(size > 1, "Size must be greater than 1!");
        SRAssert2(alignedSize > 1, "Aligned size must be greater than 1!");

        ++m_dataCount;

        auto&& offset = OffsetBlock(alignedSize);

        auto&& subBlock = SubBlock {
            .hashId = hashId,
            .size = size,
            .offset = static_cast<uint64_t>(m_size + offset),
            .hidden = hidden,
        };

        /// Reallocation
        {
            auto&& data = new SubBlock[m_dataCount];

            data[m_dataCount - 1] = subBlock;

            if (m_data) {
                memcpy(data, m_data, (m_dataCount - 1) * sizeof(SubBlock));
                delete[] m_data;
            }

            m_data = data;
        }

        m_size += alignedSize + offset;
    }

    void ShaderUBOBlock::Append(uint64_t hashId, uint64_t size, bool hidden) {
        SRAssert2(size > 1, "Size must be greater than 1!");

        ++m_dataCount;

        auto&& offset = OffsetBlock(size);

        auto&& subBlock = SubBlock {
            .hashId = hashId,
            .size = size,
            .offset = static_cast<uint64_t>(m_size + offset),
            .hidden = hidden,
        };

        /// Reallocation
        {
            auto&& data = new SubBlock[m_dataCount];

            data[m_dataCount - 1] = subBlock;

            if (m_data) {
                memcpy(data, m_data, (m_dataCount - 1) * sizeof(SubBlock));
                delete[] m_data;
            }

            m_data = data;
        }

        m_size += size + offset;
    }

    void ShaderUBOBlock::Init() {
        SRAssert2(!m_initialized, "Double initialization!");
        m_size = TopAlign(m_size);
        FreeMemory(m_memory);
        if (m_size > 0) SR_LIKELY_ATTRIBUTE {
            m_memory = AllocMemory(m_size);
        }
        m_initialized = true;
    }

    void ShaderUBOBlock::DeInit() {
        m_dataCount = 0;
        m_alignedBlock = 0;

        if (m_data) {
            delete[] m_data;
            m_data = nullptr;
        }

        m_size = 0;
        m_binding = SR_ID_INVALID;

        FreeMemory(m_memory);

        m_initialized = false;
    }

    void ShaderUBOBlock::SetField(uint64_t hashId, const void* pData) noexcept {
        if (!m_memory || !pData) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        SRAssert(m_initialized);

        for (uint8_t i = 0; i < m_dataCount; ++i) {
            SubBlock& pSubBlock = m_data[i];
            if (pSubBlock.hashId == hashId) SR_UNLIKELY_ATTRIBUTE {
                memcpy(m_memory + pSubBlock.offset, pData, pSubBlock.size);
                return;
            }
        }
    }

    void ShaderUBOBlock::FreeMemory(char*& pMemory) {
        if (!pMemory) {
            return;
        }

        /// _aligned_free(pMemory);
        free(pMemory);

        pMemory = nullptr;
    }

    char* ShaderUBOBlock::AllocMemory(uint64_t size) {
        SRAssert(m_align == 0 || size % m_align == 0);

        return (char*)malloc(size);
    }

    uint32_t ShaderUBOBlock::OffsetBlock(uint32_t block) {
        if (m_align == 0) {
            return 0;
        }

        /// small blocks
        if (block <= m_align) {
            if (m_alignedBlock + block <= m_align) {
                if ((m_alignedBlock += block) == m_align) {
                    m_alignedBlock = 0;
                }
                return 0;
            }
            else {
                const uint32_t offset = m_alignedBlock > 0 ? (m_align - m_alignedBlock) : 0;
                m_alignedBlock = block % m_align == 0 ? 0 : block;
                return offset;
            }
        }
        /// big blocks
        else {
            if (block % m_align == 0) {
                const uint32_t offset = m_alignedBlock > 0 ? (m_align - m_alignedBlock) : 0;
                m_alignedBlock = 0;
                return offset;
            }
            else {
                SRHalt("Big type isn't aligned!");
            }
        }

        return 0;
    }

    uint32_t ShaderUBOBlock::TopAlign(uint32_t block) {
        if (m_align == 0 || block % m_align == 0) {
            return block;
        }

        return static_cast<uint32_t>(static_cast<float_t>((block + (m_align - 1)) / static_cast<float_t>(m_align))) * m_align;
    }

    uint64_t ShaderUBOBlock::Align(uint64_t size) const {
        const bool powerOfTwo = (size & (size - 1)) == 0;

        if (size == 0 || powerOfTwo) {
            return size;
        }

        auto&& n = static_cast<uint64_t>(std::log2(size));

        return std::pow(2, n + 1);
    }
}