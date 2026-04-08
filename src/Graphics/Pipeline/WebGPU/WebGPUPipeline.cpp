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
        wgpu::Surface surface;
        wgpu::CommandBuffer surfaceCommandBuffer;
        wgpu::CommandEncoder activeEncoder;

        SR_HTYPES_NS::ObjectPool<wgpu::Buffer> VBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer> IBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer> UBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer> SSBOs;
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

    void WebGPUPipeline::DrawFrame() {
        SR_TRACY_ZONE;

        Super::DrawFrame();

        if (m_internalData->surfaceCommandBuffer) {
            m_internalData->queue.Submit(1, &m_internalData->surfaceCommandBuffer);
        }
    }

    void WebGPUPipeline::OnFrameBuildBegin() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildBegin();

        m_internalData->instance.ProcessEvents();

        wgpu::SurfaceTexture surfaceTexture{};
        m_internalData->surface.GetCurrentTexture(&surfaceTexture);
        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal) {
            switch (surfaceTexture.status) {
                case wgpu::SurfaceGetCurrentTextureStatus::Error:
                    SR_ERROR("WebGPUPipeline::OnFrameBuildBegin() : failed to acquire surface texture: error!");
                    break;
                case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
                    SR_ERROR("WebGPUPipeline::OnFrameBuildBegin() : failed to acquire surface texture: timeout!");
                    break;
                case wgpu::SurfaceGetCurrentTextureStatus::Lost:
                    SR_ERROR("WebGPUPipeline::OnFrameBuildBegin() : failed to acquire surface texture: surface lost!");
                    break;
                case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
                    SR_WARN("WebGPUPipeline::OnFrameBuildBegin() : acquired suboptimal surface texture!");
                    break;
                default:
                    SR_ERROR("WebGPUPipeline::OnFrameBuildBegin() : failed to acquire surface texture: unknown error!");
                    break;
            }
            return;
        }

        m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder();
        m_internalData->activeEncoder.SetLabel("Surface command encoder");

        auto view = surfaceTexture.texture.CreateView();
        wgpu::RenderPassColorAttachment color{};
        color.view = view;
        color.loadOp = wgpu::LoadOp::Clear;
        color.storeOp = wgpu::StoreOp::Store;

        static float animColor = 0.f;
        animColor += 0.01f;
        if (animColor > 1.f) {
            animColor = 0.f;
        }

        color.clearValue = {animColor, 0.2f, 0.4f, 1.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &color;

        auto pass = m_internalData->activeEncoder.BeginRenderPass(&passDesc);
        pass.End();
    }

    void WebGPUPipeline::OnFrameBuildEnd() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildEnd();

        m_internalData->surfaceCommandBuffer = m_internalData->activeEncoder.Finish();
        m_internalData->surfaceCommandBuffer.SetLabel("Surface command buffer");
    }

    bool WebGPUPipeline::PreInit(const PipelinePreInitInfo& info) {
        SR_INFO("WebGPUPipeline::PreInit() : requesting WebGPU adapter and device...");

        EM_ASM({
            navigator.gpu.requestAdapter().then(a => {
                console.log("WebGPUPipeline::PreInit() : JS adapter works:", a);
            }).catch(e => {
                console.log("WebGPUPipeline::PreInit() : JS adapter failed:", e);
            });
        });

        // Создаем экземпляр адаптера
        wgpu::RequestAdapterOptions options{};
        options.powerPreference = wgpu::PowerPreference::Undefined;

        m_internalData->instance = wgpu::CreateInstance();
        if (!m_internalData->instance) {
            SR_ERROR("WebGPUPipeline::PreInit() : failed to create WebGPU instance!");
            return false;
        }

        wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
        canvasDesc.selector = "#canvas";

        wgpu::SurfaceDescriptor surfaceDesc{};
        surfaceDesc.nextInChain = &canvasDesc;

        m_internalData->surface = m_internalData->instance.CreateSurface(&surfaceDesc);
        if (!m_internalData->surface) {
            SR_ERROR("WebGPUPipeline::PreInit() : failed to create WebGPU surface!");
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

                wgpu::SurfaceConfiguration config{};
                config.device = m_internalData->device;
                config.format = wgpu::TextureFormat::BGRA8Unorm;
                config.usage = wgpu::TextureUsage::RenderAttachment;
                config.width = 800;
                config.height = 600;
                config.presentMode = wgpu::PresentMode::Fifo;

                m_internalData->surface.Configure(&config);

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU surface configured successfully.");
            });
        });

        SR_INFO("WebGPUPipeline::PreInit() : waiting for WebGPU initialization to complete...");

        return true;
    }

    bool WebGPUPipeline::IsAsyncEarlyInit() const {
        m_internalData->instance.ProcessEvents();
        return !m_internalData->device && !m_internalData->queue;
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

    SR_NODISCARD int32_t WebGPUPipeline::AllocateVBO(uint64_t size, const void* pData) {
        if (!pData || size == 0) {
            return SR_ID_INVALID;
        }

        wgpu::BufferDescriptor desc{};
        desc.size  = size;
        desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;

        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        m_internalData->queue.WriteBuffer(pBuffer, 0, pData, desc.size);

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += desc.size;

        return m_internalData->VBOs.Add(std::move(pBuffer));
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) {
        if (!pIndices || count == 0) {
            return SR_ID_INVALID;
        }

        wgpu::BufferDescriptor desc{};
        desc.size  = indexSize * count;
        desc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;

        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        m_internalData->queue.WriteBuffer(pBuffer, 0, pIndices, desc.size);

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += desc.size;

        return m_internalData->VBOs.Add(std::move(pBuffer));
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateUBO(uint32_t uboSize) {
        if (uboSize == 0) {
            return SR_ID_INVALID;
        }

        wgpu::BufferDescriptor desc{};
        desc.size  = (uboSize + 15) & ~size_t(15); /// Выравнивание до 16 байт
        desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;

        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += desc.size;

        return m_internalData->UBOs.Add(std::move(pBuffer));
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) {
        if (ssboSize == 0) {
            return SR_ID_INVALID;
        }

        wgpu::BufferDescriptor desc{};
        desc.size  = (ssboSize + 3) & ~size_t(3);

        switch (usage) {
            case SSBOUsage::GPUOnly:
            case SSBOUsage::AutoPreferDevice: {
                desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
                break;
            }
            case SSBOUsage::CPUToGPU:
            case SSBOUsage::Auto: {
                desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
                break;
            }
            case SSBOUsage::GPUToCPU: {
                desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
                break;
            }
            case SSBOUsage::CPUCopy: {
                desc.usage = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
                desc.mappedAtCreation = true;
                break;
            }
            case SSBOUsage::CPUOnly: {
                // SSBO как таковой невозможен.
                // Делаем staging read buffer.
                desc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
                break;
            }
            case SSBOUsage::AutoPreferHost: {
                desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
                break;
            }
            case SSBOUsage::GPULazyAlloc: {
                desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
                break;
            }
        }

        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += desc.size;

        return m_internalData->SSBOs.Add(std::move(pBuffer));
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

    bool WebGPUPipeline::FreeDescriptorSet(int32_t* id) {
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeVBO(int32_t* id) {
        m_internalData->VBOs.At(*id).Destroy();
        m_internalData->VBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeIBO(int32_t* id) {
        m_internalData->UBOs.At(*id).Destroy();
        m_internalData->UBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeUBO(int32_t* id) {
        m_internalData->UBOs.At(*id).Destroy();
        m_internalData->UBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeFBO(int32_t* id) {
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeSSBO(int32_t* id) {
        m_internalData->SSBOs.At(*id).Destroy();
        m_internalData->SSBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeCubeMap(int32_t* id) {
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeShader(int32_t* id) {
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeTexture(int32_t* id) {
        *id = SR_ID_INVALID;
    }

    bool WebGPUPipeline::FreeCmdBuffer(int32_t* id) {
        *id = SR_ID_INVALID;
    }

}