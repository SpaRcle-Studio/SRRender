//
// Created by Monika on 15.09.2023.
//

#ifndef SR_ENGINE_GRAPHICS_VULKAN_PIPELINE_H
#define SR_ENGINE_GRAPHICS_VULKAN_PIPELINE_H

#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS::VulkanTools {
    class MemoryManager;
}

namespace EvoVulkan::Core {
    class VulkanKernel;
}

namespace EvoVulkan::Complexes {
    class FrameBuffer;
    class Shader;
}

namespace SR_GRAPH_NS {
    class VulkanPipeline : public Pipeline {
        using Super = Pipeline;
    public:
        explicit VulkanPipeline(const RenderContextPtr& pContext)
            : Super(pContext)
        { }

        ~VulkanPipeline() override;

    public:
        bool InitOverlay() override;

        bool PreInit(const PipelinePreInitInfo& info) override;
        bool Init() override;
        bool PostInit() override;

        bool Destroy() override;

    public:
        SR_NODISCARD PipelineType GetType() const noexcept override { return PipelineType::Vulkan; }

        SR_NODISCARD std::string GetVendor() const override;
        SR_NODISCARD std::string GetRenderer() const override { return "Vulkan"; }
        SR_NODISCARD std::string GetVersion() const override { return "VK_API_VERSION_1_3"; }

        SR_NODISCARD void* GetCurrentFBOHandle() const override;
        SR_NODISCARD std::set<void*> GetFBOHandles() const override;
        SR_NODISCARD std::set<void*> GetShaderHandles() const override;
        SR_NODISCARD uint8_t GetFrameBufferSampleCount() const override;
        SR_NODISCARD uint8_t GetBuildIterationsCount() const noexcept override;
        SR_NODISCARD SR_MATH_NS::FColor GetPixelColor(uint32_t textureId, uint32_t x, uint32_t y) override;
        SR_NODISCARD void* GetCurrentShaderHandle() const override;

        SR_NODISCARD EvoVulkan::Core::VulkanKernel* GetKernel() const noexcept { return m_kernel; }
        SR_NODISCARD VulkanTools::MemoryManager* GetMemoryManager() const noexcept { return m_memory; }
        SR_NODISCARD uint64_t GetUsedMemory() const override;
        SR_NODISCARD bool IsShaderConstantSupport() const noexcept override { ++m_state.operations; return true; }

        SR_NODISCARD int32_t AllocateUBO(uint32_t uboSize) override;
        SR_NODISCARD int32_t AllocateVBO(void* pVertices, Vertices::VertexType type, size_t count) override;
        SR_NODISCARD int32_t AllocateIBO(void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) override;
        SR_NODISCARD int32_t AllocDescriptorSet(const std::vector<DescriptorType>& types) override;
        SR_NODISCARD int32_t AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) override;
        SR_NODISCARD int32_t AllocateTexture(const SRTextureCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) override;
        SR_NODISCARD int32_t AllocateSSBO(uint32_t size, SSBOUsage usage) override;

        bool FreeDescriptorSet(int32_t* id) override;
        bool FreeVBO(int32_t* id) override;
        bool FreeIBO(int32_t* id) override;
        bool FreeUBO(int32_t* id) override;
        bool FreeFBO(int32_t* id) override;
        bool FreeSSBO(int32_t* id) override;
        bool FreeCubeMap(int32_t* id) override;
        bool FreeShader(int32_t* id) override;
        bool FreeTexture(int32_t* id) override;
        bool IsSamplerValid(int32_t id) const override;

    public:
        void SetVSyncEnabled(bool enabled) override;
        SR_NODISCARD bool IsVSyncEnabled() const override;

        void SetOverlayEnabled(OverlayType overlayType, bool enabled) override;
        void SetDirty(bool dirty) override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;

        bool BeginCmdBuffer() override;
        void EndCmdBuffer() override;

        bool BeginRender() override;
        void EndRender() override;

        void PrepareFrame() override;
        void DrawFrame() override;

        void OnMultiSampleChanged() override;
        void SetCurrentFrameBuffer(FramebufferPtr pFrameBuffer) override;

        void SetViewport(int32_t width, int32_t height) override;
        void SetScissor(int32_t width, int32_t height) override;

        void ClearBuffers() override;
        void ClearBuffers(float_t r, float_t g, float_t b, float_t a, float_t depth, uint8_t colorCount) override;
        void ClearBuffers(const ClearColors& clearColors, std::optional<float_t> depth) override;
        void ClearDepthBuffer(float_t depth) override;
        void ClearColorBuffer(const ClearColors& clearColors) override;

        void UpdateDescriptorSets(uint32_t descriptorSet, const SRDescriptorUpdateInfos& updateInfo) override;
        void UpdateUBO(uint32_t UBO, void* pData, uint64_t size) override;

        void PushConstants(void* pData, uint64_t size) override;

        void UseShader(uint32_t shaderProgram) override;
        void UnUseShader() override;

        void Draw(uint32_t count) override;
        void DrawIndices(uint32_t count) override;

        void BindAttachment(uint8_t activeTexture, uint32_t textureId) override;
        void BindVBO(uint32_t VBO) override;
        void BindUBO(uint32_t UBO) override;
        void BindIBO(uint32_t IBO) override;
        void BindTexture(uint8_t activeTexture, uint32_t textureId) override;
        bool BindDescriptorSet(uint32_t descriptorSet) override;
        void BindFrameBuffer(FramebufferPtr pFBO) override;
        void BindSSBO(uint32_t SSBO) override;

        void ResetLastShader() override;

    private:
        bool InitEvoVulkanHooks();

    private:
        VkDeviceSize m_offsets[1] = { 0 };
        VkViewport m_viewport = { };
        VkRect2D m_scissor = { };
        VkRenderPassBeginInfo m_renderPassBI = { };
        VkCommandBufferBeginInfo m_cmdBufInfo = { };

        VkDescriptorSet m_currentDescriptorSet = VK_NULL_HANDLE;

        VkCommandBuffer m_currentCmd  = VK_NULL_HANDLE;
        VkPipelineLayout m_currentLayout = VK_NULL_HANDLE;

        std::vector<VkClearValue> m_clearValues;

        EvoVulkan::Complexes::FrameBuffer* m_currentVkFrameBuffer = nullptr;
        EvoVulkan::Complexes::Shader* m_currentVkShader = nullptr;
        EvoVulkan::Complexes::Shader* m_lastVkShader = nullptr;
        EvoVulkan::Core::VulkanKernel* m_kernel = nullptr;

        VulkanTools::MemoryManager* m_memory = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_VULKAN_PIPELINE_H
