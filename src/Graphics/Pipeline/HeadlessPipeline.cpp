//
// Created by Monika on 01.12.2025.
//

#include <Graphics/Pipeline/HeadlessPipeline.h>

namespace SR_GRAPH_NS {
    HeadlessPipeline::HeadlessPipeline(const RenderContextPtr pRenderContext)
        : Super(pRenderContext)
    {
        m_supportedSampleCount = 1;
    }

    HeadlessPipeline::~HeadlessPipeline() = default;

    PipelineType HeadlessPipeline::GetType() const noexcept {
        return PipelineType::Headless;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateVBO(uint64_t size, const void* pData) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateUBO(uint32_t uboSize) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocDescriptorSet(const std::vector<DescriptorType>& types) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateTexture(const SRTextureCreateInfo& createInfo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t HeadlessPipeline::AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    void HeadlessPipeline::GetShaderHandles(std::vector<void *>& handles) const {
        handles.clear();
        handles.emplace_back(reinterpret_cast<void*>(1));
    }

    void HeadlessPipeline::GetFBOHandles(std::vector<void *>& handles) const {
        handles.clear();
        handles.emplace_back(reinterpret_cast<void*>(1));
    }

    int32_t HeadlessPipeline::AllocateCmdBuffer() {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }
}