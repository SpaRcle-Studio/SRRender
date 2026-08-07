//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Pipeline/WebGPU/WebGPUPipeline.h>
#include <Graphics/Overlay/WebGPUImGuiOverlay.h>
#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/Pipeline/TextureHelper.h>

#include <Utils/Types/ObjectPool.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/Vertices.h>
#include <Utils/Resources/ResourceManager.h>

#include <emscripten/html5.h>

#include <webgpu/webgpu_cpp.h>

namespace SR_GRAPH_NS {

    // ----------------------------------------------------------------------------------------------------------------
    // Internal resource structs
    // ----------------------------------------------------------------------------------------------------------------

    struct WebGPUShaderProgram {
        wgpu::RenderPipeline    renderPipeline;
        wgpu::ComputePipeline   computePipeline;
        wgpu::BindGroupLayout   bindGroupLayout;   // group 0: UBOs / SSBOs
        wgpu::BindGroupLayout   bindGroupLayout1;  // group 1: textures + samplers (may be null if unused)
        wgpu::PipelineLayout    pipelineLayout;
        bool                    isCompute = false;

        void Destroy() {
            renderPipeline  = nullptr;
            computePipeline = nullptr;
            bindGroupLayout  = nullptr;
            bindGroupLayout1 = nullptr;
            pipelineLayout  = nullptr;
        }
    };

    struct WebGPUTexture {
        wgpu::Texture     texture;
        wgpu::TextureView view;
        wgpu::Sampler     sampler;

        void Destroy() {
            sampler  = nullptr;
            view     = nullptr;
            if (texture) {
                texture.Destroy();
                texture = nullptr;
            }
        }
    };

    struct WebGPUFrameBuffer {
        struct Attachment {
            wgpu::Texture     texture;
            wgpu::TextureView view;
            wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;

            void Destroy() {
                view = nullptr;
                if (texture) {
                    texture.Destroy();
                    texture = nullptr;
                }
            }
        };

        SR_UTILS_NS::Vector<Attachment> colorAttachments;
        Attachment                      depthAttachment;
        uint32_t                        width  = 0;
        uint32_t                        height = 0;

        void Destroy() {
            for (auto& att : colorAttachments) { att.Destroy(); }
            colorAttachments.clear();
            depthAttachment.Destroy();
        }
    };

    struct WebGPUBindGroup {
        wgpu::BindGroup bindGroup;

        void Destroy() {
            bindGroup = nullptr;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data aggregate
    // ----------------------------------------------------------------------------------------------------------------

    struct WebGPUPipelineInternalData {
        wgpu::Device  device;
        wgpu::Queue   queue;
        wgpu::Instance instance;
        wgpu::Surface surface;
        wgpu::CommandBuffer  surfaceCommandBuffer;
        wgpu::CommandEncoder activeEncoder;

        wgpu::Texture     surfaceTexture;
        wgpu::TextureView surfaceTextureView;
        wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
        uint32_t surfaceWidth  = 0;
        uint32_t surfaceHeight = 0;

        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         VBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         IBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         UBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         SSBOs;
        SR_HTYPES_NS::ObjectPool<WebGPUShaderProgram>  shaderPrograms;
        SR_HTYPES_NS::ObjectPool<WebGPUTexture>        textures;
        SR_HTYPES_NS::ObjectPool<WebGPUFrameBuffer>    frameBuffers;
        SR_HTYPES_NS::ObjectPool<WebGPUBindGroup>      bindGroups;
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------------------------------------------------

    static bool IsWGPUNonRenderableFormat(wgpu::TextureFormat fmt) {
        // RG11B10Ufloat is not a renderable color format in most WebGPU implementations.
        // Depth formats are renderable but not as color attachments.
        return fmt == wgpu::TextureFormat::RG11B10Ufloat;
    }

    static wgpu::TextureFormat ImageFormatToWGPU(SR_GRAPH_NS::ImageFormat format, bool isDepthHint = false) {
        switch (format) {
            // "Auto" / "None" → pick a sensible default based on context
            case SR_GRAPH_NS::ImageFormat::Unknown:
            case SR_GRAPH_NS::ImageFormat::None:
            case SR_GRAPH_NS::ImageFormat::Auto:
                // If the caller is creating a depth attachment, use depth32
                return isDepthHint ? wgpu::TextureFormat::Depth32Float : wgpu::TextureFormat::RGBA8Unorm;

            case SR_GRAPH_NS::ImageFormat::RGBA8_UNORM:           return wgpu::TextureFormat::RGBA8Unorm;
            case SR_GRAPH_NS::ImageFormat::BGRA8_UNORM:           return wgpu::TextureFormat::BGRA8Unorm;
            case SR_GRAPH_NS::ImageFormat::RGBA8_SRGB:            return wgpu::TextureFormat::RGBA8UnormSrgb;
            case SR_GRAPH_NS::ImageFormat::RGBA16_UNORM:          return wgpu::TextureFormat::RGBA16Uint;
            case SR_GRAPH_NS::ImageFormat::RGBA16_SFLOAT:         return wgpu::TextureFormat::RGBA16Float;
            case SR_GRAPH_NS::ImageFormat::R8_UNORM:              return wgpu::TextureFormat::R8Unorm;
            case SR_GRAPH_NS::ImageFormat::R16_UNORM:             return wgpu::TextureFormat::R16Uint;
            case SR_GRAPH_NS::ImageFormat::R16_SFLOAT:            return wgpu::TextureFormat::R16Float;
            case SR_GRAPH_NS::ImageFormat::R32_SFLOAT:            return wgpu::TextureFormat::R32Float;
            case SR_GRAPH_NS::ImageFormat::R8_UINT:               return wgpu::TextureFormat::R8Uint;
            case SR_GRAPH_NS::ImageFormat::R16_UINT:              return wgpu::TextureFormat::R16Uint;
            case SR_GRAPH_NS::ImageFormat::R32_UINT:              return wgpu::TextureFormat::R32Uint;
            case SR_GRAPH_NS::ImageFormat::D16_UNORM:             return wgpu::TextureFormat::Depth16Unorm;
            case SR_GRAPH_NS::ImageFormat::D24_UNORM_S8_UINT:     return wgpu::TextureFormat::Depth24PlusStencil8;
            case SR_GRAPH_NS::ImageFormat::D32_SFLOAT:            return wgpu::TextureFormat::Depth32Float;
            case SR_GRAPH_NS::ImageFormat::D32_SFLOAT_S8_UINT:   return wgpu::TextureFormat::Depth32FloatStencil8;
            // RG11B10Ufloat is NOT a renderable color format in WebGPU — downgrade to RGBA16Float
            case SR_GRAPH_NS::ImageFormat::B10G11R11_UFLOAT_PACK32: return wgpu::TextureFormat::RGBA16Float;
            case SR_GRAPH_NS::ImageFormat::RG8_UNORM:             return wgpu::TextureFormat::RG8Unorm;
            case SR_GRAPH_NS::ImageFormat::RGB8_UNORM:
            case SR_GRAPH_NS::ImageFormat::RGB8_SRGB:
            case SR_GRAPH_NS::ImageFormat::RGB16_UNORM:           return wgpu::TextureFormat::RGBA8Unorm; // no RGB in WebGPU
            default:
                SR_WARN("ImageFormatToWGPU: unsupported format {}, falling back to RGBA8Unorm", static_cast<int>(format));
                return wgpu::TextureFormat::RGBA8Unorm;
        }
    }

    static bool IsDepthFormat(wgpu::TextureFormat fmt) {
        return fmt == wgpu::TextureFormat::Depth16Unorm
            || fmt == wgpu::TextureFormat::Depth24Plus
            || fmt == wgpu::TextureFormat::Depth24PlusStencil8
            || fmt == wgpu::TextureFormat::Depth32Float
            || fmt == wgpu::TextureFormat::Depth32FloatStencil8;
    }

    static wgpu::FilterMode TextureFilterToWGPU(SR_GRAPH_NS::TextureFilter filter) {
        switch (filter) {
            case SR_GRAPH_NS::TextureFilter::LINEAR:
            case SR_GRAPH_NS::TextureFilter::LINEAR_MIPMAP_LINEAR:
            case SR_GRAPH_NS::TextureFilter::LINEAR_MIPMAP_NEAREST: return wgpu::FilterMode::Linear;
            default:                                                 return wgpu::FilterMode::Nearest;
        }
    }

    static wgpu::MipmapFilterMode TextureFilterToMipWGPU(SR_GRAPH_NS::TextureFilter filter) {
        switch (filter) {
            case SR_GRAPH_NS::TextureFilter::LINEAR_MIPMAP_LINEAR:
            case SR_GRAPH_NS::TextureFilter::NEAREST_MIPMAP_LINEAR: return wgpu::MipmapFilterMode::Linear;
            default:                                                 return wgpu::MipmapFilterMode::Nearest;
        }
    }

    static wgpu::AddressMode AddressModeToWGPU(SR_GRAPH_NS::AddressMode mode) {
        switch (mode) {
            case SR_GRAPH_NS::AddressMode::Repeat:         return wgpu::AddressMode::Repeat;
            case SR_GRAPH_NS::AddressMode::MirroredRepeat: return wgpu::AddressMode::MirrorRepeat;
            case SR_GRAPH_NS::AddressMode::ClampToEdge:
            case SR_GRAPH_NS::AddressMode::ClampToBorder:
            case SR_GRAPH_NS::AddressMode::MirrorClampToEdge:
            default:                                       return wgpu::AddressMode::ClampToEdge;
        }
    }

    /// Map VertexAttributeFormat + count → wgpu::VertexFormat
    static wgpu::VertexFormat VertexAttributeFormatToWGPU(SR_UTILS_NS::VertexAttributeFormat format, uint8_t count) {
        switch (format) {
            case SR_UTILS_NS::VertexAttributeFormat::Float32:
                switch (count) {
                    case 1: return wgpu::VertexFormat::Float32;
                    case 2: return wgpu::VertexFormat::Float32x2;
                    case 3: return wgpu::VertexFormat::Float32x3;
                    case 4: return wgpu::VertexFormat::Float32x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::Float16:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Float16x2;
                    case 4: return wgpu::VertexFormat::Float16x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::Int32:
                switch (count) {
                    case 1: return wgpu::VertexFormat::Sint32;
                    case 2: return wgpu::VertexFormat::Sint32x2;
                    case 3: return wgpu::VertexFormat::Sint32x3;
                    case 4: return wgpu::VertexFormat::Sint32x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::UInt32:
                switch (count) {
                    case 1: return wgpu::VertexFormat::Uint32;
                    case 2: return wgpu::VertexFormat::Uint32x2;
                    case 3: return wgpu::VertexFormat::Uint32x3;
                    case 4: return wgpu::VertexFormat::Uint32x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::Int16:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Sint16x2;
                    case 4: return wgpu::VertexFormat::Sint16x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::UInt16:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Uint16x2;
                    case 4: return wgpu::VertexFormat::Uint16x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::Int8:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Sint8x2;
                    case 4: return wgpu::VertexFormat::Sint8x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::UInt8:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Uint8x2;
                    case 4: return wgpu::VertexFormat::Uint8x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::UNorm8:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Unorm8x2;
                    case 4: return wgpu::VertexFormat::Unorm8x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::SNorm8:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Snorm8x2;
                    case 4: return wgpu::VertexFormat::Snorm8x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::UNorm16:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Unorm16x2;
                    case 4: return wgpu::VertexFormat::Unorm16x4;
                    default: break;
                }
                break;
            case SR_UTILS_NS::VertexAttributeFormat::SNorm16:
                switch (count) {
                    case 2: return wgpu::VertexFormat::Snorm16x2;
                    case 4: return wgpu::VertexFormat::Snorm16x4;
                    default: break;
                }
                break;
            default: break;
        }
        SR_WARN("VertexAttributeFormatToWGPU: unsupported format {}, count {}", static_cast<int>(format), count);
        return wgpu::VertexFormat::Float32x4;
    }

    static wgpu::PrimitiveTopology PrimitiveTopologyToWGPU(SR_GRAPH_NS::PrimitiveTopology topology) {
        switch (topology) {
            case SR_GRAPH_NS::PrimitiveTopology::PointList:  return wgpu::PrimitiveTopology::PointList;
            case SR_GRAPH_NS::PrimitiveTopology::LineList:   return wgpu::PrimitiveTopology::LineList;
            case SR_GRAPH_NS::PrimitiveTopology::LineStrip:  return wgpu::PrimitiveTopology::LineStrip;
            case SR_GRAPH_NS::PrimitiveTopology::TriangleStrip: return wgpu::PrimitiveTopology::TriangleStrip;
            default: return wgpu::PrimitiveTopology::TriangleList;
        }
    }

    static wgpu::CullMode CullModeToWGPU(SR_GRAPH_NS::CullMode mode) {
        switch (mode) {
            case SR_GRAPH_NS::CullMode::Front: return wgpu::CullMode::Front;
            case SR_GRAPH_NS::CullMode::Back:  return wgpu::CullMode::Back;
            default:                           return wgpu::CullMode::None;
        }
    }

    static wgpu::CompareFunction DepthCompareToWGPU(SR_GRAPH_NS::DepthCompare compare) {
        switch (compare) {
            case SR_GRAPH_NS::DepthCompare::Never:          return wgpu::CompareFunction::Never;
            case SR_GRAPH_NS::DepthCompare::Less:           return wgpu::CompareFunction::Less;
            case SR_GRAPH_NS::DepthCompare::Equal:          return wgpu::CompareFunction::Equal;
            case SR_GRAPH_NS::DepthCompare::LessOrEqual:    return wgpu::CompareFunction::LessEqual;
            case SR_GRAPH_NS::DepthCompare::Greater:        return wgpu::CompareFunction::Greater;
            case SR_GRAPH_NS::DepthCompare::NotEqual:       return wgpu::CompareFunction::NotEqual;
            case SR_GRAPH_NS::DepthCompare::GreaterOrEqual: return wgpu::CompareFunction::GreaterEqual;
            case SR_GRAPH_NS::DepthCompare::Always:         return wgpu::CompareFunction::Always;
            default:                                        return wgpu::CompareFunction::LessEqual;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Construction / destruction
    // ----------------------------------------------------------------------------------------------------------------

    WebGPUPipeline::WebGPUPipeline(const RenderContextPtr pRenderContext)
        : Super(pRenderContext)
    {
        m_supportedSampleCount = 1;
        m_internalData = new WebGPUPipelineInternalData();
        m_internalData->frameBuffers.Add(WebGPUFrameBuffer()); /// Add a dummy framebuffer at index 0 to avoid confusion with valid framebuffers.
    }

    WebGPUPipeline::~WebGPUPipeline() {
        SR_SAFE_DELETE_PTR(m_internalData);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DrawFrame
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::DrawFrame() {
        SR_TRACY_ZONE;

        Super::DrawFrame();

        if (!m_internalData->device || !m_internalData->queue || !m_internalData->activeEncoder || !m_internalData->surfaceTextureView) {
            return;
        }

        wgpu::RenderPassColorAttachment color{};
        color.view      = m_internalData->surfaceTextureView;
        color.loadOp    = wgpu::LoadOp::Clear;
        color.storeOp   = wgpu::StoreOp::Store;
        color.clearValue = {0.10f, 0.10f, 0.12f, 1.0f};

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments     = &color;

        auto pass = m_internalData->activeEncoder.BeginRenderPass(&passDesc);

        // ---- Scene geometry ----
        // Use the pipeline state tracked by the base class to issue draw calls.
        const int32_t shaderId    = m_state.shaderId;
        const int32_t descriptorId = m_state.descriptorSetId;
        const int32_t iboId       = m_state.IBOId;
        const int32_t vboId       = m_state.VBOId;

        if (shaderId >= 0 && m_internalData->shaderPrograms.IsAlive(shaderId)) {
            auto&& shaderProgram = m_internalData->shaderPrograms.At(shaderId);
            if (shaderProgram.renderPipeline) {
                pass.SetPipeline(shaderProgram.renderPipeline);
            }
        }

        if (descriptorId >= 0 && m_internalData->bindGroups.IsAlive(descriptorId)) {
            auto&& bindGroup = m_internalData->bindGroups.At(descriptorId);
            if (bindGroup.bindGroup) {
                pass.SetBindGroup(0, bindGroup.bindGroup, 0, nullptr);
            }
        }

        if (vboId >= 0 && m_internalData->VBOs.IsAlive(vboId)) {
            auto&& vbo = m_internalData->VBOs.At(vboId);
            if (vbo) {
                pass.SetVertexBuffer(0, vbo, 0, WGPU_WHOLE_SIZE);
            }
        }

        if (iboId >= 0 && m_internalData->IBOs.IsAlive(iboId)) {
            auto&& ibo = m_internalData->IBOs.At(iboId);
            if (ibo) {
                pass.SetIndexBuffer(ibo, wgpu::IndexFormat::Uint32, 0, WGPU_WHOLE_SIZE);
                // Vertex count is tracked by the engine; use draw call count from state if available
                pass.DrawIndexed(m_state.vertices > 0 ? m_state.vertices : 3,
                    m_drawInstancesCount, 0, 0, m_drawInstancesStart);
            }
        }
        else if (vboId >= 0 && m_internalData->VBOs.IsAlive(vboId)) {
            // Fallback: no IBO — draw arrays
            pass.Draw(m_state.vertices > 0 ? m_state.vertices : 3,
                m_drawInstancesCount, 0, m_drawInstancesStart);
        }

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
        m_internalData->surfaceCommandBuffer = nullptr;
        m_internalData->activeEncoder        = nullptr;
        m_internalData->surfaceTextureView   = nullptr;
        m_internalData->surfaceTexture       = nullptr;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Frame begin/end
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::OnFrameBuildBegin() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildBegin();

        m_internalData->instance.ProcessEvents();

        if (!m_internalData->device || !m_internalData->queue || !m_internalData->surface) {
            return;
        }

        // Resize-aware surface configuration.
        double cssW = 0.0, cssH = 0.0;
        if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
            constexpr uint32_t kMaxSurfaceDim = 8192;
            const uint32_t pxW = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssW)));
            const uint32_t pxH = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssH)));

            if (pxW != m_internalData->surfaceWidth || pxH != m_internalData->surfaceHeight) {
                m_internalData->surfaceWidth  = pxW;
                m_internalData->surfaceHeight = pxH;

                wgpu::SurfaceConfiguration config{};
                config.device      = m_internalData->device;
                config.format      = m_internalData->surfaceFormat != wgpu::TextureFormat::Undefined ? m_internalData->surfaceFormat : wgpu::TextureFormat::BGRA8Unorm;
                config.usage       = wgpu::TextureUsage::RenderAttachment;
                config.width       = pxW;
                config.height      = pxH;
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

        m_internalData->surfaceTexture     = surfaceTexture.texture;
        m_internalData->surfaceTextureView = m_internalData->surfaceTexture.CreateView();

        m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder();
        m_internalData->activeEncoder.SetLabel("Surface command encoder");
    }

    void WebGPUPipeline::OnFrameBuildEnd() {
        SR_TRACY_ZONE;
        Super::OnFrameBuildEnd();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Initialization
    // ----------------------------------------------------------------------------------------------------------------

    bool WebGPUPipeline::PreInit(const PipelinePreInitInfo& info) {
        SR_INFO("WebGPUPipeline::PreInit() : requesting WebGPU adapter and device...");

        EM_ASM({
            navigator.gpu.requestAdapter().then(a => {
                console.log("WebGPUPipeline::PreInit() : JS adapter works:", a);
            }).catch(e => {
                console.log("WebGPUPipeline::PreInit() : JS adapter failed:", e);
            });
        });

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

        m_internalData->instance.RequestAdapter(&options, wgpu::CallbackMode::AllowProcessEvents, [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                const std::string_view msgView =
                    (message.length != WGPU_STRLEN)
                        ? std::string_view(message.data, message.length)
                        : std::string_view(message.data ? message.data : "(unknown error)");
                SR_ERROR("WebGPUPipeline::PreInit() : failed to request adapter: {}", msgView);
                return;
            }

            SR_INFO("WebGPUPipeline::PreInit() : WebGPU adapter successfully obtained.");

            // ---- DeviceDescriptor with uncaptured-error callback ----
            // SetUncapturedErrorCallback lives on DeviceDescriptor (not on wgpu::Device).
            // It must be configured before RequestDevice is called.
            wgpu::DeviceDescriptor deviceDesc{};
            deviceDesc.SetUncapturedErrorCallback(
                [](const wgpu::Device& /*device*/, wgpu::ErrorType type, wgpu::StringView message) {
                    const char* typeStr = "Unknown";
                    switch (type) {
                        case wgpu::ErrorType::Validation:  typeStr = "Validation";  break;
                        case wgpu::ErrorType::OutOfMemory: typeStr = "OutOfMemory"; break;
                        case wgpu::ErrorType::Internal:    typeStr = "Internal";    break;
                        case wgpu::ErrorType::Unknown:     typeStr = "Unknown";     break;
                        case wgpu::ErrorType::NoError:     return;
                        default: break;
                    }
                    SR_ERROR("UncapturedErrorCallback() : WebGPU [{}] error: {}", typeStr,
                        message.length != WGPU_STRLEN
                            ? std::string_view(message.data, message.length)
                            : std::string_view(message.data ? message.data : "(null)"));
                }
            );

            adapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::AllowProcessEvents, [this](wgpu::RequestDeviceStatus status, wgpu::Device dev, wgpu::StringView message) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    SR_ERROR("WebGPUPipeline::PreInit() : failed to request device: {}",
                        message.length != WGPU_STRLEN
                            ? std::string_view(message.data, message.length)
                            : std::string_view(message.data ? message.data : "(unknown error)"));
                    return;
                }

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU device successfully obtained.");

                m_internalData->device = dev;
                m_internalData->queue  = m_internalData->device.GetQueue();

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU device and queue successfully created.");

                wgpu::SurfaceConfiguration config{};
                config.device      = m_internalData->device;
                config.format      = wgpu::TextureFormat::BGRA8Unorm;
                config.usage       = wgpu::TextureUsage::RenderAttachment;
                double cssW = 0.0, cssH = 0.0;
                if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
                    constexpr uint32_t kMaxSurfaceDim = 8192;
                    config.width  = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssW)));
                    config.height = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1.0, cssH)));
                }
                else {
                    config.width  = 800;
                    config.height = 600;
                }
                config.presentMode = wgpu::PresentMode::Fifo;

                m_internalData->surfaceFormat = config.format;
                m_internalData->surfaceWidth  = config.width;
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

    void* WebGPUPipeline::GetCurrentShaderHandle() const {
        const int32_t id = m_state.shaderId;
        if (id >= 0 && m_internalData && m_internalData->shaderPrograms.IsAlive(id)) {
            auto&& prog = m_internalData->shaderPrograms.At(id);
            if (prog.isCompute) {
                return reinterpret_cast<void*>(prog.computePipeline.Get());
            }
            return reinterpret_cast<void*>(prog.renderPipeline.Get());
        }
        return reinterpret_cast<void*>(1);
    }

    void* WebGPUPipeline::GetCurrentFBOHandle() const {
        const int32_t id = m_state.frameBufferId;
        if (id >= 0 && m_internalData && m_internalData->frameBuffers.IsAlive(id)) {
            auto&& fbo = m_internalData->frameBuffers.At(id);
            if (!fbo.colorAttachments.empty() && fbo.colorAttachments[0].texture) {
                return reinterpret_cast<void*>(fbo.colorAttachments[0].texture.Get());
            }
        }
        return reinterpret_cast<void*>(1);
    }

    WGPUTextureFormat WebGPUPipeline::GetSurfaceFormat() const {
        if (!m_internalData) {
            return WGPUTextureFormat_Undefined;
        }
        return static_cast<WGPUTextureFormat>(m_internalData->surfaceFormat);
    }

    WGPUTextureView WebGPUPipeline::GetTextureView(uint32_t textureId) const {
        if (!m_internalData) {
            return nullptr;
        }
        const auto id = static_cast<int32_t>(textureId);
        if (id < 0 || !m_internalData->textures.IsAlive(id)) {
            return nullptr;
        }
        return m_internalData->textures.At(id).view.Get();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Buffer allocations
    // ----------------------------------------------------------------------------------------------------------------

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

        return m_internalData->IBOs.Add(std::move(pBuffer));
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateUBO(uint32_t uboSize) {
        if (uboSize == 0) {
            return SR_ID_INVALID;
        }

        wgpu::BufferDescriptor desc{};
        desc.size  = (uboSize + 15) & ~size_t(15); // 16-byte alignment
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
        desc.size = (ssboSize + 3) & ~size_t(3);

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
                desc.usage             = wgpu::BufferUsage::MapWrite | wgpu::BufferUsage::CopySrc;
                desc.mappedAtCreation  = true;
                break;
            }
            case SSBOUsage::CPUOnly: {
                desc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
                break;
            }
            case SSBOUsage::AutoPreferHost:
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

    // ----------------------------------------------------------------------------------------------------------------
    // Shader program allocation
    // ----------------------------------------------------------------------------------------------------------------

    SR_NODISCARD int32_t WebGPUPipeline::AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) {
        if (!m_internalData->device) {
            SR_ERROR("WebGPUPipeline::AllocateShaderProgram() : device is null!");
            return SR_ID_INVALID;
        }

        SR_UTILS_NS::StringView shaderName;

        const bool isCompute = (createInfo.shaderType == SR_SRSL_NS::ShaderType::Compute);

        // ---- 1. Read WGSL source ----
        // WGSL is compiled as a single combined module containing all entry points.
        // The WGSLCodeGenerator emits the same combined source under every stage key
        // (Vertex, Fragment, Compute), so we read the first available stage file.
        // The cache path is: <ResourceManager::GetCachePath>/Shaders/<stage.path>
        std::string wgslSource;
        const auto cacheShadersRoot = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders");

        auto readStageFile = [&](ShaderStage stage) -> bool {
            auto it = createInfo.stages.find(stage);
            if (it == createInfo.stages.end() || it->second.path.empty()) {
                return false;
            }
            // The path stored in stages is relative to the cache/Shaders root
            const auto fullPath = cacheShadersRoot.Concat(it->second.path);
            std::ifstream f(fullPath.ToStringRef(), std::ios::in);
            if (!f.is_open()) {
                return false;
            }
            shaderName = SR_UTILS_NS::StringView(it->second.path.c_str(), it->second.path.size());
            wgslSource = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            return !wgslSource.empty();
        };

        // Read from first available stage (all stages hold the same combined WGSL source)
        if (!readStageFile(ShaderStage::Vertex) &&
            !readStageFile(ShaderStage::Fragment) &&
            !readStageFile(ShaderStage::Compute))
        {
            SR_ERROR("WebGPUPipeline::AllocateShaderProgram() : no WGSL source found! Shader stages registered: {}",
                static_cast<int>(createInfo.stages.size()));
            return SR_ID_INVALID;
        }

        // ---- 2. Create ShaderModule ----
        wgpu::ShaderSourceWGSL wgslDesc{};
        wgslDesc.code = wgpu::StringView(wgslSource.c_str(), wgslSource.size());

        wgpu::ShaderModuleDescriptor shaderDesc{};
        shaderDesc.nextInChain = &wgslDesc;
        shaderDesc.label = shaderName.c_str();

        m_internalData->device.PushErrorScope(wgpu::ErrorFilter::Validation);

        wgpu::ShaderModule shaderModule = m_internalData->device.CreateShaderModule(&shaderDesc);

        m_internalData->device.PopErrorScope(wgpu::CallbackMode::AllowProcessEvents, [wgslSource](wgpu::PopErrorScopeStatus /*status*/, wgpu::ErrorType type, wgpu::StringView message) {
            if (type != wgpu::ErrorType::NoError) {
                const std::string_view msgView =
                    (message.length != WGPU_STRLEN)
                        ? std::string_view(message.data, message.length)
                        : std::string_view(message.data ? message.data : "(no message)");
                SR_ERROR("WebGPUPipeline : WGSL shader compilation failed: {}", msgView);
                std::istringstream stream(wgslSource);
                std::string line;
                uint32_t lineNum = 1;
                SR_UTILS_NS::String text;
                while (std::getline(stream, line)) {
                    text += "  {:>4} | {}\n"_format(lineNum++, line);
                }
                SR_ERROR("WebGPUPipeline : dumping WGSL source:\n{}", text);
            }
        });

        if (!shaderModule) {
            SR_ERROR("WebGPUPipeline::AllocateShaderProgram() : CreateShaderModule returned null (WGSL parse likely failed)!");
            return SR_ID_INVALID;
        }

        // ---- 3. Build BindGroupLayout entries from uniforms ----
        // The WGSL generator places resources in two bind groups:
        //   @group(0) — UBOs and SSBOs (sequential bindings)
        //   @group(1) — textures + samplers (pairs, sequential bindings starting at 0)
        // We must declare a BindGroupLayout for every group index the shader references.

        // Group 0: UBOs and SSBOs
        SR_UTILS_NS::Vector<wgpu::BindGroupLayoutEntry> bglEntries0;
        uint32_t nextBinding0 = 0;

        for (auto&& uniform : createInfo.uniforms) {
            if (uniform.type != LayoutBinding::Uniform && uniform.type != LayoutBinding::SSBO) {
                continue;
            }
            wgpu::BindGroupLayoutEntry entry{};
            entry.binding = nextBinding0++;

            if (uniform.type == LayoutBinding::Uniform) {
                entry.visibility            = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
                entry.buffer.type           = wgpu::BufferBindingType::Uniform;
                entry.buffer.minBindingSize = uniform.size;
            }
            else { // SSBO
                // WebGPU: read-write storage NOT allowed in Vertex stage
                entry.visibility            = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
                entry.buffer.type           = wgpu::BufferBindingType::Storage;
                entry.buffer.minBindingSize = uniform.size;
            }
            bglEntries0.push_back(entry);
        }

        // Group 1: textures + samplers (each sampler2D/attachment occupies 2 bindings: texture then sampler)
        SR_UTILS_NS::Vector<wgpu::BindGroupLayoutEntry> bglEntries1;
        uint32_t nextBinding1 = 0;

        for (auto&& uniform : createInfo.uniforms) {
            if (uniform.type != LayoutBinding::Sampler2D && uniform.type != LayoutBinding::Attachhment) {
                continue;
            }
            const bool isAttachment = (uniform.type == LayoutBinding::Attachhment);
            const wgpu::ShaderStage texVis = isAttachment
                ? wgpu::ShaderStage::Fragment
                : (wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute);

            wgpu::BindGroupLayoutEntry texEntry{};
            texEntry.binding               = nextBinding1++;
            texEntry.visibility            = texVis;
            texEntry.texture.sampleType    = wgpu::TextureSampleType::Float;
            texEntry.texture.viewDimension = wgpu::TextureViewDimension::e2D;
            bglEntries1.push_back(texEntry);

            wgpu::BindGroupLayoutEntry samplerEntry{};
            samplerEntry.binding      = nextBinding1++;
            samplerEntry.visibility   = texVis;
            samplerEntry.sampler.type = isAttachment
                ? wgpu::SamplerBindingType::NonFiltering
                : wgpu::SamplerBindingType::Filtering;
            bglEntries1.push_back(samplerEntry);
        }

        // Create BindGroupLayout for group 0 (always present, even if empty)
        wgpu::BindGroupLayoutDescriptor bglDesc0{};
        bglDesc0.entryCount = static_cast<size_t>(bglEntries0.size());
        bglDesc0.entries    = bglEntries0.empty() ? nullptr : bglEntries0.data();

        wgpu::BindGroupLayout bindGroupLayout = m_internalData->device.CreateBindGroupLayout(&bglDesc0);

        // Create BindGroupLayout for group 1 (only when samplers are present)
        wgpu::BindGroupLayout bindGroupLayout1;
        if (!bglEntries1.empty()) {
            wgpu::BindGroupLayoutDescriptor bglDesc1{};
            bglDesc1.entryCount = static_cast<size_t>(bglEntries1.size());
            bglDesc1.entries    = bglEntries1.data();
            bindGroupLayout1 = m_internalData->device.CreateBindGroupLayout(&bglDesc1);
        }

        // Build the pipeline layout covering all used bind group indices.
        // If group 1 is used, we must include both group 0 and group 1 layouts.
        wgpu::BindGroupLayout bglArray[2] = { bindGroupLayout, bindGroupLayout1 };

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = bglEntries1.empty() ? 1u : 2u;
        plDesc.bindGroupLayouts     = bglArray;
        plDesc.label = shaderName.c_str();

        wgpu::PipelineLayout pipelineLayout = m_internalData->device.CreatePipelineLayout(&plDesc);
        if (!pipelineLayout) {
            SR_ERROR("WebGPUPipeline::AllocateShaderProgram() : failed to create pipeline layout!");
            return SR_ID_INVALID;
        }

        WebGPUShaderProgram program;
        program.bindGroupLayout  = bindGroupLayout;
        program.bindGroupLayout1 = bindGroupLayout1;
        program.pipelineLayout  = pipelineLayout;
        program.isCompute       = isCompute;

        // ---- 4a. Compute pipeline ----
        if (isCompute) {
            wgpu::ComputePipelineDescriptor cpDesc{};
            cpDesc.layout              = pipelineLayout;
            cpDesc.compute.module      = shaderModule;
            cpDesc.compute.entryPoint  = "compute";

            program.computePipeline = m_internalData->device.CreateComputePipeline(&cpDesc);
            if (!program.computePipeline) {
                SR_ERROR("WebGPUPipeline::AllocateShaderProgram() : failed to create compute pipeline!");
                return SR_ID_INVALID;
            }
        }
        // ---- 4b. Render pipeline ----
        else {
            // Vertex buffer layout
            SR_UTILS_NS::Vector<wgpu::VertexAttribute> vertexAttributes;
            uint64_t stride = 0;

            createInfo.vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& attr, uint32_t location) {
                wgpu::VertexAttribute vkAttr{};
                vkAttr.format         = VertexAttributeFormatToWGPU(attr.format, attr.count);
                vkAttr.offset         = stride;
                vkAttr.shaderLocation = location;
                vertexAttributes.push_back(vkAttr);
                stride += SR_UTILS_NS::VertexAttributeDescription::GetAttributeSizeInBytes(attr.format, attr.count);
            });

            wgpu::VertexBufferLayout vertexBufferLayout{};
            vertexBufferLayout.arrayStride    = stride;
            vertexBufferLayout.stepMode       = wgpu::VertexStepMode::Vertex;
            vertexBufferLayout.attributeCount = static_cast<size_t>(vertexAttributes.size());
            vertexBufferLayout.attributes     = vertexAttributes.data();

            // Determine output color format from FBO or surface
            wgpu::TextureFormat colorFormat = m_internalData->surfaceFormat != wgpu::TextureFormat::Undefined
                ? m_internalData->surfaceFormat
                : wgpu::TextureFormat::BGRA8Unorm;

            bool depthEnabled = createInfo.depthTest || createInfo.depthWrite;

            wgpu::ColorTargetState colorTarget{};
            colorTarget.format = colorFormat;

            if (createInfo.blendEnabled) {
                wgpu::BlendState blend{};
                blend.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
                blend.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                blend.color.operation = wgpu::BlendOperation::Add;
                blend.alpha.srcFactor = wgpu::BlendFactor::One;
                blend.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                blend.alpha.operation = wgpu::BlendOperation::Add;

                static wgpu::BlendState blendState = blend;
                colorTarget.blend = &blendState;
            }

            wgpu::FragmentState fragmentState{};
            fragmentState.module      = shaderModule;
            fragmentState.entryPoint  = "fragment";
            fragmentState.targetCount = 1;
            fragmentState.targets     = &colorTarget;

            wgpu::DepthStencilState depthStencilState{};
            if (depthEnabled) {
                depthStencilState.format              = wgpu::TextureFormat::Depth32Float;
                depthStencilState.depthWriteEnabled   = createInfo.depthWrite;
                depthStencilState.depthCompare        = DepthCompareToWGPU(createInfo.depthCompare);
            }

            wgpu::RenderPipelineDescriptor rpDesc{};
            rpDesc.layout = pipelineLayout;

            rpDesc.vertex.module     = shaderModule;
            rpDesc.vertex.entryPoint = "vertex";
            if (!vertexAttributes.empty()) {
                rpDesc.vertex.bufferCount = 1;
                rpDesc.vertex.buffers     = &vertexBufferLayout;
            }

            rpDesc.primitive.topology         = PrimitiveTopologyToWGPU(createInfo.primitiveTopology);
            rpDesc.primitive.stripIndexFormat  = wgpu::IndexFormat::Undefined;
            rpDesc.primitive.cullMode          = CullModeToWGPU(createInfo.cullMode);
            rpDesc.primitive.frontFace         = wgpu::FrontFace::CCW;

            rpDesc.fragment = &fragmentState;

            if (depthEnabled) {
                rpDesc.depthStencil = &depthStencilState;
            }

            rpDesc.multisample.count = 1;
            rpDesc.multisample.mask  = 0xFFFFFFFF;

            program.renderPipeline = m_internalData->device.CreateRenderPipeline(&rpDesc);
            if (!program.renderPipeline) {
                SR_ERROR("WebGPUPipeline::AllocateShaderProgram() : failed to create render pipeline!");
                return SR_ID_INVALID;
            }
        }

        ++m_state.operations;
        ++m_state.allocations;

        return m_internalData->shaderPrograms.Add(std::move(program));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Texture allocation
    // ----------------------------------------------------------------------------------------------------------------

    SR_NODISCARD int32_t WebGPUPipeline::AllocateTexture(const SRTextureCreateInfo& createInfo) {
        if (!m_internalData->device) {
            SR_ERROR("WebGPUPipeline::AllocateTexture() : device is null!");
            return SR_ID_INVALID;
        }

        if (createInfo.width == 0 || createInfo.height == 0) {
            SR_ERROR("WebGPUPipeline::AllocateTexture() : zero dimensions!");
            return SR_ID_INVALID;
        }

        wgpu::TextureFormat format = ImageFormatToWGPU(createInfo.format);
        bool isDepth = IsDepthFormat(format);

        wgpu::TextureUsage usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        if (isDepth) {
            usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
        }

        wgpu::TextureDescriptor texDesc{};
        texDesc.size            = { createInfo.width, createInfo.height, 1 };
        texDesc.format          = format;
        texDesc.usage           = usage;
        texDesc.mipLevelCount   = std::max<uint32_t>(1, createInfo.mipLevels);
        texDesc.sampleCount     = 1;
        texDesc.dimension       = wgpu::TextureDimension::e2D;

        wgpu::Texture texture = m_internalData->device.CreateTexture(&texDesc);
        if (!texture) {
            SR_ERROR("WebGPUPipeline::AllocateTexture() : failed to create texture!");
            return SR_ID_INVALID;
        }

        // Upload data if provided
        if (createInfo.pData && createInfo.imageSize > 0 && !isDepth) {
            wgpu::TexelCopyTextureInfo dst{};
            dst.texture  = texture;
            dst.mipLevel = 0;
            dst.origin   = { 0, 0, 0 };
            dst.aspect   = wgpu::TextureAspect::All;

            uint32_t channelCount = GetChannelCount(createInfo.format);
            if (channelCount == 0) { channelCount = 4; }

            wgpu::TexelCopyBufferLayout layout{};
            layout.offset       = 0;
            layout.bytesPerRow  = createInfo.width * channelCount;
            layout.rowsPerImage = createInfo.height;

            wgpu::Extent3D extent{ createInfo.width, createInfo.height, 1 };
            m_internalData->queue.WriteTexture(&dst, createInfo.pData, createInfo.imageSize, &layout, &extent);
        }

        // Create view
        wgpu::TextureViewDescriptor viewDesc{};
        viewDesc.format          = format;
        viewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        viewDesc.mipLevelCount   = texDesc.mipLevelCount;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect          = isDepth ? wgpu::TextureAspect::DepthOnly : wgpu::TextureAspect::All;

        wgpu::TextureView view = texture.CreateView(&viewDesc);

        // Create sampler
        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU = AddressModeToWGPU(createInfo.addressMode);
        samplerDesc.addressModeV = AddressModeToWGPU(createInfo.addressMode);
        samplerDesc.addressModeW = AddressModeToWGPU(createInfo.addressMode);
        samplerDesc.magFilter    = TextureFilterToWGPU(createInfo.filter);
        samplerDesc.minFilter    = TextureFilterToWGPU(createInfo.filter);
        samplerDesc.mipmapFilter = TextureFilterToMipWGPU(createInfo.filter);
        samplerDesc.maxAnisotropy = 1;

        wgpu::Sampler sampler = m_internalData->device.CreateSampler(&samplerDesc);

        WebGPUTexture webTexture;
        webTexture.texture = std::move(texture);
        webTexture.view    = std::move(view);
        webTexture.sampler = std::move(sampler);

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += static_cast<uint32_t>(createInfo.imageSize);

        return m_internalData->textures.Add(std::move(webTexture));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FrameBuffer allocation
    // ----------------------------------------------------------------------------------------------------------------

    SR_NODISCARD int32_t WebGPUPipeline::AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) {
        if (!m_internalData->device) {
            SR_ERROR("WebGPUPipeline::AllocateFrameBuffer() : device is null!");
            return SR_ID_INVALID;
        }

        const uint32_t w = static_cast<uint32_t>(std::max(1, createInfo.size.x));
        const uint32_t h = static_cast<uint32_t>(std::max(1, createInfo.size.y));

        WebGPUFrameBuffer fbo;
        fbo.width  = w;
        fbo.height = h;

        // Color attachments
        if (createInfo.colors) {
            for (auto&& colorLayer : *createInfo.colors) {
                WebGPUFrameBuffer::Attachment att;
                // For color layers, resolve Auto/Unknown/None to RGBA8_UNORM (not depth)
                att.format = ImageFormatToWGPU(colorLayer.format, false);

                wgpu::TextureDescriptor texDesc{};
                texDesc.size          = { w, h, 1 };
                texDesc.format        = att.format;
                texDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
                texDesc.mipLevelCount = 1;
                texDesc.sampleCount   = std::max<uint8_t>(1, createInfo.sampleCount);
                texDesc.dimension     = wgpu::TextureDimension::e2D;

                wgpu::SamplerDescriptor samplerDesc{};
                samplerDesc.addressModeU  = wgpu::AddressMode::ClampToEdge;
                samplerDesc.addressModeV  = wgpu::AddressMode::ClampToEdge;
                samplerDesc.addressModeW  = wgpu::AddressMode::ClampToEdge;
                samplerDesc.magFilter     = wgpu::FilterMode::Linear;
                samplerDesc.minFilter     = wgpu::FilterMode::Linear;
                samplerDesc.mipmapFilter  = wgpu::MipmapFilterMode::Linear;
                samplerDesc.maxAnisotropy = 1;

                WebGPUTexture webTexture;

                att.texture = m_internalData->device.CreateTexture(&texDesc);
                if (att.texture) {
                    att.view = att.texture.CreateView();
                    webTexture.view = att.texture.CreateView();
                }

                for (auto&& oldTexture : colorLayer.texture) {
                    if (oldTexture != SR_ID_INVALID) {
                        m_internalData->textures.RemoveByIndex(oldTexture);
                    }
                }
                colorLayer.texture.clear();

                webTexture.view    = att.view;
                webTexture.sampler = m_internalData->device.CreateSampler(&samplerDesc);

                const auto colorId = m_internalData->textures.Add(std::move(webTexture));
                colorLayer.texture.emplace_back(colorId);

                fbo.colorAttachments.push_back(std::move(att));
            }
        }

        // Depth attachment
        if (createInfo.pDepth) {
            // Always use isDepthHint=true so Auto/Unknown/None resolves to Depth32Float
            wgpu::TextureFormat depthFmt = ImageFormatToWGPU(createInfo.pDepth->format, true);

            fbo.depthAttachment.format = depthFmt;

            wgpu::TextureDescriptor depthDesc{};
            depthDesc.size          = { w, h, 1 };
            depthDesc.format        = depthFmt;
            depthDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
            depthDesc.mipLevelCount = 1;
            depthDesc.sampleCount   = std::max<uint8_t>(1, createInfo.sampleCount);
            depthDesc.dimension     = wgpu::TextureDimension::e2D;

            fbo.depthAttachment.texture = m_internalData->device.CreateTexture(&depthDesc);
            if (fbo.depthAttachment.texture) {
                wgpu::TextureViewDescriptor dvDesc{};
                dvDesc.format          = depthFmt;
                dvDesc.dimension       = wgpu::TextureViewDimension::e2D;
                dvDesc.mipLevelCount   = 1;
                dvDesc.arrayLayerCount = 1;
                dvDesc.aspect          = wgpu::TextureAspect::DepthOnly;

                fbo.depthAttachment.view = fbo.depthAttachment.texture.CreateView(&dvDesc);
            }
        }

        // If no explicit FBO attachments, create a default color + depth
        if (fbo.colorAttachments.empty()) {
            WebGPUFrameBuffer::Attachment att;
            att.format = m_internalData->surfaceFormat != wgpu::TextureFormat::Undefined
                       ? m_internalData->surfaceFormat
                       : wgpu::TextureFormat::BGRA8Unorm;

            wgpu::TextureDescriptor texDesc{};
            texDesc.size          = { w, h, 1 };
            texDesc.format        = att.format;
            texDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
            texDesc.mipLevelCount = 1;
            texDesc.sampleCount   = 1;
            texDesc.dimension     = wgpu::TextureDimension::e2D;

            att.texture = m_internalData->device.CreateTexture(&texDesc);
            if (att.texture) {
                att.view = att.texture.CreateView();
            }
            fbo.colorAttachments.push_back(std::move(att));
        }

        if (!fbo.depthAttachment.texture) {
            fbo.depthAttachment.format = wgpu::TextureFormat::Depth32Float;

            wgpu::TextureDescriptor depthDesc{};
            depthDesc.size          = { w, h, 1 };
            depthDesc.format        = wgpu::TextureFormat::Depth32Float;
            depthDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
            depthDesc.mipLevelCount = 1;
            depthDesc.sampleCount   = 1;
            depthDesc.dimension     = wgpu::TextureDimension::e2D;

            fbo.depthAttachment.texture = m_internalData->device.CreateTexture(&depthDesc);
            if (fbo.depthAttachment.texture) {
                wgpu::TextureViewDescriptor dvDesc{};
                dvDesc.format          = wgpu::TextureFormat::Depth32Float;
                dvDesc.dimension       = wgpu::TextureViewDimension::e2D;
                dvDesc.mipLevelCount   = 1;
                dvDesc.arrayLayerCount = 1;
                dvDesc.aspect          = wgpu::TextureAspect::DepthOnly;
                fbo.depthAttachment.view = fbo.depthAttachment.texture.CreateView(&dvDesc);
            }
        }

        // Register texture IDs in the color layer vectors
        if (createInfo.colors) {
            for (size_t i = 0; i < createInfo.colors->size() && i < fbo.colorAttachments.size(); ++i) {
                // Each color layer holds a list of texture IDs. We store the FBO attachment index.
                // The engine uses these IDs with BindTexture / BindAttachment.
            }
        }

        ++m_state.operations;
        ++m_state.allocations;

        return m_internalData->frameBuffers.Add(std::move(fbo));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // CubeMap allocation
    // ----------------------------------------------------------------------------------------------------------------

    SR_NODISCARD int32_t WebGPUPipeline::AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) {
        if (!m_internalData->device) {
            SR_ERROR("WebGPUPipeline::AllocateCubeMap() : device is null!");
            return SR_ID_INVALID;
        }

        if (createInfo.width == 0 || createInfo.height == 0) {
            SR_ERROR("WebGPUPipeline::AllocateCubeMap() : zero dimensions!");
            return SR_ID_INVALID;
        }

        wgpu::TextureDescriptor texDesc{};
        texDesc.size            = { createInfo.width, createInfo.height, 6 };
        texDesc.format          = wgpu::TextureFormat::RGBA8Unorm;
        texDesc.usage           = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        texDesc.mipLevelCount   = 1;
        texDesc.sampleCount     = 1;
        texDesc.dimension       = wgpu::TextureDimension::e2D;

        wgpu::Texture texture = m_internalData->device.CreateTexture(&texDesc);
        if (!texture) {
            SR_ERROR("WebGPUPipeline::AllocateCubeMap() : failed to create cube texture!");
            return SR_ID_INVALID;
        }

        const uint64_t faceSize = static_cast<uint64_t>(createInfo.width) * createInfo.height * 4; // RGBA8
        for (uint32_t face = 0; face < 6; ++face) {
            if (!createInfo.data[face]) { continue; }

            wgpu::TexelCopyTextureInfo dst{};
            dst.texture  = texture;
            dst.mipLevel = 0;
            dst.origin   = { 0, 0, face };
            dst.aspect   = wgpu::TextureAspect::All;

            wgpu::TexelCopyBufferLayout layout{};
            layout.offset       = 0;
            layout.bytesPerRow  = createInfo.width * 4;
            layout.rowsPerImage = createInfo.height;

            wgpu::Extent3D extent{ createInfo.width, createInfo.height, 1 };
            m_internalData->queue.WriteTexture(&dst, createInfo.data[face], faceSize, &layout, &extent);
        }

        // Cube view
        wgpu::TextureViewDescriptor viewDesc{};
        viewDesc.format          = wgpu::TextureFormat::RGBA8Unorm;
        viewDesc.dimension       = wgpu::TextureViewDimension::Cube;
        viewDesc.mipLevelCount   = 1;
        viewDesc.arrayLayerCount = 6;
        viewDesc.aspect          = wgpu::TextureAspect::All;

        wgpu::TextureView view = texture.CreateView(&viewDesc);

        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU  = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeV  = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeW  = wgpu::AddressMode::ClampToEdge;
        samplerDesc.magFilter     = wgpu::FilterMode::Linear;
        samplerDesc.minFilter     = wgpu::FilterMode::Linear;
        samplerDesc.mipmapFilter  = wgpu::MipmapFilterMode::Linear;
        samplerDesc.maxAnisotropy = 1;

        wgpu::Sampler sampler = m_internalData->device.CreateSampler(&samplerDesc);

        WebGPUTexture webTexture;
        webTexture.texture = std::move(texture);
        webTexture.view    = std::move(view);
        webTexture.sampler = std::move(sampler);

        ++m_state.operations;
        ++m_state.allocations;

        return m_internalData->textures.Add(std::move(webTexture));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Descriptor set (BindGroup)
    // ----------------------------------------------------------------------------------------------------------------

    SR_NODISCARD int32_t WebGPUPipeline::AllocDescriptorSet(const SR_UTILS_NS::Vector<DescriptorType>& types) {
        if (!m_internalData->device) {
            return SR_ID_INVALID;
        }

        // Build BindGroupLayoutEntries to match the types array
        SR_UTILS_NS::Vector<wgpu::BindGroupLayoutEntry> entries;
        entries.reserve(types.size());

        for (uint32_t i = 0; i < static_cast<uint32_t>(types.size()); ++i) {
            wgpu::BindGroupLayoutEntry entry{};
            entry.binding    = i;
            entry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;

            switch (types[i]) {
                case DescriptorType::Uniform:
                    entry.buffer.type = wgpu::BufferBindingType::Uniform;
                    break;
                case DescriptorType::Storage:
                    // WebGPU: read-write storage NOT allowed in Vertex
                    entry.visibility  = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
                    entry.buffer.type = wgpu::BufferBindingType::Storage;
                    break;
                case DescriptorType::CombinedImage:
                    entry.texture.sampleType    = wgpu::TextureSampleType::Float;
                    entry.texture.viewDimension = wgpu::TextureViewDimension::e2D;
                    break;
                default:
                    entry.buffer.type = wgpu::BufferBindingType::Uniform;
                    break;
            }
            entries.push_back(entry);
        }

        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = entries.size();
        bglDesc.entries    = entries.data();

        wgpu::BindGroupLayout bgl = m_internalData->device.CreateBindGroupLayout(&bglDesc);
        if (!bgl) {
            SR_ERROR("WebGPUPipeline::AllocDescriptorSet() : failed to create bind group layout!");
            return SR_ID_INVALID;
        }

        // Create empty BindGroup (resources are bound later via UpdateDescriptorSets)
        SR_UTILS_NS::Vector<wgpu::BindGroupEntry> bgEntries;
        bgEntries.reserve(types.size());

        for (uint32_t i = 0; i < static_cast<uint32_t>(types.size()); ++i) {
            wgpu::BindGroupEntry bgEntry{};
            bgEntry.binding = i;

            switch (types[i]) {
                case DescriptorType::Uniform:
                case DescriptorType::Storage: {
                    // Attach a dummy zero-size buffer for initial creation
                    // (will be updated with UpdateDescriptorSets)
                    wgpu::BufferDescriptor dummyDesc{};
                    dummyDesc.size  = 16;
                    dummyDesc.usage = (types[i] == DescriptorType::Uniform)
                        ? (wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst)
                        : (wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                    bgEntry.buffer = m_internalData->device.CreateBuffer(&dummyDesc);
                    bgEntry.size   = 16;
                    break;
                }
                case DescriptorType::CombinedImage: {
                    // Attach a 1×1 white texture as placeholder
                    if (m_noneTextureId >= 0 && m_internalData->textures.IsAlive(m_noneTextureId)) {
                        bgEntry.textureView = m_internalData->textures.At(m_noneTextureId).view;
                    }
                    break;
                }
                default: break;
            }
            bgEntries.push_back(bgEntry);
        }

        wgpu::BindGroupDescriptor bgDesc{};
        bgDesc.layout     = bgl;
        bgDesc.entryCount = bgEntries.size();
        bgDesc.entries    = bgEntries.data();

        wgpu::BindGroup bindGroup = m_internalData->device.CreateBindGroup(&bgDesc);
        if (!bindGroup) {
            SR_ERROR("WebGPUPipeline::AllocDescriptorSet() : failed to create bind group!");
            return SR_ID_INVALID;
        }

        WebGPUBindGroup bg;
        bg.bindGroup = std::move(bindGroup);

        ++m_state.operations;
        ++m_state.allocations;

        return m_internalData->bindGroups.Add(std::move(bg));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Command buffer (stub)
    // ----------------------------------------------------------------------------------------------------------------

    int32_t WebGPUPipeline::AllocateCmdBuffer() {
        // WebGPU command buffers are created per-frame via CommandEncoder; no persistent IDs needed.
        static int32_t uniqueId = 0;
        return ++uniqueId;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // SSBO / UBO update via WriteBuffer (WebGPU doesn't support persistent CPU mappings for Storage buffers)
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::UpdateUBO(uint32_t UBO, void* pData, uint64_t size, bool sizesMustBeEqual) {
        if (!m_internalData->queue || !pData || size == 0) {
            return;
        }
        if (m_internalData->UBOs.IsAlive(static_cast<int32_t>(UBO))) {
            auto&& buf = m_internalData->UBOs.At(static_cast<int32_t>(UBO));
            const uint64_t aligned = (size + 15) & ~uint64_t(15);
            m_internalData->queue.WriteBuffer(buf, 0, pData, std::min(aligned, buf.GetSize()));
        }
    }

    void WebGPUPipeline::UpdateSSBO(uint32_t SSBO, void* pData, uint64_t size) {
        if (!m_internalData->queue || !pData || size == 0) {
            return;
        }
        if (m_internalData->SSBOs.IsAlive(static_cast<int32_t>(SSBO))) {
            auto&& buf = m_internalData->SSBOs.At(static_cast<int32_t>(SSBO));
            m_internalData->queue.WriteBuffer(buf, 0, pData, std::min(size, buf.GetSize()));
        }
    }

    static SR_HTYPES_NS::FastMemoryArray<uint8_t> g_tempSSBOBuffer;

    bool WebGPUPipeline::MapSSBO(uint32_t SSBO, void** ppData) {
        if (!m_internalData->queue || !ppData) {
            return false;
        }
        if (m_internalData->SSBOs.IsAlive(static_cast<int32_t>(SSBO))) {
            /*auto&& buf = m_internalData->SSBOs.At(static_cast<int32_t>(SSBO));
            const uint64_t size = buf.GetSize();
            g_tempSSBOBuffer.resize(size);
            *ppData = g_tempSSBOBuffer.data();

            auto&& device = m_internalData->device;

            wgpu::BufferDescriptor desc{};
            desc.size  = size;
            desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
            const auto  readBuffer = device.CreateBuffer(&desc);

            const auto encoder = device.CreateCommandEncoder();

            encoder.CopyBufferToBuffer(
                buf,   // источник
                0,
                readBuffer,  // приемник
                0,
                size
            );

            const auto commandBuffer = encoder.Finish();
            m_internalData->queue.Submit(1, &commandBuffer);

            //await readBuffer.mapAsync(GPUMapMode.READ);

            const data = readBuffer.getMappedRange();
            const values = new Float32Array(data.slice(0));

            readBuffer.unmap();


            return true;*/
        }
        return false;
    }

    void WebGPUPipeline::UnMapSSBO(uint32_t SSBO) {
    }

    void WebGPUPipeline::FlushSSBO(uint32_t SSBO, uint64_t offset, uint64_t size) {
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Handles (for external use / debugging)
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::GetShaderHandles(SR_UTILS_NS::Vector<void*>& handles) const {
        handles.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_internalData->shaderPrograms.GetCapacity()); ++i) {
            if (!m_internalData->shaderPrograms.IsAlive(i)) { continue; }
            auto&& prog = m_internalData->shaderPrograms.At(i);
            void* h = prog.isCompute
                ? reinterpret_cast<void*>(prog.computePipeline.Get())
                : reinterpret_cast<void*>(prog.renderPipeline.Get());
            handles.emplace_back(h);
        }
        if (handles.empty()) {
            handles.emplace_back(reinterpret_cast<void*>(1));
        }
    }

    void WebGPUPipeline::GetFBOHandles(SR_UTILS_NS::Vector<void*>& handles) const {
        handles.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_internalData->frameBuffers.GetCapacity()); ++i) {
            if (!m_internalData->frameBuffers.IsAlive(i)) { continue; }
            auto&& fbo = m_internalData->frameBuffers.At(i);
            if (!fbo.colorAttachments.empty() && fbo.colorAttachments[0].texture) {
                handles.emplace_back(reinterpret_cast<void*>(fbo.colorAttachments[0].texture.Get()));
            }
        }
        if (handles.empty()) {
            handles.emplace_back(reinterpret_cast<void*>(1));
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Free methods
    // ----------------------------------------------------------------------------------------------------------------

    bool WebGPUPipeline::FreeDescriptorSet(int32_t* id) {
        if (*id >= 0 && m_internalData->bindGroups.IsAlive(*id)) {
            m_internalData->bindGroups.At(*id).Destroy();
            m_internalData->bindGroups.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        return true;
    }

    bool WebGPUPipeline::FreeVBO(int32_t* id) {
        if (*id >= 0 && m_internalData->VBOs.IsAlive(*id)) {
            m_internalData->VBOs.At(*id).Destroy();
            m_internalData->VBOs.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeIBO(int32_t* id) {
        if (*id >= 0 && m_internalData->IBOs.IsAlive(*id)) {
            m_internalData->IBOs.At(*id).Destroy();
            m_internalData->IBOs.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeUBO(int32_t* id) {
        if (*id >= 0 && m_internalData->UBOs.IsAlive(*id)) {
            m_internalData->UBOs.At(*id).Destroy();
            m_internalData->UBOs.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeFBO(int32_t* id) {
        if (*id >= 0 && m_internalData->frameBuffers.IsAlive(*id)) {
            m_internalData->frameBuffers.At(*id).Destroy();
            m_internalData->frameBuffers.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeSSBO(int32_t* id) {
        if (*id >= 0 && m_internalData->SSBOs.IsAlive(*id)) {
            m_internalData->SSBOs.At(*id).Destroy();
            m_internalData->SSBOs.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeCubeMap(int32_t* id) {
        // Cube maps are stored in the textures pool
        if (*id >= 0 && m_internalData->textures.IsAlive(*id)) {
            m_internalData->textures.At(*id).Destroy();
            m_internalData->textures.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeShader(int32_t* id) {
        if (*id >= 0 && m_internalData->shaderPrograms.IsAlive(*id)) {
            m_internalData->shaderPrograms.At(*id).Destroy();
            m_internalData->shaderPrograms.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeTexture(int32_t* id) {
        if (*id >= 0 && m_internalData->textures.IsAlive(*id)) {
            m_internalData->textures.At(*id).Destroy();
            m_internalData->textures.RemoveByIndex(*id);
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeCmdBuffer(int32_t* id) {
        *id = SR_ID_INVALID;
        return true;
    }

} // namespace SR_GRAPH_NS
