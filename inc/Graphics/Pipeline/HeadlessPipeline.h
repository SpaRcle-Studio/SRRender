//
// Created by Monika on 01.12.2025.
//

#ifndef SR_ENGINE_GRAPHICS_HEADLESS_PIPELINE_H
#define SR_ENGINE_GRAPHICS_HEADLESS_PIPELINE_H

#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    class HeadlessPipeline : public Pipeline {
        using Super = Pipeline;
    public:
        explicit HeadlessPipeline(RenderContextPtr pRenderContext);
        ~HeadlessPipeline() override;

        SR_NODISCARD PipelineType GetType() const noexcept override;

        SR_NODISCARD int32_t AllocateVBO(uint64_t size, const void* pData) override;
        SR_NODISCARD int32_t AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) override;
        SR_NODISCARD int32_t AllocateUBO(uint32_t uboSize) override;
        SR_NODISCARD int32_t AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) override;
        SR_NODISCARD int32_t AllocDescriptorSet(const std::vector<DescriptorType>& types) override;
        SR_NODISCARD int32_t AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) override;
        SR_NODISCARD int32_t AllocateTexture(const SRTextureCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateCmdBuffer() override;

        bool FreeDescriptorSet(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeVBO(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeIBO(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeUBO(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeFBO(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeSSBO(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeCubeMap(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeShader(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeTexture(int32_t* id) override { *id = SR_ID_INVALID; return true; }
        bool FreeCmdBuffer(int32_t* id) override { *id = SR_ID_INVALID; return true; }

        void GetFBOHandles(std::vector<void*>& handles) const override;
        void GetShaderHandles(std::vector<void*>& handles) const override;

        bool IsSamplerValid(int32_t id) const override { return true; }

        SR_NODISCARD void* GetCurrentShaderHandle() const override { return reinterpret_cast<void*>(1); }
        SR_NODISCARD void* GetCurrentFBOHandle() const override { return reinterpret_cast<void*>(1); }

        SR_NODISCARD uint16_t GetSwapchainImagesCount() const override { return 1; }

    };
}

#endif //SR_ENGINE_GRAPHICS_HEADLESS_PIPELINE_H
