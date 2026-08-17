//
// Created by Monika on 01.03.2026.
//

#ifndef SR_ENGINE_GRAPHICS_WEB_GPU_PIPELINE_H
#define SR_ENGINE_GRAPHICS_WEB_GPU_PIPELINE_H

#include <Graphics/Pipeline/Pipeline.h>

#if defined(SR_RENDER_USE_WEBGPU)
    #include <webgpu/webgpu.h>
#endif

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
        bool Destroy() override;

        bool InitOverlay() override;

        void DrawFrame() override;
        void OnFrameBuildBegin() override;
        void OnFrameBuildEnd() override;

        SR_NODISCARD PipelineType GetType() const noexcept override;

        SR_NODISCARD int32_t AllocateVBO(int32_t VBO, uint64_t size, const void* pData) override;
        SR_NODISCARD int32_t AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) override;
        SR_NODISCARD int32_t AllocateUBO(uint32_t uboSize) override;
        SR_NODISCARD int32_t AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) override;
        SR_NODISCARD int32_t AllocDescriptorSet(const SR_UTILS_NS::Vector<DescriptorType>& types) override;
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

        void GetFBOHandles(SR_UTILS_NS::Vector<void*>& handles) const override;
        void GetShaderHandles(SR_UTILS_NS::Vector<void*>& handles) const override;

        bool IsSamplerValid(int32_t id) const override { return id >= 0; }

        void UpdateUBO(uint32_t UBO, void* pData, uint64_t size, bool sizesMustBeEqual) override;
        void UpdateSSBO(uint32_t SSBO, void* pData, uint64_t size) override;

        bool MapSSBO(uint32_t SSBO, void** ppData) override;
        void UnMapSSBO(uint32_t SSBO) override;
        void FlushSSBO(uint32_t SSBO, uint64_t offset, uint64_t size) override;

        void BindFrameBuffer(FramebufferPtr pFBO) override;
        bool BeginRender() override;
        void EndRender() override;
        void ClearBuffers() override;
        void ClearBuffers(float_t r, float_t g, float_t b, float_t a, float_t depth, uint8_t colorCount) override;
        void ClearBuffers(const ClearColors& clearColors, std::optional<float_t> depth) override;
        void SetViewport(int32_t width, int32_t height) override;
        void SetScissor(int32_t width, int32_t height) override;

        void UseShader(uint32_t shaderProgram) override;
        void BindVBO(uint32_t VBO, uint32_t slot, VertexInputRate inputRate) override;
        void BindIBO(uint32_t IBO) override;
        void BindTexture(uint8_t activeTexture, uint32_t textureId) override;
        void BindAttachment(uint8_t activeTexture, uint32_t textureId) override;
        bool BindDescriptorSet(uint32_t descriptorSet) override;
        void Draw(uint32_t count) override;
        void DrawIndices(uint32_t count) override;
        void UpdateDescriptorSets(uint32_t descriptorSet, const SRDescriptorUpdateInfos& updateInfo) override;

        SR_NODISCARD void* GetCurrentShaderHandle() const override;
        SR_NODISCARD void* GetCurrentFBOHandle() const override;

        SR_NODISCARD uint16_t GetSwapchainImagesCount() const override { return 1; }
        SR_NODISCARD uint8_t GetCurrentImageIndex() const override { return 0; }

        /// WebGPU helpers for overlay/backends (Emscripten).
        SR_NODISCARD WGPUDevice GetWGPUDevice() const;
        SR_NODISCARD int32_t GetFramesInFlightCount() const { return 3; }
        SR_NODISCARD WGPUTextureFormat GetSurfaceFormat() const;

        /// Returns the WGPUTextureView for the texture with the given pool id,
        /// or nullptr if the id is invalid / not alive.
        SR_NODISCARD WGPUTextureView GetTextureView(uint32_t textureId) const;

    private:
        /// Запоминает текстуру в привязанном наборе дескрипторов. Как и в Vulkan, текстуры
        /// принадлежат набору, а не шейдеру: движок привязывает их только при изменении.
        void BindTextureToDescriptorSet(uint8_t binding, uint32_t textureId, bool isAttachment);

        /// Пересобирает (если нужно) и привязывает BindGroup набора дескрипторов к @group(0).
        void FlushDescriptorBindGroup();

        /// Создает (или достает из кеша) BindGroup для @group(1) и привязывает его к активному проходу.
        /// В WebGPU все группы, объявленные в layout'е пайплайна, обязаны быть привязаны перед отрисовкой.
        void FlushTextureBindGroup();

        /// Сбрасывает кеш BindGroup'ов текстур. Обязательно вызывать при удалении текстур,
        /// иначе в кеше останутся группы со ссылками на уничтоженные ресурсы.
        void InvalidateTextureBindGroups();

    private:
        WebGPUPipelineInternalData* m_internalData = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_WEB_GPU_PIPELINE_H