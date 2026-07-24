//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Pipeline/WebGPU/WebGPUPipeline.h>
#include <Graphics/Overlay/WebGPUImGuiOverlay.h>

#include <Utils/Types/ObjectPool.h>
#include <Utils/Common/Features.h>

#include <emscripten/html5.h>

#include <webgpu/webgpu_cpp.h>

#include <algorithm>

namespace SR_GRAPH_NS {
    struct WebGPUPipelineInternalData {
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Instance instance;
        wgpu::Surface surface;
        wgpu::CommandBuffer surfaceCommandBuffer;
        wgpu::CommandEncoder activeEncoder;

        wgpu::Texture surfaceTexture;
        wgpu::TextureView surfaceTextureView;
        wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
        uint32_t surfaceWidth = 0;
        uint32_t surfaceHeight = 0;

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

        if (!m_internalData->device || !m_internalData->queue || !m_internalData->activeEncoder || !m_internalData->surfaceTextureView) {
            return;
        }

        // Draw scene (TODO) + ImGui into the current surface texture.
        wgpu::RenderPassColorAttachment color{};
        color.view = m_internalData->surfaceTextureView;
        color.loadOp = wgpu::LoadOp::Clear;
        color.storeOp = wgpu::StoreOp::Store;
        color.clearValue = {0.10f, 0.10f, 0.12f, 1.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &color;

        auto pass = m_internalData->activeEncoder.BeginRenderPass(&passDesc);

    #ifdef SR_USE_IMGUI
        if (auto&& pOverlayBase = GetOverlay(OverlayType::ImGui)) {
            if (pOverlayBase->IsEnabled() && !pOverlayBase->IsSurfaceDirty()) {
                if (auto&& pImGuiOverlay = SR_UTILS_NS::DynamicPointerCast<WebGPUImGuiOverlay>(pOverlayBase)) {
                    pImGuiOverlay->Render(pass.Get());
                }
            }
        }
    #endif

        pass.End();

        m_internalData->surfaceCommandBuffer = m_internalData->activeEncoder.Finish();
        m_internalData->surfaceCommandBuffer.SetLabel("Surface command buffer");

        if (m_internalData->surfaceCommandBuffer) {
            m_internalData->queue.Submit(1, &m_internalData->surfaceCommandBuffer);
        }

        // Emscripten/WebGPU: wgpuSurfacePresent is unsupported.
        // Presentation is driven by the browser main loop (requestAnimationFrame).
        // Release surface texture references after submitting work.
        m_internalData->surfaceCommandBuffer = nullptr;
        m_internalData->activeEncoder = nullptr;
        m_internalData->surfaceTextureView = nullptr;
        m_internalData->surfaceTexture = nullptr;
    }

    void WebGPUPipeline::OnFrameBuildBegin() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildBegin();

        m_internalData->instance.ProcessEvents();

        if (!m_internalData->device || !m_internalData->queue || !m_internalData->surface) {
            return;
        }

        // Resize-aware surface configuration.
        // In Emscripten/WebGPU we keep surface size in CSS pixels to avoid feedback loops
        // and to match ImGui coordinates when DisplayFramebufferScale = (1,1).
        double cssW = 0.0, cssH = 0.0;
        if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
            constexpr uint32_t kMaxSurfaceDim = 8192; // Safe default WebGPU limit (unless requested higher at device creation).
            const uint32_t pxW = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssW)));
            const uint32_t pxH = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssH)));

            if (pxW != m_internalData->surfaceWidth || pxH != m_internalData->surfaceHeight) {
                m_internalData->surfaceWidth = pxW;
                m_internalData->surfaceHeight = pxH;

                wgpu::SurfaceConfiguration config{};
                config.device = m_internalData->device;
                config.format = m_internalData->surfaceFormat != wgpu::TextureFormat::Undefined ? m_internalData->surfaceFormat : wgpu::TextureFormat::BGRA8Unorm;
                config.usage = wgpu::TextureUsage::RenderAttachment;
                config.width = pxW;
                config.height = pxH;
                config.presentMode = wgpu::PresentMode::Fifo;

                m_internalData->surface.Configure(&config);
                SetOverlaySurfaceDirty();
                ReCreateOverlay();
            }
        }

        wgpu::SurfaceTexture surfaceTexture{};
        m_internalData->surface.GetCurrentTexture(&surfaceTexture);
        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
            surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal)
        {
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

        m_internalData->surfaceTexture = surfaceTexture.texture;
        m_internalData->surfaceTextureView = m_internalData->surfaceTexture.CreateView();

        m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder();
        m_internalData->activeEncoder.SetLabel("Surface command encoder");
    }

    void WebGPUPipeline::OnFrameBuildEnd() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildEnd();
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
        canvasDesc.selector = EMSCRIPTEN_CANVAS_ID;

        wgpu::SurfaceDescriptor surfaceDesc{};
        surfaceDesc.nextInChain = &canvasDesc;

        SR_LOG("WebGPUPipeline::PreInit() : creating WebGPU surface for canvas element...");

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
                double cssW = 0.0, cssH = 0.0;
                if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
                    constexpr uint32_t kMaxSurfaceDim = 8192;
                    config.width = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssW)));
                    config.height = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssH)));
                }
                else {
                    config.width = 800;
                    config.height = 600;
                }
                config.presentMode = wgpu::PresentMode::Fifo;

                m_internalData->surfaceFormat = config.format;
                m_internalData->surfaceWidth = config.width;
                m_internalData->surfaceHeight = config.height;

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

    bool WebGPUPipeline::Destroy() {
        DestroyOverlay();
        return Super::Destroy();
    }

    bool WebGPUPipeline::InitOverlay() {
        SR_TRACY_ZONE;

    #ifdef SR_USE_IMGUI
        const bool defaultEnabled =
        #if defined(SR_EMSCRIPTEN)
            true;
        #else
            false;
        #endif

        if (SR_UTILS_NS::Features::Instance().Enabled("ImGUI", defaultEnabled)) {
            auto&& pImGuiOverlay = m_overlays[OverlayType::ImGui];
            pImGuiOverlay = new WebGPUImGuiOverlay(GetThis());
            if (!pImGuiOverlay->Init()) {
                PipelineError("WebGPUPipeline::InitOverlay() : failed to initialize ImGui overlay!");
                return false;
            }
        }
    #endif

        return Pipeline::InitOverlay();
    }

    PipelineType WebGPUPipeline::GetType() const noexcept {
        return PipelineType::WebGPU;
    }

    WGPUDevice WebGPUPipeline::GetWGPUDevice() const {
        return m_internalData ? m_internalData->device.Get() : nullptr;
    }

    WGPUTextureFormat WebGPUPipeline::GetSurfaceFormat() const {
        if (!m_internalData) {
            return WGPUTextureFormat_Undefined;
        }
        return static_cast<WGPUTextureFormat>(m_internalData->surfaceFormat);
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateVBO(int32_t VBO, uint64_t size, const void* pData) {
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

    SR_NODISCARD int32_t WebGPUPipeline::AllocDescriptorSet(const SR_UTILS_NS::Vector<DescriptorType>& types) {
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

    void WebGPUPipeline::GetShaderHandles(SR_UTILS_NS::Vector<void *>& handles) const {
        handles.clear();
        handles.emplace_back(reinterpret_cast<void*>(1));
    }

    void WebGPUPipeline::GetFBOHandles(SR_UTILS_NS::Vector<void *>& handles) const {
        handles.clear();
        handles.emplace_back(reinterpret_cast<void*>(1));
    }

    int32_t WebGPUPipeline::AllocateCmdBuffer() {
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    bool WebGPUPipeline::FreeDescriptorSet(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeVBO(int32_t* id) {
        m_internalData->VBOs.At(*id).Destroy();
        m_internalData->VBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeIBO(int32_t* id) {
        m_internalData->UBOs.At(*id).Destroy();
        m_internalData->UBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeUBO(int32_t* id) {
        m_internalData->UBOs.At(*id).Destroy();
        m_internalData->UBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeFBO(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeSSBO(int32_t* id) {
        m_internalData->SSBOs.At(*id).Destroy();
        m_internalData->SSBOs.RemoveByIndex(*id);
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeCubeMap(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeShader(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeTexture(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeCmdBuffer(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }
}