//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Pipeline/WebGPU/WebGPUPipeline.h>

#include <emscripten/html5.h>
#include <webgpu/webgpu_cpp.h>

namespace SR_GRAPH_NS {
    struct WebGPUPipelineInternalData {
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Instance instance;
    };

    WebGPUPipeline::WebGPUPipeline(const RenderContextPtr pRenderContext)
        : Super(pRenderContext)
    {
        m_supportedSampleCount = 1;
        m_internalData = new WebGPUPipelineInternalData();
    }

    WebGPUPipeline::~WebGPUPipeline() {
        SR_SAFE_DELETE_PTR(m_internalData);
    }

    bool WebGPUPipeline::PreInit(const PipelinePreInitInfo& info) {
        SR_INFO("WebGPUPipeline::PreInit() : requesting WebGPU adapter and device...");

        // Создаем экземпляр адаптера
        wgpu::RequestAdapterOptions options{};
        options.powerPreference = wgpu::PowerPreference::Undefined;

        m_internalData->instance = wgpu::CreateInstance();
        if (!m_internalData->instance) {
            SR_ERROR("WebGPUPipeline::PreInit() : failed to create WebGPU instance!");
            return false;
        }

        m_internalData->instance.RequestAdapter(&options, wgpu::CallbackMode::AllowProcessEvents, [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, const char *message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                SR_ERROR("WebGPUPipeline::PreInit() : failed to request adapter: {}", message);
                return;
            }

            SR_INFO("WebGPUPipeline::PreInit() : WebGPU adapter successfully obtained.");

            // Запрашиваем устройство и очередь
            adapter.RequestDevice(nullptr, wgpu::CallbackMode::AllowProcessEvents, [this](wgpu::RequestDeviceStatus status, wgpu::Device dev, const char *message) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    SR_ERROR("WebGPUPipeline::PreInit() : failed to request device: {}", message);
                    return;
                }

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU device successfully obtained.");

                m_internalData->device = dev;
                m_internalData->queue = m_internalData->device.GetQueue();

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU device and queue successfully created.");
            });
        });

        SR_INFO("WebGPUPipeline::PreInit() : waiting for WebGPU initialization to complete...");

        /// wait for device creation (in a real implementation, this should be handled asynchronously)
        while (!m_internalData->device) {
            //emscripten_sleep(100);
            SR_NOOP;
        }

        SR_INFO("WebGPUPipeline::PreInit() : WebGPU successfully initialized.");

        return true;
    }

    bool WebGPUPipeline::Init() {
        return true;
    }

    bool WebGPUPipeline::PostInit() {
        return true;
    }


    PipelineType WebGPUPipeline::GetType() const noexcept {
        return PipelineType::WebGPU;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateVBO(const void* pVertices, Vertices::VertexType type, size_t count) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateUBO(uint32_t uboSize) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocDescriptorSet(const std::vector<DescriptorType>& types) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateTexture(const SRTextureCreateInfo& createInfo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    void WebGPUPipeline::GetShaderHandles(std::vector<void *>& handles) const {
        handles.clear();
        handles.emplace_back(reinterpret_cast<void*>(1));
    }

    void WebGPUPipeline::GetFBOHandles(std::vector<void *>& handles) const {
        handles.clear();
        handles.emplace_back(reinterpret_cast<void*>(1));
    }

    int32_t WebGPUPipeline::AllocateCmdBuffer() {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }
}