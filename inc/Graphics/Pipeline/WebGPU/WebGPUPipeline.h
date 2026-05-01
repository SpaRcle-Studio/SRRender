//
// Created by Monika on 01.03.2026.
//

#ifndef SR_ENGINE_GRAPHICS_WEB_GPU_PIPELINE_H
#define SR_ENGINE_GRAPHICS_WEB_GPU_PIPELINE_H

#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    struct WebGPUPipelineInternalData;

    class WebGPUPipeline : public Pipeline {
        using Super = Pipeline;
    public:
        explicit WebGPUPipeline(RenderContextPtr pRenderContext);
        ~WebGPUPipeline() override;

        bool PreInit(const PipelinePreInitInfo& info) override;
        bool IsAsyncEarlyInit() const override;
        bool Init() override;
        bool PostInit() override;

        void DrawFrame() override;
        void OnFrameBuildBegin() override;
        void OnFrameBuildEnd() override;

        SR_NODISCARD PipelineType GetType() const noexcept override;

        SR_NODISCARD int32_t AllocateVBO(int32_t VBO, uint64_t size, const void* pData) override;
        SR_NODISCARD int32_t AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) override;
        SR_NODISCARD int32_t AllocateUBO(uint32_t uboSize) override;
        SR_NODISCARD int32_t AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) override;
        SR_NODISCARD int32_t AllocDescriptorSet(const std::vector<DescriptorType>& types) override;
        SR_NODISCARD int32_t AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) override;
        SR_NODISCARD int32_t AllocateTexture(const SRTextureCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateCmdBuffer() override;

        bool FreeDescriptorSet(int32_t* id) override;
        bool FreeVBO(int32_t* id) override;
        bool FreeIBO(int32_t* id) override;
        bool FreeUBO(int32_t* id) override;
        bool FreeFBO(int32_t* id) override;
        bool FreeSSBO(int32_t* id) override;
        bool FreeCubeMap(int32_t* id) override;
        bool FreeShader(int32_t* id) override;
        bool FreeTexture(int32_t* id) override;
        bool FreeCmdBuffer(int32_t* id) override;

        void GetFBOHandles(std::vector<void*>& handles) const override;
        void GetShaderHandles(std::vector<void*>& handles) const override;

        bool IsSamplerValid(int32_t id) const override { return true; }

        SR_NODISCARD void* GetCurrentShaderHandle() const override { return reinterpret_cast<void*>(1); }
        SR_NODISCARD void* GetCurrentFBOHandle() const override { return reinterpret_cast<void*>(1); }

        SR_NODISCARD uint16_t GetSwapchainImagesCount() const override { return 1; }
        SR_NODISCARD uint8_t GetCurrentImageIndex() const override { return 0; }

    private:
        WebGPUPipelineInternalData* m_internalData = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_WEB_GPU_PIPELINE_H