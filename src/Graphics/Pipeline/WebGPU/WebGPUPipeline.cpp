//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Pipeline/WebGPU/WebGPUPipeline.h>
#include <Graphics/Overlay/WebGPUImGuiOverlay.h>
#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/Shader.h>

#include <Utils/Types/ObjectPool.h>
#include <Utils/Types/WeakPtr.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/Vertices.h>
#include <Utils/Common/StoreUtils.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/FileSystem/FileSystem.h>

#include <emscripten/html5.h>

#include <webgpu/webgpu_cpp.h>

namespace SR_GRAPH_NS {
    static void WriteAlignedBuffer(const wgpu::Queue& queue, const wgpu::Buffer& buffer,
        uint64_t offset, const void* data, uint64_t size) {
        if (!data || size == 0 || offset % 4 != 0 || offset > buffer.GetSize() || size > buffer.GetSize() - offset) { return; }
        const uint64_t aligned = (size + 3) & ~uint64_t(3);
        if (aligned > buffer.GetSize() - offset) { return; }
        if (aligned == size) { queue.WriteBuffer(buffer, offset, data, size); }
        else {
            SR_UTILS_NS::Vector<uint8_t> padded;
            padded.resize(aligned);
            std::memset(padded.data(), 0, aligned);
            std::memcpy(padded.data(), data, size);
            queue.WriteBuffer(buffer, offset, padded.data(), aligned);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Internal resource structs
    // ----------------------------------------------------------------------------------------------------------------

    /// Слот сэмплера в group 1. Индекс слота — это порядковый номер сэмплера в шейдере,
    /// он же определяет биндинги: slot * 2 (текстура) и slot * 2 + 1 (сэмплер).
    struct WebGPUSamplerSlot {
        uint32_t binding = 0;        // биндинг из SRSL, с которым приходит BindTexture()
        bool     isAttachment = false;
    };

    /// Слот буфера в group 0.
    struct WebGPUBufferSlot {
        uint32_t binding = 0;
        bool     isStorage = false;
    };

    struct WebGPUShaderProgram {
        wgpu::RenderPipeline    renderPipeline;
        wgpu::ComputePipeline   computePipeline;
        wgpu::BindGroupLayout   bindGroupLayout;   // group 0: UBOs / SSBOs
        wgpu::BindGroupLayout   bindGroupLayout1;  // group 1: textures + samplers (may be null if unused)
        wgpu::BindGroupLayout   bindGroupLayout2;  // group 2: emulated push constants
        wgpu::Buffer            pushConstantBuffer;
        wgpu::BindGroup         pushConstantBindGroup;
        wgpu::PipelineLayout    pipelineLayout;
        SR_UTILS_NS::Vector<WebGPUBufferSlot>  bufferSlots;
        SR_UTILS_NS::Vector<WebGPUSamplerSlot> samplerSlots;
        bool                    isCompute = false;

        void Destroy() {
            renderPipeline  = nullptr;
            computePipeline = nullptr;
            bindGroupLayout  = nullptr;
            bindGroupLayout1 = nullptr;
            bindGroupLayout2 = nullptr;
            pushConstantBuffer = nullptr;
            pushConstantBindGroup = nullptr;
            pipelineLayout  = nullptr;
            bufferSlots.clear();
            samplerSlots.clear();
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
            wgpu::Texture     resolveTexture;
            wgpu::TextureView resolveView;
            SR_UTILS_NS::Vector<wgpu::TextureView> layerViews;
            wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;

            void Destroy() {
                layerViews.clear();
                view = nullptr;
                resolveView = nullptr;
                if (texture) {
                    texture.Destroy();
                    texture = nullptr;
                }
                if (resolveTexture) {
                    resolveTexture.Destroy();
                    resolveTexture = nullptr;
                }
            }
        };

        SR_UTILS_NS::Vector<Attachment> colorAttachments;
        Attachment                      depthAttachment;
        uint32_t                        width  = 0;
        uint32_t                        height = 0;
        uint8_t                         sampleCount = 1;

        void Destroy() {
            for (auto& att : colorAttachments) { att.Destroy(); }
            colorAttachments.clear();
            depthAttachment.Destroy();
        }
    };

    /// Накопленное состояние набора дескрипторов. BindGroup в WebGPU неизменяемый, а движок
    /// обновляет биндинги по одному (и только когда они меняются), поэтому состояние накапливается
    /// здесь, а сами BindGroup'ы пересоздаются лениво, перед отрисовкой, по layout'у шейдера.
    struct WebGPUBindGroup {
        struct BufferBinding {
            uint32_t binding = 0;
            DescriptorType type = DescriptorType::Unknown;
            int32_t resourceId = SR_ID_INVALID;
        };

        struct TextureBinding {
            uint32_t binding = 0;
            int32_t textureId = SR_ID_INVALID;
            bool isAttachment = false;
        };

        SR_UTILS_NS::Vector<BufferBinding>  buffers;   // group 0
        SR_UTILS_NS::Vector<TextureBinding> textures;  // group 1

        wgpu::BindGroup bufferBindGroup;
        wgpu::BindGroup textureBindGroup;

        int32_t buffersBuiltForShader  = SR_ID_INVALID;
        int32_t texturesBuiltForShader = SR_ID_INVALID;

        bool isBuffersDirty  = true;
        bool isTexturesDirty = true;

        void Destroy() {
            bufferBindGroup  = nullptr;
            textureBindGroup = nullptr;
            buffers.clear();
            textures.clear();
            buffersBuiltForShader  = SR_ID_INVALID;
            texturesBuiltForShader = SR_ID_INVALID;
            isBuffersDirty  = true;
            isTexturesDirty = true;
        }
    };

    // ----------------------------------------------------------------------------------------------------------------
    // Internal data aggregate
    // ----------------------------------------------------------------------------------------------------------------

    struct WebGPUPipelineInternalData {
        bool failed = false;
        bool destroyed = false;
        bool surfaceNeedsConfigure = false;
        wgpu::Limits limits;
        struct PixelReadback {
            uint64_t id = SR_ID_INVALID;
            wgpu::Buffer buffer;
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t bytesPerRow = 0;
            bool mapPending = false;
            bool ready = false;
            bool failed = false;
            void* mapped = nullptr;
            int32_t ssboId = SR_ID_INVALID;
            bool copyPending = false;
            wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;
        };
        struct HostBuffer {
            int32_t id = SR_ID_INVALID;
            SR_UTILS_NS::Vector<uint8_t> data;
            bool mapped = false;
        };
        SR_UTILS_NS::Vector<HostBuffer> hostBuffers;

        wgpu::Device  device;
        wgpu::Queue   queue;
        wgpu::Instance instance;
        wgpu::Surface surface;
        wgpu::CommandBuffer  surfaceCommandBuffer;
        wgpu::CommandEncoder activeEncoder;

        wgpu::Texture     surfaceTexture;
        wgpu::Texture     surfaceColorTexture;
        uint8_t surfaceAcquireFailures = 0;
        wgpu::TextureView surfaceTextureView;
        wgpu::Texture     surfaceDepthTexture;
        wgpu::TextureView surfaceDepthTextureView;
        wgpu::Texture     surfaceMultisampleTexture;
        wgpu::TextureView surfaceMultisampleTextureView;
        wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;
        uint32_t surfaceWidth  = 0;
        uint32_t surfaceHeight = 0;
        uint32_t surfaceDepthWidth  = 0;
        uint32_t surfaceDepthHeight = 0;
        uint8_t surfaceSampleCount = 1;

        // ---- Per-render-pass state ----
        // Kept alive for the duration of BeginRender..EndRender
        wgpu::RenderPassEncoder activeRenderPass;
        // Kept alive for the duration of BeginCompute..EndCompute
        wgpu::ComputePassEncoder activeComputePass;
        // Clear values set by ClearBuffers before BeginRender
        wgpu::Color        clearColor  = { 0.0, 0.0, 0.0, 1.0 };
        float              clearDepth  = 1.0f;
        bool               hasClearColor = false;
        bool               hasClearDepth = false;
        SR_UTILS_NS::Vector<wgpu::Color> clearColors;
        // Tracks whether BeginRender was called for the swapchain (no FBO) this frame.
        // Used in DrawFrame to decide loadOp for the ImGui pass.
        bool               surfaceWasRendered = false;
        SR_MATH_NS::IRect  scissor;
        SR_MATH_NS::IRect  activeScissor;
        bool               activeScissorValid = false;

        // Descriptor set (group 0) selected by BindDescriptorSet. Bound at draw time, because the
        // engine keeps updating its bindings after BindDescriptorSet was called.
        int32_t activeBindGroupId = SR_ID_INVALID;
        std::array<int32_t, SR_MAX_VERTEX_INPUT_BINDINGS> activeVBOs;
        SR_HTYPES_NS::FlatHashMap<int32_t, uint32_t> indexSizes;
        uint32_t viewportWidth = 0;
        uint32_t viewportHeight = 0;

        // Attachment layout entries are declared as non-filtering, so they can't be bound
        // with the linear sampler that comes with the attachment texture.
        wgpu::Sampler nonFilteringSampler;

        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         VBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         IBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         UBOs;
        SR_HTYPES_NS::ObjectPool<wgpu::Buffer>         SSBOs;
        SR_HTYPES_NS::ObjectPool<WebGPUShaderProgram>  shaderPrograms;
        SR_HTYPES_NS::ObjectPool<WebGPUTexture>        textures;
        SR_HTYPES_NS::ObjectPool<WebGPUFrameBuffer>    frameBuffers;
        SR_HTYPES_NS::ObjectPool<WebGPUBindGroup>      bindGroups;
        SR_UTILS_NS::Vector<PixelReadback>              pixelReadbacks;
        uint64_t                                       nextPixelReadbackId = 1;
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
            case SR_GRAPH_NS::ImageFormat::RGBA16_UNORM:          return wgpu::TextureFormat::RGBA16Float;
            case SR_GRAPH_NS::ImageFormat::RGBA16_SFLOAT:         return wgpu::TextureFormat::RGBA16Float;
            case SR_GRAPH_NS::ImageFormat::R8_UNORM:              return wgpu::TextureFormat::R8Unorm;
            case SR_GRAPH_NS::ImageFormat::R16_UNORM:             return wgpu::TextureFormat::R16Float;
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
            case SR_GRAPH_NS::ImageFormat::RGB8_UNORM:           return wgpu::TextureFormat::RGBA8Unorm;
            case SR_GRAPH_NS::ImageFormat::RGB8_SRGB:            return wgpu::TextureFormat::RGBA8UnormSrgb;
            case SR_GRAPH_NS::ImageFormat::RGB16_UNORM:          return wgpu::TextureFormat::RGBA16Float;
            default:
                SR_ERROR("Unsupported WebGPU image format {}", static_cast<int32_t>(format));
                return wgpu::TextureFormat::Undefined;
        }
    }

    static uint32_t TexelBytes(wgpu::TextureFormat format) {
        switch (format) {
            case wgpu::TextureFormat::R8Unorm: case wgpu::TextureFormat::R8Uint: return 1;
            case wgpu::TextureFormat::RG8Unorm: case wgpu::TextureFormat::R16Uint:
            case wgpu::TextureFormat::R16Float: case wgpu::TextureFormat::Depth16Unorm: return 2;
            case wgpu::TextureFormat::RGBA16Float: return 8;
            case wgpu::TextureFormat::RGBA8Unorm: case wgpu::TextureFormat::RGBA8UnormSrgb:
            case wgpu::TextureFormat::BGRA8Unorm: case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::R32Float: case wgpu::TextureFormat::R32Uint:
            case wgpu::TextureFormat::Depth32Float: return 4;
            default: return 0;
        }
    }

    static uint16_t FloatToHalf(float_t value) {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        const uint32_t sign = (bits >> 16) & 0x8000;
        const int32_t exponent = static_cast<int32_t>((bits >> 23) & 255) - 127 + 15;
        const uint32_t mantissa = bits & 0x7fffff;
        if (exponent >= 31) { return static_cast<uint16_t>(sign | 0x7c00 | (((bits & 0x7f800000) == 0x7f800000 && mantissa) ? 0x200 : 0)); }
        if (exponent <= 0) {
            if (exponent < -10) { return static_cast<uint16_t>(sign); }
            return static_cast<uint16_t>(sign | (((mantissa | 0x800000) + (1u << (12 - exponent))) >> (14 - exponent)));
        }
        return static_cast<uint16_t>(sign | ((static_cast<uint32_t>(exponent) << 10) + ((mantissa + 0x1000) >> 13)));
    }

    static float_t HalfToFloat(uint16_t value) {
        const uint32_t exponent = (value >> 10) & 31;
        const uint32_t mantissa = value & 1023;
        float_t result = exponent == 0 ? std::ldexp(static_cast<float_t>(mantissa), -24)
            : exponent == 31 ? (mantissa ? std::numeric_limits<float_t>::quiet_NaN() : std::numeric_limits<float_t>::infinity())
            : std::ldexp(1.f + static_cast<float_t>(mantissa) / 1024.f, static_cast<int32_t>(exponent) - 15);
        return value & 0x8000 ? -result : result;
    }

    static bool ConvertTextureUpload(const SRTextureCreateInfo& info, wgpu::TextureFormat format,
        SR_UTILS_NS::Vector<uint8_t>& converted, const uint8_t*& data, uint64_t& dataSize) {
        data = info.pData;
        dataSize = info.imageSize;
        if (!data) { return true; }
        const uint64_t pixels = static_cast<uint64_t>(info.width) * info.height;
        uint32_t sourceBytes = TexelBytes(format);
        const bool rgb8 = info.format == ImageFormat::RGB8_UNORM || info.format == ImageFormat::RGB8_SRGB;
        const bool normalized16 = info.format == ImageFormat::R16_UNORM || info.format == ImageFormat::RGB16_UNORM || info.format == ImageFormat::RGBA16_UNORM;
        const bool packed = info.format == ImageFormat::B10G11R11_UFLOAT_PACK32;
        if (rgb8) { sourceBytes = 3; }
        if (info.format == ImageFormat::RGB16_UNORM) { sourceBytes = 6; }
        if (packed) { sourceBytes = 4; }
        if (!sourceBytes || dataSize < pixels * sourceBytes) { return false; }
        if (!rgb8 && !normalized16 && !packed) { return true; }
        const uint32_t targetBytes = TexelBytes(format);
        converted.resize(pixels * targetBytes);
        for (uint64_t pixel = 0; pixel < pixels; ++pixel) {
            const auto* src = data + pixel * sourceBytes;
            auto* dst = converted.data() + pixel * targetBytes;
            if (rgb8) {
                std::memcpy(dst, src, 3); dst[3] = 255;
                continue;
            }
            uint32_t packedValue = 0;
            if (packed) { std::memcpy(&packedValue, src, 4); }
            for (uint32_t channel = 0; channel < targetBytes / 2; ++channel) {
                float_t value = 1.f;
                if (normalized16 && channel < sourceBytes / 2) {
                    uint16_t integer; std::memcpy(&integer, src + channel * 2, 2);
                    value = static_cast<float_t>(integer) / 65535.f;
                }
                if (packed && channel < 3) {
                    const uint32_t mantissaBits = channel == 2 ? 5 : 6;
                    const uint32_t component = (packedValue >> (channel == 0 ? 0 : channel == 1 ? 11 : 22)) & ((1u << (mantissaBits + 5)) - 1);
                    value = HalfToFloat(static_cast<uint16_t>(component << (10 - mantissaBits)));
                }
                const uint16_t half = FloatToHalf(value);
                std::memcpy(dst + channel * 2, &half, 2);
            }
        }
        data = converted.data(); dataSize = converted.size();
        return true;
    }

    /// Не для всех форматов WebGPU разрешает блендинг: 32-битные float и целочисленные
    /// форматы не являются blendable, и попытка включить для них блендинг ломает пайплайн
    /// (Validation error: "Blending is enabled but color format ... is not blendable").
    static bool IsBlendableFormat(wgpu::TextureFormat fmt) {
        switch (fmt) {
            case wgpu::TextureFormat::R8Unorm:
            case wgpu::TextureFormat::R8Snorm:
            case wgpu::TextureFormat::R16Float:
            case wgpu::TextureFormat::RG8Unorm:
            case wgpu::TextureFormat::RG8Snorm:
            case wgpu::TextureFormat::RG16Float:
            case wgpu::TextureFormat::RGBA8Unorm:
            case wgpu::TextureFormat::RGBA8UnormSrgb:
            case wgpu::TextureFormat::RGBA8Snorm:
            case wgpu::TextureFormat::BGRA8Unorm:
            case wgpu::TextureFormat::BGRA8UnormSrgb:
            case wgpu::TextureFormat::RGB10A2Unorm:
            case wgpu::TextureFormat::RG11B10Ufloat:
            case wgpu::TextureFormat::RGBA16Float:
                return true;
            default:
                /// R32Float / RG32Float / RGBA32Float и все *Uint / *Sint форматы
                return false;
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

    /// Аттачменты объявляются в layout'е как non-filtering, поэтому их нельзя привязать
    /// с линейным сэмплером, который создается вместе с текстурой аттачмента.
    static const wgpu::Sampler& GetNonFilteringSampler(WebGPUPipelineInternalData* pInternalData) {
        if (!pInternalData->nonFilteringSampler) SR_UNLIKELY_ATTRIBUTE {
            wgpu::SamplerDescriptor samplerDesc{};
            samplerDesc.addressModeU  = wgpu::AddressMode::ClampToEdge;
            samplerDesc.addressModeV  = wgpu::AddressMode::ClampToEdge;
            samplerDesc.addressModeW  = wgpu::AddressMode::ClampToEdge;
            samplerDesc.magFilter     = wgpu::FilterMode::Nearest;
            samplerDesc.minFilter     = wgpu::FilterMode::Nearest;
            samplerDesc.mipmapFilter  = wgpu::MipmapFilterMode::Nearest;
            samplerDesc.maxAnisotropy = 1;

            pInternalData->nonFilteringSampler = pInternalData->device.CreateSampler(&samplerDesc);
        }

        return pInternalData->nonFilteringSampler;
    }

    static wgpu::TextureViewDimension SamplerDimensionToWGPU(SR_GRAPH_NS::SamplerDimension dimension) {
        switch (dimension) {
            case SR_GRAPH_NS::SamplerDimension::Texture3D:      return wgpu::TextureViewDimension::e3D;
            case SR_GRAPH_NS::SamplerDimension::TextureCube:    return wgpu::TextureViewDimension::Cube;
            case SR_GRAPH_NS::SamplerDimension::Texture2DArray: return wgpu::TextureViewDimension::e2DArray;
            case SR_GRAPH_NS::SamplerDimension::Texture2DArrayDepth: return wgpu::TextureViewDimension::e2DArray;
            default:                                            return wgpu::TextureViewDimension::e2D;
        }
    }

    static wgpu::ShaderStage ShaderStageToWGPU(SR_GRAPH_NS::ShaderStage stage) {
        switch (stage) {
            case SR_GRAPH_NS::ShaderStage::Vertex:   return wgpu::ShaderStage::Vertex;
            case SR_GRAPH_NS::ShaderStage::Fragment: return wgpu::ShaderStage::Fragment;
            case SR_GRAPH_NS::ShaderStage::Compute:  return wgpu::ShaderStage::Compute;
            default: return wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
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
        m_internalData->activeVBOs.fill(SR_ID_INVALID);
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

        // ---- ImGui pass into swapchain ----
        // All scene geometry has already been rendered into FBOs via BeginRender/EndRender.
        // Here we open a final swapchain pass to render the ImGui UI on top.
        // If nothing was rendered to the swapchain by SwapchainPass, we still clear it
        // so it is not undefined/transparent.
    #ifdef SR_USE_IMGUI
        if (auto&& pOverlayBase = GetOverlay(OverlayType::ImGui)) {
            if (pOverlayBase->IsEnabled() && !pOverlayBase->IsSurfaceDirty()) {
                if (auto&& pImGuiOverlay = SR_UTILS_NS::DynamicPointerCast<WebGPUImGuiOverlay>(pOverlayBase)) {
                    wgpu::RenderPassColorAttachment color{};
                    color.view       = m_internalData->surfaceTextureView;
                    // Use Load to preserve anything written by a SwapchainPass via BeginRender/EndRender.
                    // The surface starts as undefined each frame; if no SwapchainPass ran, it will appear
                    // as black/transparent. That is acceptable — the ImGui layer renders the editor UI.
                    // If SwapchainPass already rendered to surface this frame → preserve it (Load).
                    // Otherwise clear to a neutral dark color so ImGui has a clean background.
                    color.loadOp     = m_internalData->surfaceWasRendered ? wgpu::LoadOp::Load : wgpu::LoadOp::Clear;
                    color.storeOp    = wgpu::StoreOp::Store;
                    color.clearValue = { 0.1, 0.1, 0.12, 1.0 };

                    wgpu::RenderPassDescriptor passDesc{};
                    passDesc.colorAttachmentCount = 1;
                    passDesc.colorAttachments     = &color;

                    auto pass = m_internalData->activeEncoder.BeginRenderPass(&passDesc);
                    pImGuiOverlay->Render(pass.Get());
                    pass.End();
                    m_internalData->surfaceWasRendered = true;
                }
            }
        }
    #endif

        for (auto& request : m_internalData->pixelReadbacks) {
            if (request.copyPending && m_internalData->SSBOs.IsAlive(request.ssboId)) {
                m_internalData->activeEncoder.CopyBufferToBuffer(m_internalData->SSBOs.At(request.ssboId), 0,
                    request.buffer, 0, request.bytesPerRow);
                request.copyPending = false;
            }
        }
        if (!m_internalData->surfaceWasRendered) {
            wgpu::RenderPassColorAttachment color{};
            color.view = m_internalData->surfaceTextureView;
            color.loadOp = wgpu::LoadOp::Clear; color.storeOp = wgpu::StoreOp::Store;
            color.clearValue = {0.1, 0.1, 0.12, 1.0};
            wgpu::RenderPassDescriptor descriptor{};
            descriptor.colorAttachmentCount = 1; descriptor.colorAttachments = &color;
            auto pass = m_internalData->activeEncoder.BeginRenderPass(&descriptor);
            pass.End();
        }
        wgpu::SurfaceTexture presentation;
        m_internalData->surface.GetCurrentTexture(&presentation);
        const bool present = presentation.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal
            || presentation.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal;
        if (present) {
            m_internalData->surfaceAcquireFailures = 0;
            m_internalData->surfaceNeedsConfigure |= presentation.status == wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal;
            wgpu::TexelCopyTextureInfo src{}; src.texture = m_internalData->surfaceTexture;
            wgpu::TexelCopyTextureInfo dst{}; dst.texture = presentation.texture;
            const wgpu::Extent3D extent{m_internalData->surfaceWidth, m_internalData->surfaceHeight, 1};
            m_internalData->activeEncoder.CopyTextureToTexture(&src, &dst, &extent);
        }
        else if (presentation.status != wgpu::SurfaceGetCurrentTextureStatus::Timeout) {
            m_internalData->surfaceNeedsConfigure = true;
            if (++m_internalData->surfaceAcquireFailures >= 3) {
                m_internalData->failed = true;
                EM_ASM({
                    globalThis.srEngineFatalError = 'The browser graphics surface could not be restored. Reload the page.';
                    globalThis.dispatchEvent(new CustomEvent('srengine-error', {detail: globalThis.srEngineFatalError}));
                });
            }
        }
        m_internalData->surfaceCommandBuffer = m_internalData->activeEncoder.Finish();
        m_internalData->surfaceCommandBuffer.SetLabel("Surface command buffer");

        if (m_internalData->surfaceCommandBuffer) {
            m_internalData->queue.Submit(1, &m_internalData->surfaceCommandBuffer);
            if (present) EM_ASM({
                Module['srSubmittedFrames'] = (Module['srSubmittedFrames'] || 0) + 1;
                if (Module['srSubmittedFrames'] === 1) globalThis.dispatchEvent(new CustomEvent('srengine-running'));
            });
        }

        StartReadbackMappings();

        // Presentation is driven by the browser main loop.
        m_internalData->surfaceCommandBuffer = nullptr;
        m_internalData->activeEncoder = nullptr;
        m_internalData->surfaceTextureView = nullptr;
        m_internalData->surfaceTexture = nullptr;
    }

    void WebGPUPipeline::StartReadbackMappings() {
        for (auto& request : m_internalData->pixelReadbacks) {
            if (request.copyPending || request.mapPending || request.ready || request.failed) {
                continue;
            }

            const uint64_t requestId = request.id;
            const auto weak = GetWeakThis<WebGPUPipeline>();
            request.mapPending = true;
            request.buffer.MapAsync(
                wgpu::MapMode::Read,
                0,
                request.height * request.bytesPerRow,
                wgpu::CallbackMode::AllowProcessEvents,
                [weak, requestId](wgpu::MapAsyncStatus status, wgpu::StringView message) {
                    const auto self = weak.Lock();
                    if (!self || self->m_internalData->destroyed) { return; }
                    auto* const pipelineData = self->m_internalData;
                    for (auto& readback : pipelineData->pixelReadbacks) {
                        if (readback.id == requestId) {
                            readback.mapPending = false;
                            readback.ready = true;
                            readback.failed = status != wgpu::MapAsyncStatus::Success;
                            if (readback.failed) {
                                SR_ERROR("WebGPUPipeline::RequestPixelRange() : MapAsync failed for request {}: {}",
                                    requestId,
                                    message.length != WGPU_STRLEN
                                        ? SR_UTILS_NS::StringView(message.data, message.length)
                                        : SR_UTILS_NS::StringView(message.data ? message.data : "(unknown error)"));
                            }
                            break;
                        }
                    }
                }
            );
        }

    }

    // ----------------------------------------------------------------------------------------------------------------
    // Frame begin/end
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::OnFrameBuildBegin() {
        SR_TRACY_ZONE;

        Super::OnFrameBuildBegin();

        m_internalData->instance.ProcessEvents();

        if (m_internalData->failed || m_internalData->destroyed || !m_internalData->device || !m_internalData->queue || !m_internalData->surface) {
            return;
        }

        // Resize-aware surface configuration.
        int32_t cssW = 1, cssH = 1;
        if (emscripten_get_canvas_element_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
            const uint32_t kMaxSurfaceDim = m_internalData->limits.maxTextureDimension2D;
            const uint32_t pxW = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1, cssW)));
            const uint32_t pxH = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1, cssH)));

            if (m_internalData->surfaceNeedsConfigure || pxW != m_internalData->surfaceWidth || pxH != m_internalData->surfaceHeight) {
                m_internalData->surfaceNeedsConfigure = false;
                m_internalData->surfaceWidth  = pxW;
                m_internalData->surfaceHeight = pxH;

                wgpu::SurfaceConfiguration config{};
                config.device      = m_internalData->device;
                config.format      = m_internalData->surfaceFormat != wgpu::TextureFormat::Undefined ? m_internalData->surfaceFormat : wgpu::TextureFormat::BGRA8Unorm;
                config.usage       = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
                config.width       = pxW;
                config.height      = pxH;
                config.presentMode = wgpu::PresentMode::Fifo;

                m_internalData->surface.Configure(&config);
                SetOverlaySurfaceDirty();
                ReCreateOverlay();
            }
        }

        if (!m_internalData->surfaceColorTexture
            || m_internalData->surfaceColorTexture.GetWidth() != m_internalData->surfaceWidth
            || m_internalData->surfaceColorTexture.GetHeight() != m_internalData->surfaceHeight) {
            wgpu::TextureDescriptor descriptor{};
            descriptor.size = {m_internalData->surfaceWidth, m_internalData->surfaceHeight, 1};
            descriptor.format = m_internalData->surfaceFormat;
            descriptor.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
            m_internalData->surfaceColorTexture = m_internalData->device.CreateTexture(&descriptor);
        }
        m_internalData->surfaceTexture = m_internalData->surfaceColorTexture;
        m_internalData->surfaceTextureView = m_internalData->surfaceTexture.CreateView();

        const uint8_t surfaceSampleCount = IsMultiSamplingSupported() ? GetSamplesCount() : 1;
        if (!m_internalData->surfaceDepthTexture ||
            m_internalData->surfaceDepthWidth != m_internalData->surfaceWidth ||
            m_internalData->surfaceDepthHeight != m_internalData->surfaceHeight ||
            m_internalData->surfaceSampleCount != surfaceSampleCount)
        {
            m_internalData->surfaceDepthTextureView = nullptr;
            m_internalData->surfaceMultisampleTextureView = nullptr;
            if (m_internalData->surfaceDepthTexture) {
                m_internalData->surfaceDepthTexture.Destroy();
                m_internalData->surfaceDepthTexture = nullptr;
            }
            if (m_internalData->surfaceMultisampleTexture) {
                m_internalData->surfaceMultisampleTexture.Destroy();
                m_internalData->surfaceMultisampleTexture = nullptr;
            }

            wgpu::TextureDescriptor depthDesc{};
            depthDesc.size          = { m_internalData->surfaceWidth, m_internalData->surfaceHeight, 1 };
            depthDesc.format        = wgpu::TextureFormat::Depth32Float;
            depthDesc.usage         = wgpu::TextureUsage::RenderAttachment;
            depthDesc.mipLevelCount = 1;
            depthDesc.sampleCount   = surfaceSampleCount;
            depthDesc.dimension     = wgpu::TextureDimension::e2D;

            m_internalData->surfaceDepthTexture = m_internalData->device.CreateTexture(&depthDesc);
            m_internalData->surfaceDepthTextureView = m_internalData->surfaceDepthTexture.CreateView();
            m_internalData->surfaceDepthWidth = m_internalData->surfaceWidth;
            m_internalData->surfaceDepthHeight = m_internalData->surfaceHeight;
            m_internalData->surfaceSampleCount = surfaceSampleCount;

            if (surfaceSampleCount > 1) {
                wgpu::TextureDescriptor multisampleDesc{};
                multisampleDesc.size = { m_internalData->surfaceWidth, m_internalData->surfaceHeight, 1 };
                multisampleDesc.format = m_internalData->surfaceFormat;
                multisampleDesc.usage = wgpu::TextureUsage::RenderAttachment;
                multisampleDesc.mipLevelCount = 1;
                multisampleDesc.sampleCount = surfaceSampleCount;
                multisampleDesc.dimension = wgpu::TextureDimension::e2D;
                m_internalData->surfaceMultisampleTexture = m_internalData->device.CreateTexture(&multisampleDesc);
                m_internalData->surfaceMultisampleTextureView = m_internalData->surfaceMultisampleTexture.CreateView();
            }
        }

        m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder();
        m_internalData->activeEncoder.SetLabel("Surface command encoder");
        m_internalData->surfaceWasRendered = false;
    }

    void WebGPUPipeline::OnFrameBuildEnd() {
        SR_TRACY_ZONE;
        Super::OnFrameBuildEnd();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Initialization
    // ----------------------------------------------------------------------------------------------------------------

    bool WebGPUPipeline::PreInit(const PipelinePreInitInfo& info) {
        m_requestedSampleCount = info.samplesCount;
        if (!Super::PreInit(info)) {
            return false;
        }

        SR_INFO("WebGPUPipeline::PreInit() : requesting WebGPU adapter and device...");

        if (!EM_ASM_INT({ return !!(globalThis.navigator && navigator.gpu); })) {
            SR_ERROR("WebGPU is unavailable. Use a browser with WebGPU support in a secure context.");
            m_internalData->failed = true;
            return false;
        }

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

        const auto weak = GetWeakThis<WebGPUPipeline>();
        m_internalData->instance.RequestAdapter(&options, wgpu::CallbackMode::AllowProcessEvents, [weak](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
            const auto self = weak.Lock();
            if (!self || self->m_internalData->destroyed) { return; }
            auto* const pData = self->m_internalData;
            if (status != wgpu::RequestAdapterStatus::Success) {
                const SR_UTILS_NS::StringView msgView =
                    (message.length != WGPU_STRLEN)
                        ? SR_UTILS_NS::StringView(message.data, message.length)
                        : SR_UTILS_NS::StringView(message.data ? message.data : "(unknown error)");
                SR_ERROR("WebGPUPipeline::PreInit() : failed to request adapter: {}", msgView);
                pData->failed = true;
                return;
            }

            SR_INFO("WebGPUPipeline::PreInit() : WebGPU adapter successfully obtained.");
            wgpu::SurfaceCapabilities capabilities;
            if (!pData->surface.GetCapabilities(adapter, &capabilities) || capabilities.formatCount == 0) {
                pData->failed = true;
                SR_ERROR("WebGPU surface has no supported formats.");
                return;
            }
            pData->surfaceFormat = capabilities.formats[0];
            for (size_t i = 0; i < capabilities.formatCount; ++i) {
                if (capabilities.formats[i] == wgpu::TextureFormat::BGRA8Unorm) {
                    pData->surfaceFormat = capabilities.formats[i];
                    break;
                }
            }

            // ---- DeviceDescriptor with uncaptured-error callback ----
            // SetUncapturedErrorCallback lives on DeviceDescriptor (not on wgpu::Device).
            // It must be configured before RequestDevice is called.
            wgpu::DeviceDescriptor deviceDesc{};
            wgpu::Limits requiredLimits;
            if (!adapter.GetLimits(&requiredLimits)) {
                pData->failed = true;
                SR_ERROR("Failed to obtain WebGPU adapter limits.");
                return;
            }
            deviceDesc.requiredLimits = &requiredLimits;
            std::array<wgpu::FeatureName, 2> optionalFeatures = {
                wgpu::FeatureName::Depth32FloatStencil8, wgpu::FeatureName::Float32Filterable};
            SR_UTILS_NS::Vector<wgpu::FeatureName> features;
            for (const auto feature : optionalFeatures) {
                if (adapter.HasFeature(feature)) { features.emplace_back(feature); }
            }
            deviceDesc.requiredFeatureCount = features.size();
            deviceDesc.requiredFeatures = features.data();
            deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowProcessEvents,
                [weak](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
                    const auto self = weak.Lock();
                    if (!self || self->m_internalData->destroyed || reason == wgpu::DeviceLostReason::Destroyed) { return; }
                    self->m_internalData->failed = true;
                    SR_ERROR("WebGPU device lost: {}", message.length == WGPU_STRLEN
                        ? SR_UTILS_NS::StringView(message.data ? message.data : "")
                        : SR_UTILS_NS::StringView(message.data, message.length));
                    EM_ASM({
                        globalThis.srEngineFatalError = 'The graphics device was lost. Reload the page to restart.';
                        globalThis.dispatchEvent(new CustomEvent('srengine-error', {detail: globalThis.srEngineFatalError}));
                    });
                });
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
                            ? SR_UTILS_NS::StringView(message.data, message.length)
                            : SR_UTILS_NS::StringView(message.data ? message.data : "(null)"));
                }
            );

            adapter.RequestDevice(&deviceDesc, wgpu::CallbackMode::AllowProcessEvents, [weak](wgpu::RequestDeviceStatus status, wgpu::Device dev, wgpu::StringView message) {
                const auto self = weak.Lock();
                if (!self || self->m_internalData->destroyed) { return; }
                auto* const m_internalData = self->m_internalData;
                if (status != wgpu::RequestDeviceStatus::Success) {
                    SR_ERROR("WebGPUPipeline::PreInit() : failed to request device: {}",
                        message.length != WGPU_STRLEN
                            ? SR_UTILS_NS::StringView(message.data, message.length)
                            : SR_UTILS_NS::StringView(message.data ? message.data : "(unknown error)"));
                    m_internalData->failed = true;
                    return;
                }

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU device successfully obtained.");

                m_internalData->device = dev;
                m_internalData->queue  = m_internalData->device.GetQueue();
                if (!dev.GetLimits(&m_internalData->limits)) {
                    m_internalData->failed = true;
                    SR_ERROR("Failed to obtain WebGPU device limits.");
                    return;
                }
                EM_ASM({ Module['srMaxTextureDimension'] = $0; }, m_internalData->limits.maxTextureDimension2D);

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU device and queue successfully created.");

                wgpu::SurfaceConfiguration config{};
                config.device      = m_internalData->device;
                config.format      = m_internalData->surfaceFormat;
                config.usage       = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
                int32_t cssW = 1, cssH = 1;
                if (emscripten_get_canvas_element_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
                    const uint32_t kMaxSurfaceDim = m_internalData->limits.maxTextureDimension2D;
                    config.width  = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1, cssW)));
                    config.height = std::min<uint32_t>(kMaxSurfaceDim, static_cast<uint32_t>(std::max(1, cssH)));
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

                self->m_sampleProbePending = 6;
                for (uint16_t sampleCount = 2; sampleCount <= 64; sampleCount *= 2) {
                    self->ProbeSupportedSampleCount(static_cast<uint8_t>(sampleCount));
                }

                SR_INFO("WebGPUPipeline::PreInit() : WebGPU surface configured successfully.");
            });
        });

        SR_INFO("WebGPUPipeline::PreInit() : waiting for WebGPU initialization to complete...");

        return true;
    }

    bool WebGPUPipeline::IsAsyncEarlyInit() const {
        if (m_internalData->instance) { m_internalData->instance.ProcessEvents(); }
        if (m_internalData->failed) {
            EM_ASM({
                globalThis.srEngineFatalError = globalThis.srEngineFatalError || 'WebGPU initialization failed. See the console for details.';
                globalThis.dispatchEvent(new CustomEvent('srengine-error', {detail: globalThis.srEngineFatalError}));
            });
        }
        return !m_internalData->failed && (!m_internalData->device || !m_internalData->queue);
    }

    bool WebGPUPipeline::Init() {
        return !m_internalData->failed && m_internalData->device && m_internalData->queue;
    }

    bool WebGPUPipeline::PostInit() {
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // FrameBuffer binding & render pass management
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::BindFrameBuffer(Pipeline::FramebufferPtr pFBO) {
        Super::BindFrameBuffer(pFBO);
        // m_state.pFrameBuffer is set by Super.
        // Also set frameBufferId so GetCurrentFrameBufferId() returns the correct value
        // (used by ShaderProgramManager to key per-FBO shader variants).
        m_state.frameBufferId = pFBO ? pFBO->GetId() : 0;
    }

    bool WebGPUPipeline::BeginRender() {
        if (!Super::BeginRender()) {
            return false;
        }

        if (!m_internalData->activeEncoder) {
            SR_ERROR("WebGPUPipeline::BeginRender() : no active command encoder! Call OnFrameBuildBegin first.");
            return false;
        }

        auto&& pFBO = m_state.pFrameBuffer;
        bool keepClearFlags = false;

        if (pFBO) {
            // Render into the FBO's color + depth attachments
            const int32_t fboId = pFBO->GetId();
            if (fboId < 0 || !m_internalData->frameBuffers.IsAlive(fboId)) {
                SR_ERROR("WebGPUPipeline::BeginRender() : FBO id {} is invalid or not alive!", fboId);
                return false;
            }

            auto&& fbo = m_internalData->frameBuffers.At(fboId);
            keepClearFlags = m_state.frameBufferLayer + 1 < fbo.depthAttachment.layerViews.size();
            if (fbo.colorAttachments.empty() || !fbo.colorAttachments[0].view) {
                SR_ERROR("WebGPUPipeline::BeginRender() : FBO {} has no valid color attachment!", fboId);
                return false;
            }

            SR_UTILS_NS::Vector<wgpu::RenderPassColorAttachment> colorAttachments;
            colorAttachments.reserve(fbo.colorAttachments.size());

            for (auto&& att : fbo.colorAttachments) {
                const auto attachmentIndex = colorAttachments.size();
                wgpu::RenderPassColorAttachment colorAtt{};
                colorAtt.view     = att.view;
                colorAtt.resolveTarget = att.resolveView;
                const bool clear = m_internalData->hasClearColor
                    && (m_internalData->clearColors.empty() || attachmentIndex < m_internalData->clearColors.size());
                colorAtt.loadOp   = clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
                colorAtt.storeOp  = wgpu::StoreOp::Store;
                colorAtt.clearValue = clear
                    ? (m_internalData->clearColors.empty() ? m_internalData->clearColor : m_internalData->clearColors[attachmentIndex])
                    : wgpu::Color{ 0.0, 0.0, 0.0, 1.0 };

                colorAttachments.push_back(colorAtt);
            }

            wgpu::RenderPassDescriptor passDesc{};
            passDesc.colorAttachmentCount = static_cast<size_t>(colorAttachments.size());
            passDesc.colorAttachments     = colorAttachments.data();

            wgpu::RenderPassDepthStencilAttachment depthAtt{};
            if (fbo.depthAttachment.view) {
                const bool hasStencil = fbo.depthAttachment.format == wgpu::TextureFormat::Depth24PlusStencil8
                                     || fbo.depthAttachment.format == wgpu::TextureFormat::Depth32FloatStencil8;

                if (fbo.depthAttachment.layerViews.empty()) {
                    depthAtt.view = fbo.depthAttachment.view;
                }
                else {
                    const uint32_t layer = SR_MIN(m_state.frameBufferLayer,
                        static_cast<uint32_t>(fbo.depthAttachment.layerViews.size() - 1));
                    depthAtt.view = fbo.depthAttachment.layerViews[layer];
                }
                depthAtt.depthLoadOp       = m_internalData->hasClearDepth ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
                depthAtt.depthStoreOp      = wgpu::StoreOp::Store;
                depthAtt.depthClearValue   = m_internalData->hasClearDepth ? m_internalData->clearDepth : 1.0f;
                // Stencil ops must be Undefined for depth-only formats (no stencil component).
                depthAtt.stencilLoadOp     = hasStencil ? (m_internalData->hasClearDepth ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load) : wgpu::LoadOp::Undefined;
                depthAtt.stencilStoreOp    = hasStencil ? wgpu::StoreOp::Store : wgpu::StoreOp::Undefined;
                depthAtt.stencilClearValue = 0;
                passDesc.depthStencilAttachment = &depthAtt;
            }

            m_internalData->activeRenderPass = m_internalData->activeEncoder.BeginRenderPass(&passDesc);
        }
        else {
            // Render directly into swapchain (SwapchainPass)
            if (!m_internalData->surfaceTextureView) {
                SR_ERROR("WebGPUPipeline::BeginRender() : no surface texture view for swapchain pass!");
                return false;
            }

            wgpu::RenderPassColorAttachment color{};
            color.view       = m_internalData->surfaceSampleCount > 1
                ? m_internalData->surfaceMultisampleTextureView
                : m_internalData->surfaceTextureView;
            color.resolveTarget = m_internalData->surfaceSampleCount > 1
                ? m_internalData->surfaceTextureView
                : nullptr;
            // Always clear on first swapchain render pass of the frame to avoid undefined content.
            color.loadOp     = !m_internalData->surfaceWasRendered || m_internalData->hasClearColor
                ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
            color.storeOp    = wgpu::StoreOp::Store;
            color.clearValue = m_internalData->hasClearColor
                ? m_internalData->clearColor
                : wgpu::Color{ 0.1, 0.1, 0.12, 1.0 };

            wgpu::RenderPassDescriptor passDesc{};
            passDesc.colorAttachmentCount = 1;
            passDesc.colorAttachments     = &color;

            wgpu::RenderPassDepthStencilAttachment depth{};
            depth.view            = m_internalData->surfaceDepthTextureView;
            depth.depthLoadOp     = m_internalData->hasClearDepth ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
            depth.depthStoreOp    = wgpu::StoreOp::Store;
            depth.depthClearValue = m_internalData->hasClearDepth ? m_internalData->clearDepth : 1.0f;
            depth.stencilLoadOp   = wgpu::LoadOp::Undefined;
            depth.stencilStoreOp  = wgpu::StoreOp::Undefined;
            passDesc.depthStencilAttachment = &depth;

            m_internalData->activeRenderPass = m_internalData->activeEncoder.BeginRenderPass(&passDesc);
            m_internalData->surfaceWasRendered = true;
        }

        m_internalData->activeScissorValid = false;
        m_internalData->activeVBOs.fill(SR_ID_INVALID);

        if (!keepClearFlags) {
            m_internalData->hasClearColor = false;
            m_internalData->hasClearDepth = false;
        }

        return true;
    }

    void WebGPUPipeline::EndRender() {
        if (m_internalData->activeRenderPass) {
            m_internalData->activeRenderPass.End();
            m_internalData->activeRenderPass = nullptr;
        }

        /// Набор дескрипторов привязывается заново в каждом проходе
        m_internalData->activeBindGroupId = SR_ID_INVALID;

        Super::EndRender();
    }

    bool WebGPUPipeline::BeginCompute() {
        if (!Super::BeginCompute()) {
            return false;
        }

        if (!m_internalData->activeEncoder) {
            SR_ERROR("WebGPUPipeline::BeginCompute() : no active command encoder! Call OnFrameBuildBegin first.");
            Super::EndCompute();
            return false;
        }

        wgpu::ComputePassDescriptor passDesc{};
        m_internalData->activeComputePass = m_internalData->activeEncoder.BeginComputePass(&passDesc);
        if (!m_internalData->activeComputePass) {
            SR_ERROR("WebGPUPipeline::BeginCompute() : failed to begin compute pass!");
            Super::EndCompute();
            return false;
        }

        m_internalData->activeBindGroupId = SR_ID_INVALID;
        return true;
    }

    void WebGPUPipeline::EndCompute() {
        if (m_internalData->activeComputePass) {
            m_internalData->activeComputePass.End();
            m_internalData->activeComputePass = nullptr;
        }

        m_internalData->activeBindGroupId = SR_ID_INVALID;
        Super::EndCompute();
    }

    void WebGPUPipeline::WaitComputeIdle() {
        // WebGPU commands are ordered within a command encoder. The queue submission
        // provides the required synchronization; there is no blocking compute wait.
        if (m_internalData->activeEncoder && !m_internalData->activeComputePass && !m_internalData->activeRenderPass) {
            const auto commands = m_internalData->activeEncoder.Finish();
            m_internalData->queue.Submit(1, &commands);
            m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder();
        }
        Super::WaitComputeIdle();
    }

    void WebGPUPipeline::ClearBuffers() {
        Super::ClearBuffers();
        m_internalData->clearColors.clear();
        m_internalData->clearColor    = { 0.0, 0.0, 0.0, 1.0 };
        m_internalData->clearDepth    = 1.0f;
        m_internalData->hasClearColor = true;
        m_internalData->hasClearDepth = true;
    }

    void WebGPUPipeline::ClearBuffers(float_t r, float_t g, float_t b, float_t a, float_t depth, uint8_t colorCount) {
        Super::ClearBuffers(r, g, b, a, depth, colorCount);
        m_internalData->clearColors.clear();
        for (uint8_t i = 0; i < colorCount; ++i) {
            m_internalData->clearColors.emplace_back(wgpu::Color{r, g, b, a});
        }
        m_internalData->clearColor    = { r, g, b, a };
        m_internalData->clearDepth    = depth;
        m_internalData->hasClearColor = colorCount > 0;
        m_internalData->hasClearDepth = true;
    }

    void WebGPUPipeline::ClearBuffers(const ClearColors& clearColors, std::optional<float_t> depth) {
        Super::ClearBuffers(clearColors, depth);
        m_internalData->clearColors.clear();
        m_internalData->hasClearColor = !clearColors.empty();
        m_internalData->hasClearDepth = depth.has_value();
        for (const auto& color : clearColors) {
            m_internalData->clearColors.emplace_back(wgpu::Color{color.r, color.g, color.b, color.a});
        }
        if (!clearColors.empty()) {
            const auto& c = clearColors[0];
            m_internalData->clearColor    = { c.r, c.g, c.b, c.a };
            m_internalData->hasClearColor = true;
        }
        if (depth.has_value()) {
            m_internalData->clearDepth    = depth.value();
            m_internalData->hasClearDepth = true;
        }
    }

    void WebGPUPipeline::SetViewport(int32_t width, int32_t height) {
        Super::SetViewport(width, height);

        if (!m_internalData->activeRenderPass) {
            return;
        }

        auto&& pFBO = m_state.pFrameBuffer;
        const uint32_t w = (width  > 0) ? static_cast<uint32_t>(width)  : (pFBO ? pFBO->GetWidth()  : m_internalData->surfaceWidth);
        const uint32_t h = (height > 0) ? static_cast<uint32_t>(height) : (pFBO ? pFBO->GetHeight() : m_internalData->surfaceHeight);
        m_internalData->viewportWidth = w;
        m_internalData->viewportHeight = h;

        m_internalData->activeRenderPass.SetViewport(0.f, 0.f, static_cast<float>(w), static_cast<float>(h), 0.f, 1.f);
    }

    void WebGPUPipeline::SetScissor(int32_t width, int32_t height) {
        Super::SetScissor(width, height);

        auto&& pFBO = m_state.pFrameBuffer;
        const uint32_t w = (width  > 0) ? static_cast<uint32_t>(width)  : (pFBO ? pFBO->GetWidth()  : m_internalData->surfaceWidth);
        const uint32_t h = (height > 0) ? static_cast<uint32_t>(height) : (pFBO ? pFBO->GetHeight() : m_internalData->surfaceHeight);
        m_internalData->scissor = SR_MATH_NS::IRect(0, 0, static_cast<int32_t>(w), static_cast<int32_t>(h));
        m_internalData->activeScissorValid = false;
    }

    void WebGPUPipeline::SetSampleCount(uint8_t count) {
        m_requestedSampleCount = count;
        const uint8_t supportedCount = ResolveSupportedSampleCount(count);
        if (m_sampleProbePending == 0 && supportedCount != count) {
            SR_LOG("WebGPUPipeline::SetSampleCount() : sample count {} is not supported, using {}.", count, supportedCount);
        }
        Super::SetSampleCount(supportedCount);
    }

    uint8_t WebGPUPipeline::ResolveSupportedSampleCount(uint8_t requested) const noexcept {
        for (uint8_t sampleCount = m_supportedSampleCount; sampleCount > 1; sampleCount /= 2) {
            if (sampleCount <= requested && (m_supportedSampleCounts & sampleCount) != 0) {
                return sampleCount;
            }
        }
        return 1;
    }

    void WebGPUPipeline::ProbeSupportedSampleCount(uint8_t sampleCount) {
        wgpu::TextureDescriptor colorDesc{};
        colorDesc.size = { 1, 1, 1 };
        colorDesc.format = m_internalData->surfaceFormat;
        colorDesc.usage = wgpu::TextureUsage::RenderAttachment;
        colorDesc.mipLevelCount = 1;
        colorDesc.sampleCount = sampleCount;
        colorDesc.dimension = wgpu::TextureDimension::e2D;

        wgpu::TextureDescriptor depthDesc = colorDesc;
        depthDesc.format = wgpu::TextureFormat::Depth32Float;

        m_internalData->device.PushErrorScope(wgpu::ErrorFilter::Validation);
        auto colorTexture = m_internalData->device.CreateTexture(&colorDesc);
        auto depthTexture = m_internalData->device.CreateTexture(&depthDesc);
        m_internalData->device.PopErrorScope(wgpu::CallbackMode::AllowProcessEvents,
            [weak = GetWeakThis<WebGPUPipeline>(), sampleCount, colorTexture, depthTexture](wgpu::PopErrorScopeStatus, wgpu::ErrorType type, wgpu::StringView) mutable {
                const auto self = weak.Lock();
                if (!self || self->m_internalData->destroyed) { return; }
                if (type == wgpu::ErrorType::NoError && colorTexture && depthTexture) {
                    self->m_supportedSampleCounts |= sampleCount;
                    self->m_supportedSampleCount = SR_MAX(self->m_supportedSampleCount, sampleCount);
                }
                if (colorTexture) {
                    colorTexture.Destroy();
                }
                if (depthTexture) {
                    depthTexture.Destroy();
                }

                if (--self->m_sampleProbePending == 0) {
                    self->SetSampleCount(self->m_requestedSampleCount);
                    SR_INFO("WebGPUPipeline::ProbeSupportedSampleCount() : maximum supported sample count is {}.", self->m_supportedSampleCount);
                }
            });
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Draw commands — delegated to the currently open render pass
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::UseShader(uint32_t shaderProgram) {
        Super::UseShader(shaderProgram);

        if (m_internalData->activeComputePass) {
            const int32_t id = static_cast<int32_t>(shaderProgram);
            if (id >= 0 && m_internalData->shaderPrograms.IsAlive(id)) {
                auto&& prog = m_internalData->shaderPrograms.At(id);
                if (prog.computePipeline) {
                    m_internalData->activeComputePass.SetPipeline(prog.computePipeline);
                }
            }
            return;
        }

        if (!m_internalData->activeRenderPass) {
            return;
        }

        const int32_t id = static_cast<int32_t>(shaderProgram);
        if (id >= 0 && m_internalData->shaderPrograms.IsAlive(id)) {
            auto&& prog = m_internalData->shaderPrograms.At(id);
            if (prog.renderPipeline) {
                m_internalData->activeRenderPass.SetPipeline(prog.renderPipeline);
            }
        }
    }

    void WebGPUPipeline::BindVBO(uint32_t VBO, uint32_t slot, VertexInputRate inputRate) {
        Super::BindVBO(VBO, slot, inputRate);

        if (!m_internalData->activeRenderPass) {
            return;
        }

        const int32_t id = static_cast<int32_t>(VBO);
        if (slot < m_internalData->activeVBOs.size()) { m_internalData->activeVBOs[slot] = id; }
        if (id >= 0 && m_internalData->VBOs.IsAlive(id)) {
            auto&& buf = m_internalData->VBOs.At(id);
            if (buf) {
                m_internalData->activeRenderPass.SetVertexBuffer(slot, buf, 0, WGPU_WHOLE_SIZE);
            }
        }
    }

    void WebGPUPipeline::BindIBO(uint32_t IBO) {
        Super::BindIBO(IBO);

        if (!m_internalData->activeRenderPass) {
            return;
        }

        const int32_t id = static_cast<int32_t>(IBO);
        if (id >= 0 && m_internalData->IBOs.IsAlive(id)) {
            auto&& buf = m_internalData->IBOs.At(id);
            if (buf) {
                const auto found = m_internalData->indexSizes.find(id);
                const auto format = found != m_internalData->indexSizes.end() && found->second == 2
                    ? wgpu::IndexFormat::Uint16 : wgpu::IndexFormat::Uint32;
                m_internalData->activeRenderPass.SetIndexBuffer(buf, format, 0, WGPU_WHOLE_SIZE);
            }
        }
    }

    void WebGPUPipeline::BindTexture(const uint8_t activeTexture, const uint32_t textureId) {
        Super::BindTexture(activeTexture, textureId);
        BindTextureToDescriptorSet(activeTexture, textureId, false);
    }

    void WebGPUPipeline::BindAttachment(const uint8_t activeTexture, const uint32_t textureId) {
        Super::BindAttachment(activeTexture, textureId);
        BindTextureToDescriptorSet(activeTexture, textureId, true);
    }

    void WebGPUPipeline::BindTextureToDescriptorSet(const uint8_t binding, const uint32_t textureId, const bool isAttachment) {
        /// Текстуры, как и в Vulkan, принадлежат набору дескрипторов: движок привязывает их
        /// только когда они меняются, а не перед каждой отрисовкой.
        const int32_t id = m_internalData->activeBindGroupId;
        if (id < 0 || !m_internalData->bindGroups.IsAlive(id)) SR_UNLIKELY_ATTRIBUTE {
            PipelineError("WebGPUPipeline::BindTextureToDescriptorSet() : descriptor set is not binded!");
            return;
        }

        auto&& bindGroup = m_internalData->bindGroups.At(id);

        auto&& pIt = std::find_if(bindGroup.textures.begin(), bindGroup.textures.end(), [binding](auto&& texture) {
            return texture.binding == binding;
        });

        if (pIt != bindGroup.textures.end()) {
            if (pIt->textureId == static_cast<int32_t>(textureId) && pIt->isAttachment == isAttachment) {
                return;
            }
            pIt->textureId = static_cast<int32_t>(textureId);
            pIt->isAttachment = isAttachment;
        }
        else {
            bindGroup.textures.push_back(WebGPUBindGroup::TextureBinding { binding, static_cast<int32_t>(textureId), isAttachment });
        }

        bindGroup.isTexturesDirty = true;
    }

    void WebGPUPipeline::FlushTextureBindGroup() {
        if (!m_internalData->activeRenderPass && !m_internalData->activeComputePass) {
            return;
        }

        const int32_t shaderId = m_state.shaderId;
        if (shaderId < 0 || !m_internalData->shaderPrograms.IsAlive(shaderId)) {
            return;
        }

        auto&& program = m_internalData->shaderPrograms.At(shaderId);
        if (!program.bindGroupLayout1) {
            return;
        }

        const int32_t id = m_internalData->activeBindGroupId;
        if (id < 0 || !m_internalData->bindGroups.IsAlive(id)) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("WebGPUPipeline::FlushTextureBindGroup() : shader has samplers, but no descriptor set is binded!");
            return;
        }

        auto&& bindGroup = m_internalData->bindGroups.At(id);

        if (bindGroup.isTexturesDirty || bindGroup.texturesBuiltForShader != shaderId) {
            /// Помечаем группу собранной до попытки сборки, чтобы не спамить ошибками каждую отрисовку
            bindGroup.isTexturesDirty = false;
            bindGroup.texturesBuiltForShader = shaderId;
            bindGroup.textureBindGroup = nullptr;

            SR_UTILS_NS::Vector<wgpu::BindGroupEntry> entries;
            entries.reserve(program.samplerSlots.size() * 2);

            for (uint32_t slot = 0; slot < static_cast<uint32_t>(program.samplerSlots.size()); ++slot) {
                auto&& samplerSlot = program.samplerSlots[slot];

                auto&& pIt = std::find_if(bindGroup.textures.begin(), bindGroup.textures.end(), [&samplerSlot](auto&& texture) {
                    return texture.binding == samplerSlot.binding;
                });

                /// Группу нельзя привязать частично, поэтому непривязанные слоты получают заглушку
                int32_t textureId = pIt != bindGroup.textures.end() ? pIt->textureId : SR_ID_INVALID;
                if (textureId < 0 || !m_internalData->textures.IsAlive(textureId)) {
                    textureId = m_noneTextureId;
                }

                if (textureId < 0 || !m_internalData->textures.IsAlive(textureId)) SR_UNLIKELY_ATTRIBUTE {
                    SR_ERROR("WebGPUPipeline::FlushTextureBindGroup() : no texture for the sampler at binding {}!", samplerSlot.binding);
                    return;
                }

                auto&& texture = m_internalData->textures.At(textureId);

                wgpu::BindGroupEntry texEntry{};
                texEntry.binding     = slot * 2;
                texEntry.textureView = texture.view;
                entries.push_back(texEntry);

                wgpu::BindGroupEntry samplerEntry{};
                samplerEntry.binding = slot * 2 + 1;
                samplerEntry.sampler = (samplerSlot.isAttachment || !texture.sampler)
                    ? GetNonFilteringSampler(m_internalData)
                    : texture.sampler;
                entries.push_back(samplerEntry);
            }

            wgpu::BindGroupDescriptor bgDesc{};
            bgDesc.layout     = program.bindGroupLayout1;
            bgDesc.entryCount = static_cast<size_t>(entries.size());
            bgDesc.entries    = entries.empty() ? nullptr : entries.data();

            bindGroup.textureBindGroup = m_internalData->device.CreateBindGroup(&bgDesc);
            if (!bindGroup.textureBindGroup) {
                SR_ERROR("WebGPUPipeline::FlushTextureBindGroup() : failed to create bind group!");
                return;
            }
        }

        if (bindGroup.textureBindGroup) SR_LIKELY_ATTRIBUTE {
            if (m_internalData->activeComputePass) {
                m_internalData->activeComputePass.SetBindGroup(1, bindGroup.textureBindGroup, 0, nullptr);
            }
            else {
                m_internalData->activeRenderPass.SetBindGroup(1, bindGroup.textureBindGroup, 0, nullptr);
            }
        }
    }

    void WebGPUPipeline::InvalidateTextureBindGroups() {
        /// Закешированные группы могут ссылаться на уничтоженную текстуру, а идентификаторы
        /// в пуле переиспользуются, поэтому пересобираем их все.
        for (uint32_t i = 0; i < m_internalData->bindGroups.GetCapacity(); ++i) {
            if (!m_internalData->bindGroups.IsAlive(i)) {
                continue;
            }

            auto&& bindGroup = m_internalData->bindGroups.At(i);
            bindGroup.textureBindGroup = nullptr;
            bindGroup.isTexturesDirty = true;
        }
    }

    void WebGPUPipeline::InvalidateBufferBindGroups() {
        /// То же самое для буферов: BindGroup держит сам объект буфера, а не его идентификатор,
        /// поэтому после освобождения UBO/SSBO (индекс в пуле переиспользуется) группу
        /// обязательно нужно пересобрать, иначе шейдер будет читать уничтоженный буфер.
        for (uint32_t i = 0; i < m_internalData->bindGroups.GetCapacity(); ++i) {
            if (!m_internalData->bindGroups.IsAlive(i)) {
                continue;
            }

            auto&& bindGroup = m_internalData->bindGroups.At(i);
            bindGroup.bufferBindGroup = nullptr;
            bindGroup.isBuffersDirty = true;
        }
    }

    bool WebGPUPipeline::BindDescriptorSet(uint32_t descriptorSet) {
        if (!Super::BindDescriptorSet(descriptorSet)) {
            return false;
        }

        /// Сам BindGroup привязывается перед отрисовкой: движок обновляет биндинги набора
        /// уже после этого вызова, а BindGroup в WebGPU неизменяемый и пересоздается при обновлении.
        m_internalData->activeBindGroupId = static_cast<int32_t>(descriptorSet);

        return true;
    }

    void WebGPUPipeline::FlushDescriptorBindGroup() {
        if (!m_internalData->activeRenderPass && !m_internalData->activeComputePass) {
            return;
        }

        const int32_t id = m_internalData->activeBindGroupId;
        if (id < 0 || !m_internalData->bindGroups.IsAlive(id)) {
            return;
        }

        const int32_t shaderId = m_state.shaderId;
        if (shaderId < 0 || !m_internalData->shaderPrograms.IsAlive(shaderId)) {
            return;
        }

        auto&& program = m_internalData->shaderPrograms.At(shaderId);
        auto&& bindGroup = m_internalData->bindGroups.At(id);

        if (bindGroup.isBuffersDirty || bindGroup.buffersBuiltForShader != shaderId) {
            /// Помечаем группу собранной до попытки сборки, чтобы не спамить ошибками каждую отрисовку
            bindGroup.isBuffersDirty = false;
            bindGroup.buffersBuiltForShader = shaderId;
            bindGroup.bufferBindGroup = nullptr;

            SR_UTILS_NS::Vector<wgpu::BindGroupEntry> entries;
            entries.reserve(program.bufferSlots.size());

            for (auto&& slot : program.bufferSlots) {
                auto&& pIt = std::find_if(bindGroup.buffers.begin(), bindGroup.buffers.end(), [&slot](auto&& binding) {
                    return binding.binding == slot.binding;
                });

                if (pIt == bindGroup.buffers.end()) SR_UNLIKELY_ATTRIBUTE {
                    SR_ERROR("WebGPUPipeline::FlushDescriptorBindGroup() : binding {} of the shader is not updated!", slot.binding);
                    return;
                }

                auto&& pool = slot.isStorage ? m_internalData->SSBOs : m_internalData->UBOs;
                if (pIt->resourceId < 0 || !pool.IsAlive(pIt->resourceId)) SR_UNLIKELY_ATTRIBUTE {
                    SR_ERROR("WebGPUPipeline::FlushDescriptorBindGroup() : invalid buffer at binding {}!", slot.binding);
                    return;
                }

                auto&& buffer = pool.At(pIt->resourceId);

                wgpu::BindGroupEntry entry{};
                entry.binding = slot.binding;
                entry.buffer  = buffer;
                entry.offset  = 0;
                entry.size    = buffer.GetSize();
                entries.push_back(entry);
            }

            wgpu::BindGroupDescriptor bgDesc{};
            bgDesc.layout     = program.bindGroupLayout;
            bgDesc.entryCount = static_cast<size_t>(entries.size());
            bgDesc.entries    = entries.empty() ? nullptr : entries.data();

            bindGroup.bufferBindGroup = m_internalData->device.CreateBindGroup(&bgDesc);
            if (!bindGroup.bufferBindGroup) {
                SR_ERROR("WebGPUPipeline::FlushDescriptorBindGroup() : failed to create bind group!");
                return;
            }
        }

        if (bindGroup.bufferBindGroup) SR_LIKELY_ATTRIBUTE {
            if (m_internalData->activeComputePass) {
                m_internalData->activeComputePass.SetBindGroup(0, bindGroup.bufferBindGroup, 0, nullptr);
            }
            else {
                m_internalData->activeRenderPass.SetBindGroup(0, bindGroup.bufferBindGroup, 0, nullptr);
            }
        }
    }

    void WebGPUPipeline::FlushPushConstantBindGroup() {
        if (!m_internalData->activeRenderPass && !m_internalData->activeComputePass) {
            return;
        }

        const int32_t shaderId = m_state.shaderId;
        if (shaderId >= 0 && m_internalData->shaderPrograms.IsAlive(shaderId)) {
            auto&& program = m_internalData->shaderPrograms.At(shaderId);
            if (program.pushConstantBindGroup) {
                if (m_internalData->activeComputePass) {
                    m_internalData->activeComputePass.SetBindGroup(2, program.pushConstantBindGroup, 0, nullptr);
                }
                else {
                    m_internalData->activeRenderPass.SetBindGroup(2, program.pushConstantBindGroup, 0, nullptr);
                }
            }
        }
    }

    void WebGPUPipeline::PushConstants(void* pData, uint64_t size) {
        Super::PushConstants(pData, size);

        const int32_t shaderId = m_state.shaderId;
        if (!pData || shaderId < 0 || !m_internalData->shaderPrograms.IsAlive(shaderId)) {
            return;
        }

        auto&& program = m_internalData->shaderPrograms.At(shaderId);
        if (program.bindGroupLayout2 && program.pushConstantBuffer) {
            const uint64_t bufferSize = program.pushConstantBuffer.GetSize();

            /// Queue::WriteBuffer updates are visible before the encoded command buffer executes.
            /// Keep a separate buffer snapshot for draws that were already recorded.
            wgpu::BufferDescriptor bufferDesc{};
            bufferDesc.size = bufferSize;
            bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            program.pushConstantBuffer = m_internalData->device.CreateBuffer(&bufferDesc);

            m_internalData->queue.WriteBuffer(program.pushConstantBuffer, 0, pData,
                SR_MIN(size, bufferSize));

            wgpu::BindGroupEntry entry{};
            entry.binding = 0;
            entry.buffer = program.pushConstantBuffer;
            entry.offset = 0;
            entry.size = bufferSize;

            wgpu::BindGroupDescriptor bindGroupDesc{};
            bindGroupDesc.layout = program.bindGroupLayout2;
            bindGroupDesc.entryCount = 1;
            bindGroupDesc.entries = &entry;
            program.pushConstantBindGroup = m_internalData->device.CreateBindGroup(&bindGroupDesc);
        }
    }

    bool WebGPUPipeline::FlushScissor() {
        if (!m_internalData->activeRenderPass) {
            return false;
        }

        auto&& pFBO = m_state.pFrameBuffer;
        const int32_t targetWidth = static_cast<int32_t>(pFBO ? pFBO->GetWidth() : m_internalData->surfaceWidth);
        const int32_t targetHeight = static_cast<int32_t>(pFBO ? pFBO->GetHeight() : m_internalData->surfaceHeight);

        SR_MATH_NS::IRect scissor = m_scissorsStack.empty() ? m_internalData->scissor : m_scissorsStack.back();
        if (scissor.w <= 0 || scissor.h <= 0) {
            if (!m_scissorsStack.empty()) {
                return false;
            }
            scissor = SR_MATH_NS::IRect(0, 0, targetWidth, targetHeight);
        }

        const int32_t minX = std::max(scissor.Left(), 0);
        const int32_t minY = std::max(scissor.Bottom(), 0);
        const int32_t maxX = std::min(scissor.Right(), targetWidth);
        const int32_t maxY = std::min(scissor.Top(), targetHeight);
        if (minX >= maxX || minY >= maxY) {
            return false;
        }

        scissor = SR_MATH_NS::IRect(minX, minY, maxX - minX, maxY - minY);
        if (!m_internalData->activeScissorValid || m_internalData->activeScissor != scissor) {
            m_internalData->activeRenderPass.SetScissorRect(
                static_cast<uint32_t>(scissor.x), static_cast<uint32_t>(scissor.y),
                static_cast<uint32_t>(scissor.w), static_cast<uint32_t>(scissor.h)
            );
            m_internalData->activeScissor = scissor;
            m_internalData->activeScissorValid = true;
        }

        return true;
    }

    void WebGPUPipeline::Draw(uint32_t count) {
        Super::Draw(count);

        if (!m_internalData->activeRenderPass || count == 0 || !FlushScissor()) {
            return;
        }

        FlushDescriptorBindGroup();
        FlushTextureBindGroup();
        FlushPushConstantBindGroup();

        m_internalData->activeRenderPass.Draw(count, m_drawInstancesCount, 0, m_drawInstancesStart);
    }

    void WebGPUPipeline::DrawIndices(uint32_t count) {
        Super::DrawIndices(count);

        if (!m_internalData->activeRenderPass || count == 0 || !FlushScissor()) {
            return;
        }

        FlushDescriptorBindGroup();
        FlushTextureBindGroup();
        FlushPushConstantBindGroup();

        m_internalData->activeRenderPass.DrawIndexed(count, m_drawInstancesCount, 0, 0, m_drawInstancesStart);
    }

    void WebGPUPipeline::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        Super::Dispatch(groupCountX, groupCountY, groupCountZ);

        if (!m_internalData->activeComputePass || groupCountX == 0 || groupCountY == 0 || groupCountZ == 0) {
            return;
        }

        const int32_t shaderId = m_state.shaderId;
        if (shaderId < 0 || !m_internalData->shaderPrograms.IsAlive(shaderId)) {
            PipelineError("WebGPUPipeline::Dispatch() : current compute shader is invalid!");
            return;
        }

        auto&& program = m_internalData->shaderPrograms.At(shaderId);
        if (!program.isCompute || !program.computePipeline) {
            PipelineError("WebGPUPipeline::Dispatch() : current shader is not a compute shader!");
            return;
        }

        FlushDescriptorBindGroup();
        FlushTextureBindGroup();
        FlushPushConstantBindGroup();
        m_internalData->activeComputePass.DispatchWorkgroups(groupCountX, groupCountY, groupCountZ);
    }

    void WebGPUPipeline::UpdateDescriptorSets(uint32_t descriptorSet, const SRDescriptorUpdateInfos& updateInfo) {
        Super::UpdateDescriptorSets(descriptorSet, updateInfo);

        const int32_t id = static_cast<int32_t>(descriptorSet);
        if (id < 0 || !m_internalData->bindGroups.IsAlive(id)) {
            return;
        }

        // WebGPU BindGroups are immutable and the engine updates bindings one at a time, so the
        // update is only accumulated here — the BindGroup itself is rebuilt before the draw call.
        auto&& bindGroup = m_internalData->bindGroups.At(id);

        for (auto&& info : updateInfo) {
            if (info.descriptorType != DescriptorType::Uniform && info.descriptorType != DescriptorType::Storage) {
                continue;
            }

            const auto resourceId = static_cast<int32_t>(info.ubo);

            auto&& pIt = std::find_if(bindGroup.buffers.begin(), bindGroup.buffers.end(), [&info](auto&& binding) {
                return binding.binding == info.binding;
            });

            if (pIt != bindGroup.buffers.end()) {
                if (pIt->resourceId == resourceId && pIt->type == info.descriptorType) {
                    continue;
                }
                pIt->resourceId = resourceId;
                pIt->type = info.descriptorType;
            }
            else {
                bindGroup.buffers.push_back(WebGPUBindGroup::BufferBinding { info.binding, info.descriptorType, resourceId });
            }

            bindGroup.isBuffersDirty = true;
        }
    }

    bool WebGPUPipeline::Destroy() {
        if (m_internalData->destroyed) { return true; }
        m_internalData->destroyed = true;
        DestroyOverlay();
        if (m_internalData->activeRenderPass) { m_internalData->activeRenderPass.End(); m_internalData->activeRenderPass = nullptr; }
        if (m_internalData->activeComputePass) { m_internalData->activeComputePass.End(); m_internalData->activeComputePass = nullptr; }
        m_internalData->activeEncoder = nullptr;
        m_internalData->surfaceCommandBuffer = nullptr;
        m_internalData->pixelReadbacks.clear();
        m_internalData->hostBuffers.clear();
        m_internalData->bindGroups.Clear();
        m_internalData->shaderPrograms.Clear();
        m_internalData->textures.Clear();
        m_internalData->frameBuffers.Clear();
        m_internalData->VBOs.Clear();
        m_internalData->IBOs.Clear();
        m_internalData->UBOs.Clear();
        m_internalData->SSBOs.Clear();
        m_internalData->surfaceTextureView = nullptr;
        m_internalData->surfaceTexture = nullptr;
        m_internalData->surfaceColorTexture = nullptr;
        m_internalData->surfaceDepthTextureView = nullptr;
        m_internalData->surfaceDepthTexture = nullptr;
        m_internalData->surfaceMultisampleTextureView = nullptr;
        m_internalData->surfaceMultisampleTexture = nullptr;
        m_internalData->nonFilteringSampler = nullptr;
        if (m_internalData->surface) { m_internalData->surface.Unconfigure(); }
        m_internalData->surface = nullptr;
        m_internalData->queue = nullptr;
        if (m_internalData->device) { m_internalData->device.Destroy(); }
        m_internalData->device = nullptr;
        return Super::Destroy();
    }

    bool WebGPUPipeline::InitOverlay() {
        SR_TRACY_ZONE;

    #ifdef SR_USE_IMGUI
        if (SR_UTILS_NS::StoreUtils::Temp::GetBool("ImGuiOverlayEnabled")) {
            auto&& pImGuiOverlay = m_overlays[OverlayType::ImGui];
            pImGuiOverlay = new WebGPUImGuiOverlay(GetThis());
            if (!pImGuiOverlay->Init()) {
                PipelineError("WebGPUPipeline::InitOverlay() : failed to initialize ImGui overlay!");
                return false;
            }
            SR_LOG("WebGPUPipeline::InitOverlay() : imgui overlay initialized!");
        }
        else {
            SR_LOG("WebGPUPipeline::InitOverlay() : imgui overlay is disabled!");
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
        /// Хендл резолвится через объект шейдера (как в VulkanPipeline), а не через m_state.shaderId:
        /// последний выставляется только UseShader() при записи отрисовки и сбрасывается UnUseShader(),
        /// а этот хендл используется как ключ в UBOManager и DescriptorManager в том числе вне прохода
        /// (см. Shader::BeginSharedUBO(), который делает только SetCurrentShader()). Иначе на запись
        /// и на привязку shared-UBO приходятся разные ключи, и шейдер читает не тот буфер.
        const int32_t id = m_state.pShader ? m_state.pShader->GetId() : m_state.shaderId;

        if (id >= 0 && m_internalData && m_internalData->shaderPrograms.IsAlive(id)) {
            auto&& prog = m_internalData->shaderPrograms.At(id);
            if (prog.isCompute) {
                return reinterpret_cast<void*>(prog.computePipeline.Get());
            }
            return reinterpret_cast<void*>(prog.renderPipeline.Get());
        }

        return nullptr;
    }

    void* WebGPUPipeline::GetCurrentFBOHandle() const {
        /// Тоже как в Vulkan: приоритет у объекта кадрового буфера, т.к. m_state.frameBufferId
        /// обновляется только в BindFrameBuffer(), а идентификатор нужен и вне прохода.
        const int32_t id = m_state.pFrameBuffer ? m_state.pFrameBuffer->GetId() : m_state.frameBufferId;

        if (id >= 0 && m_internalData && m_internalData->frameBuffers.IsAlive(id)) {
            auto&& fbo = m_internalData->frameBuffers.At(id);
            if (!fbo.colorAttachments.empty() && fbo.colorAttachments[0].texture) {
                return reinterpret_cast<void*>(fbo.colorAttachments[0].texture.Get());
            }
        }

        /// Идентификатор swapchain'а
        return reinterpret_cast<void*>(1);
    }

    uint8_t WebGPUPipeline::GetFrameBufferSampleCount() const {
        const int32_t id = m_state.pFrameBuffer ? m_state.pFrameBuffer->GetId() : m_state.frameBufferId;
        if (id > 0 && m_internalData && m_internalData->frameBuffers.IsAlive(id)) {
            return m_internalData->frameBuffers.At(id).sampleCount;
        }
        return IsMultiSamplingSupported() ? GetSamplesCount() : 1;
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

    SR_MATH_NS::FColor WebGPUPipeline::GetPixelColor(uint32_t texture, uint32_t x, uint32_t y) {
        SR_MATH_NS::FColor color(0.f);
        const uint64_t request = RequestPixelRange(SR_ID_INVALID, texture, x, y, 1, 1);
        if (request == SR_ID_INVALID) { return color; }
        if (m_internalData->activeEncoder) {
            const auto commands = m_internalData->activeEncoder.Finish();
            m_internalData->queue.Submit(1, &commands);
            m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder();
            StartReadbackMappings();
        }
        const double_t deadline = emscripten_get_now() + 30000.0;
        while (!IsPixelRangeReady(request) && !m_internalData->failed && emscripten_get_now() < deadline) {
            emscripten_sleep(1);
        }
        GetPixelRangeResult(request, &color, 1, 1);
        ReleasePixelRangeRequest(request);
        return color;
    }

    uint64_t WebGPUPipeline::RequestPixelRange(uint64_t workId, uint32_t textureId, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        if (!m_internalData || width == 0 || height == 0) {
            return SR_ID_INVALID;
        }

        const auto id = static_cast<int32_t>(textureId);
        if (id < 0 || !m_internalData->textures.IsAlive(id)) {
            return SR_ID_INVALID;
        }

        auto& source = m_internalData->textures.At(id).texture;
        if (!source) {
            return SR_ID_INVALID;
        }
        const auto format = source.GetFormat();
        const uint32_t texelBytes = TexelBytes(format);
        if (!texelBytes || IsDepthFormat(format) || source.GetSampleCount() != 1
            || (source.GetUsage() & wgpu::TextureUsage::CopySrc) == wgpu::TextureUsage::None
            || x >= source.GetWidth() || y >= source.GetHeight()
            || width > source.GetWidth() - x || height > source.GetHeight() - y
            || m_internalData->activeRenderPass || m_internalData->activeComputePass) {
            SR_ERROR("Pixel readback requires a readable color texture and must be requested outside a pass.");
            return SR_ID_INVALID;
        }

        if (workId != SR_ID_INVALID) {
            ReleasePixelRangeRequest(workId);
        }

        const uint32_t bytesPerRow = ((width * texelBytes + 255) / 256) * 256;
        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.size = static_cast<uint64_t>(bytesPerRow) * height;
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        auto buffer = m_internalData->device.CreateBuffer(&bufferDesc);
        if (!buffer) {
            return SR_ID_INVALID;
        }

        auto encoder = m_internalData->activeEncoder;
        const bool submitImmediately = !encoder;
        if (submitImmediately) {
            encoder = m_internalData->device.CreateCommandEncoder();
            if (!encoder) {
                return SR_ID_INVALID;
            }
        }

        wgpu::TexelCopyTextureInfo src{};
        src.texture = source;
        src.origin = { x, y, 0 };
        src.aspect = wgpu::TextureAspect::All;
        wgpu::TexelCopyBufferInfo dst{};
        dst.buffer = buffer;
        dst.layout.bytesPerRow = bytesPerRow;
        dst.layout.rowsPerImage = height;
        wgpu::Extent3D extent{ width, height, 1 };
        encoder.CopyTextureToBuffer(&src, &dst, &extent);

        if (submitImmediately) {
            auto commandBuffer = encoder.Finish();
            if (!commandBuffer) {
                return SR_ID_INVALID;
            }
            m_internalData->queue.Submit(1, &commandBuffer);
        }

        WebGPUPipelineInternalData::PixelReadback request;
        request.id = m_internalData->nextPixelReadbackId++;
        request.buffer = buffer;
        request.width = width;
        request.height = height;
        request.bytesPerRow = bytesPerRow;
        request.format = format;
        m_internalData->pixelReadbacks.emplace_back(std::move(request));
        if (submitImmediately) { StartReadbackMappings(); }
        return m_internalData->pixelReadbacks.back().id;
    }

    bool WebGPUPipeline::IsPixelRangeReady(uint64_t workId) const {
        if (!m_internalData) return false;

        // MapAsync callbacks are dispatched by Dawn only while its event queue
        // is being pumped.  A readback can be submitted after OnFrameBuildBegin
        // (for example, from the scene viewer mouse handler), so waiting for the
        // next frame's event pump may leave the request pending indefinitely.
        const_cast<WebGPUPipelineInternalData*>(m_internalData)->instance.ProcessEvents();

        for (const auto& request : m_internalData->pixelReadbacks) {
            if (request.id == workId) return request.ready;
        }
        return false;
    }

    bool WebGPUPipeline::GetPixelRangeResult(uint64_t workId, SR_MATH_NS::FColor* pixels, uint32_t width, uint32_t height) {
        if (!m_internalData || !pixels) return false;
        for (auto& request : m_internalData->pixelReadbacks) {
            if (request.id != workId || !request.ready || request.failed
                || request.width != width || request.height != height)
            {
                continue;
            }
            const auto* data = static_cast<const uint8_t*>(request.buffer.GetConstMappedRange(0, request.height * request.bytesPerRow));
            if (!data) return false;
            for (uint32_t row = 0; row < height; ++row) {
                const auto* rowData = data + row * request.bytesPerRow;
                for (uint32_t column = 0; column < width; ++column) {
                    const uint32_t stride = TexelBytes(request.format);
                    const auto* pixel = rowData + column * stride;
                    float_t channels[4] = {0.f, 0.f, 0.f, 1.f};
                    if (request.format == wgpu::TextureFormat::R32Float) { std::memcpy(channels, pixel, 4); }
                    else if (request.format == wgpu::TextureFormat::R16Float || request.format == wgpu::TextureFormat::RGBA16Float) {
                        for (uint32_t c = 0; c < stride / 2; ++c) {
                            uint16_t half; std::memcpy(&half, pixel + c * 2, 2); channels[c] = HalfToFloat(half);
                        }
                    }
                    else if (request.format == wgpu::TextureFormat::R32Uint || request.format == wgpu::TextureFormat::R16Uint || request.format == wgpu::TextureFormat::R8Uint) {
                        uint32_t integer = 0; std::memcpy(&integer, pixel, stride); channels[0] = static_cast<float_t>(integer);
                    }
                    else {
                        // Byte readback returns 0..255, as in Vulkan and the color-buffer picking contract.
                        channels[3] = 255.f;
                        for (uint32_t c = 0; c < stride; ++c) { channels[c] = static_cast<float_t>(pixel[c]); }
                        if (request.format == wgpu::TextureFormat::BGRA8Unorm || request.format == wgpu::TextureFormat::BGRA8UnormSrgb) {
                            std::swap(channels[0], channels[2]);
                        }
                    }
                    pixels[row * width + column] = SR_MATH_NS::FColor(channels[0], channels[1], channels[2], channels[3]);
                }
            }
            request.buffer.Unmap();
            request.ready = false;
            request.failed = true;
            return true;
        }
        return false;
    }

    void WebGPUPipeline::ReleasePixelRangeRequest(uint64_t workId) {
        if (!m_internalData) return;
        for (uint32_t i = 0; i < m_internalData->pixelReadbacks.size(); ++i) {
            if (m_internalData->pixelReadbacks[i].id == workId) {
                m_internalData->pixelReadbacks.erase(m_internalData->pixelReadbacks.begin() + i);
                return;
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Buffer allocations
    // ----------------------------------------------------------------------------------------------------------------

    SR_NODISCARD int32_t WebGPUPipeline::AllocateVBO(int32_t VBO, uint64_t size, const void* pData) {
        if (!pData || size == 0) {
            return SR_ID_INVALID;
        }

        wgpu::BufferDescriptor desc{};
        desc.size  = (size + 3) & ~uint64_t(3);
        desc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;

        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        WriteAlignedBuffer(m_internalData->queue, pBuffer, 0, pData, size);

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
        const uint64_t dataSize = static_cast<uint64_t>(indexSize) * count;
        desc.size  = (dataSize + 3) & ~uint64_t(3);
        desc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;

        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        WriteAlignedBuffer(m_internalData->queue, pBuffer, 0, pIndices, dataSize);

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += desc.size;

        const int32_t id = m_internalData->IBOs.Add(std::move(pBuffer));
        m_internalData->indexSizes[id] = indexSize;
        return id;
    }

    void WebGPUPipeline::WriteBufferOrdered(const wgpu::Buffer& buffer, uint64_t offset, const void* data, uint64_t size) {
        if (!buffer || !data || size == 0 || offset % 4 || offset > buffer.GetSize() || size > buffer.GetSize() - offset) { return; }
        if (!m_internalData->activeEncoder) {
            WriteAlignedBuffer(m_internalData->queue, buffer, offset, data, size);
            return;
        }
        const uint64_t aligned = (size + 3) & ~uint64_t(3);
        if (aligned > buffer.GetSize() - offset) { return; }
        wgpu::BufferDescriptor descriptor{};
        descriptor.size = aligned;
        descriptor.usage = wgpu::BufferUsage::CopySrc;
        descriptor.mappedAtCreation = true;
        auto upload = m_internalData->device.CreateBuffer(&descriptor);
        if (!upload) { return; }
        void* mapped = upload.GetMappedRange(0, aligned);
        if (!mapped) { return; }
        std::memset(mapped, 0, aligned);
        std::memcpy(mapped, data, size);
        upload.Unmap();

        const bool render = static_cast<bool>(m_internalData->activeRenderPass);
        const bool compute = static_cast<bool>(m_internalData->activeComputePass);
        const auto vertexBuffers = m_internalData->activeVBOs;
        const int32_t index = m_state.IBOId;
        const int32_t shader = m_state.shaderId;
        const int32_t descriptors = m_internalData->activeBindGroupId;
        const uint32_t viewportWidth = m_internalData->viewportWidth;
        const uint32_t viewportHeight = m_internalData->viewportHeight;
        if (render) { EndRender(); }
        if (compute) { EndCompute(); }
        m_internalData->activeEncoder.CopyBufferToBuffer(upload, 0, buffer, offset, aligned);
        if (render) {
            const bool clearColor = m_internalData->hasClearColor;
            const bool clearDepth = m_internalData->hasClearDepth;
            m_internalData->hasClearColor = false;
            m_internalData->hasClearDepth = false;
            const bool resumed = BeginRender();
            m_internalData->hasClearColor = clearColor;
            m_internalData->hasClearDepth = clearDepth;
            if (!resumed) { return; }
            SetViewport(viewportWidth ? static_cast<int32_t>(viewportWidth) : -1, viewportHeight ? static_cast<int32_t>(viewportHeight) : -1);
            for (uint32_t slot = 0; slot < vertexBuffers.size(); ++slot) {
                if (vertexBuffers[slot] >= 0) { BindVBO(vertexBuffers[slot], slot, VertexInputRate::Vertex); }
            }
            if (index >= 0) { BindIBO(index); }
        }
        if (compute && !BeginCompute()) { return; }
        if (render || compute) {
            if (shader >= 0) { UseShader(shader); }
            if (descriptors >= 0) { BindDescriptorSet(descriptors); }
        }
    }

    bool WebGPUPipeline::UpdateVBO(int32_t VBO, const void* pData, uint64_t size) {
        if (!pData || size == 0 || VBO < 0 || !m_internalData->VBOs.IsAlive(VBO)) {
            return false;
        }

        auto&& buffer = m_internalData->VBOs.At(VBO);
        if (!buffer || buffer.GetSize() < size) {
            return false;
        }

        /// WriteBuffer упорядочен с отправленными командами, ожидание GPU не нужно
        WriteBufferOrdered(buffer, 0, pData, size);
        ++m_state.operations;

        return true;
    }

    bool WebGPUPipeline::UpdateIBO(int32_t IBO, const void* pData, uint64_t size) {
        if (!pData || size == 0 || IBO < 0 || !m_internalData->IBOs.IsAlive(IBO)) {
            return false;
        }

        auto&& buffer = m_internalData->IBOs.At(IBO);
        if (!buffer || buffer.GetSize() < size) {
            return false;
        }

        WriteBufferOrdered(buffer, 0, pData, size);
        ++m_state.operations;

        return true;
    }

    SR_NODISCARD int32_t WebGPUPipeline::AllocateUBO(uint32_t uboSize) {
        if (uboSize == 0 || uboSize > m_internalData->limits.maxUniformBufferBindingSize) {
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
        if (ssboSize == 0 || ssboSize > m_internalData->limits.maxStorageBufferBindingSize) {
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

        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        desc.mappedAtCreation = false;
        wgpu::Buffer pBuffer = m_internalData->device.CreateBuffer(&desc);
        if (!pBuffer) {
            return SR_ID_INVALID;
        }

        ++m_state.operations;
        ++m_state.allocations;
        m_state.allocatedMemory += desc.size;

        const int32_t id = m_internalData->SSBOs.Add(std::move(pBuffer));
        if (usage == SSBOUsage::CPUToGPU || usage == SSBOUsage::CPUCopy || usage == SSBOUsage::CPUOnly
            || usage == SSBOUsage::AutoPreferHost) {
            auto& host = m_internalData->hostBuffers.emplace_back();
            host.id = id;
            host.data.resize(desc.size);
            std::memset(host.data.data(), 0, desc.size);
        }
        return id;
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
        SR_UTILS_NS::String wgslSource;
        const auto cacheShadersRoot = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders");

        auto readStageFile = [&](ShaderStage stage) -> bool {
            auto it = createInfo.stages.find(stage);
            if (it == createInfo.stages.end() || it->second.path.empty()) {
                return false;
            }
            // The path stored in stages is relative to the cache/Shaders root
            const auto fullPath = cacheShadersRoot.Concat(it->second.path);
            if (!SR_UTILS_NS::FileSystem::ReadFile(fullPath, wgslSource) || wgslSource.empty()) {
                return false;
            }
            shaderName = SR_UTILS_NS::StringView(it->second.path.c_str(), it->second.path.size());
            return true;
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
                const SR_UTILS_NS::StringView msgView =
                    (message.length != WGPU_STRLEN)
                        ? SR_UTILS_NS::StringView(message.data, message.length)
                        : SR_UTILS_NS::StringView(message.data ? message.data : "(no message)");
                SR_ERROR("WebGPUPipeline : WGSL shader compilation failed: {}", msgView);
                uint32_t lineNum = 1;
                SR_UTILS_NS::String text;
                SR_UTILS_NS::StringView remaining(wgslSource);
                while (!remaining.empty()) {
                    const auto newline = remaining.find('\n');
                    text += "  {:>4} | {}\n"_format(lineNum++, remaining.substr(0, newline));
                    if (newline == SR_UTILS_NS::StringView::npos) {
                        break;
                    }
                    remaining.remove_prefix(newline + 1);
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
        //   @group(0) — UBOs and SSBOs, each one at its own SRSL binding
        //   @group(1) — textures + samplers: the N-th sampler of the shader (ordered by its SRSL
        //               binding) occupies bindings N * 2 (texture) and N * 2 + 1 (sampler)
        // We must declare a BindGroupLayout for every group index the shader references.
        //
        // createInfo.uniforms holds one entry per (resource, stage) pair, so the same binding may
        // be listed several times — duplicates are merged and their visibility is ORed together.

        // Group 0: UBOs and SSBOs
        SR_UTILS_NS::Vector<wgpu::BindGroupLayoutEntry> bglEntries0;
        SR_UTILS_NS::Vector<WebGPUBufferSlot> bufferSlots;

        auto findEntry = [](SR_UTILS_NS::Vector<wgpu::BindGroupLayoutEntry>& entries, uint32_t binding) -> wgpu::BindGroupLayoutEntry* {
            for (auto&& entry : entries) {
                if (entry.binding == binding) {
                    return &entry;
                }
            }
            return nullptr;
        };

        for (auto&& uniform : createInfo.uniforms) {
            if (uniform.type != LayoutBinding::Uniform && uniform.type != LayoutBinding::SSBO) {
                continue;
            }

            const auto binding = static_cast<uint32_t>(uniform.binding);
            const bool isUniform = uniform.type == LayoutBinding::Uniform;

            const wgpu::ShaderStage visibility = ShaderStageToWGPU(uniform.stage);
            if (!isUniform && !uniform.storageReadOnly && uniform.stage == ShaderStage::Vertex) {
                SR_ERROR("Writable storage binding {} cannot be used in the vertex stage. Mark the SRSL block readonly.", binding);
                return SR_ID_INVALID;
            }

            if (auto&& pEntry = findEntry(bglEntries0, binding)) {
                pEntry->visibility |= visibility;
                continue;
            }

            wgpu::BindGroupLayoutEntry entry{};
            entry.binding               = binding;
            entry.visibility            = visibility;
            entry.buffer.type           = isUniform ? wgpu::BufferBindingType::Uniform
                : (uniform.storageReadOnly ? wgpu::BufferBindingType::ReadOnlyStorage : wgpu::BufferBindingType::Storage);
            /// minBindingSize намеренно оставлен нулевым (= проверка минимального размера отключена).
            /// uniform.size приходит из SRSLUniformBlock::Align(), который просто складывает размеры
            /// полей (vec3 = 12 байт), а реальный буфер выделяется по Memory::ShaderUBOBlock, который
            /// раскладывает поля по правилам std140 (vec3 занимает свой 16-байтный слот) — как и WGSL.
            /// Из-за этого uniform.size меньше того, что видит шейдер, и валидация падала с
            /// "The shader uses more bytes of the buffer (208) than the layout's minBindingSize (192)".
            /// Фактический размер привязки всё равно проверяется при создании bind group и при отрисовке.
            entry.buffer.minBindingSize = 0;

            bglEntries0.push_back(entry);
            bufferSlots.push_back(WebGPUBufferSlot { binding, !isUniform });
        }

        // Group 1: textures + samplers (each sampler occupies 2 bindings: texture then sampler)
        struct SamplerLayoutInfo {
            uint32_t binding = 0;
            bool isAttachment = false;
            SamplerDimension dimension = SamplerDimension::Unknown;
            wgpu::ShaderStage visibility = wgpu::ShaderStage::None;
        };

        SR_UTILS_NS::Vector<SamplerLayoutInfo> samplerInfos;

        for (auto&& uniform : createInfo.uniforms) {
            if (uniform.type != LayoutBinding::Sampler2D && uniform.type != LayoutBinding::Attachhment
                && uniform.type != LayoutBinding::UnfilterableSampler2D)
            {
                continue;
            }

            const auto binding = static_cast<uint32_t>(uniform.binding);
            const wgpu::ShaderStage visibility = ShaderStageToWGPU(uniform.stage);

            auto&& pIt = std::find_if(samplerInfos.begin(), samplerInfos.end(), [binding](auto&& info) {
                return info.binding == binding;
            });

            if (pIt != samplerInfos.end()) {
                pIt->visibility |= visibility;
                pIt->isAttachment |= uniform.type == LayoutBinding::Attachhment
                    || uniform.type == LayoutBinding::UnfilterableSampler2D;
                continue;
            }

            SamplerLayoutInfo info;
            info.binding = binding;
            info.isAttachment = uniform.type == LayoutBinding::Attachhment
                || uniform.type == LayoutBinding::UnfilterableSampler2D;
            info.dimension = uniform.samplerDimension;
            info.visibility = visibility;

            samplerInfos.push_back(info);
        }

        // The WGSL generator numbers samplers in ascending binding order, so the layout must too.
        std::sort(samplerInfos.begin(), samplerInfos.end(), [](auto&& left, auto&& right) {
            return left.binding < right.binding;
        });

        SR_UTILS_NS::Vector<wgpu::BindGroupLayoutEntry> bglEntries1;
        SR_UTILS_NS::Vector<WebGPUSamplerSlot> samplerSlots;

        for (uint32_t slot = 0; slot < static_cast<uint32_t>(samplerInfos.size()); ++slot) {
            auto&& info = samplerInfos[slot];
            const bool isDepth = info.dimension == SamplerDimension::Texture2DShadow
                              || info.dimension == SamplerDimension::Texture2DArrayDepth;

            wgpu::TextureSampleType sampleType = wgpu::TextureSampleType::Float;
            if (isDepth) {
                sampleType = wgpu::TextureSampleType::Depth;
            }
            else if (info.isAttachment) {
                sampleType = wgpu::TextureSampleType::UnfilterableFloat;
            }

            wgpu::BindGroupLayoutEntry texEntry{};
            texEntry.binding               = slot * 2;
            texEntry.visibility            = info.visibility;
            texEntry.texture.sampleType    = sampleType;
            texEntry.texture.viewDimension = SamplerDimensionToWGPU(info.dimension);
            bglEntries1.push_back(texEntry);

            wgpu::BindGroupLayoutEntry samplerEntry{};
            samplerEntry.binding      = slot * 2 + 1;
            samplerEntry.visibility   = info.visibility;
            samplerEntry.sampler.type = info.isAttachment || info.dimension == SamplerDimension::Texture2DArrayDepth
                ? wgpu::SamplerBindingType::NonFiltering
                : wgpu::SamplerBindingType::Filtering;
            bglEntries1.push_back(samplerEntry);

            samplerSlots.push_back(WebGPUSamplerSlot { info.binding, info.isAttachment });
        }

        // Create BindGroupLayout for group 0 (always present, even if empty)
        wgpu::BindGroupLayoutDescriptor bglDesc0{};
        bglDesc0.entryCount = static_cast<size_t>(bglEntries0.size());
        bglDesc0.entries    = bglEntries0.empty() ? nullptr : bglEntries0.data();

        wgpu::BindGroupLayout bindGroupLayout = m_internalData->device.CreateBindGroupLayout(&bglDesc0);

        uint64_t pushConstantSize = 0;
        for (auto&& stageEntry : createInfo.stages) {
            for (auto&& pushConstant : stageEntry.second.pushConstants) {
                pushConstantSize = SR_MAX(pushConstantSize, pushConstant.offset + pushConstant.size);
            }
        }
        const bool hasPushConstants = pushConstantSize > 0;

        // Create BindGroupLayout for group 1. An empty layout is required when group 2 is used
        // without samplers because WebGPU bind group indices must be contiguous.
        wgpu::BindGroupLayout bindGroupLayout1;
        if (!bglEntries1.empty() || hasPushConstants) {
            wgpu::BindGroupLayoutDescriptor bglDesc1{};
            bglDesc1.entryCount = static_cast<size_t>(bglEntries1.size());
            bglDesc1.entries    = bglEntries1.data();
            bindGroupLayout1 = m_internalData->device.CreateBindGroupLayout(&bglDesc1);
        }

        wgpu::BindGroupLayout bindGroupLayout2;
        wgpu::Buffer pushConstantBuffer;
        wgpu::BindGroup pushConstantBindGroup;
        if (hasPushConstants) {
            wgpu::BindGroupLayoutEntry pushConstantEntry{};
            pushConstantEntry.binding = 0;
            pushConstantEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute;
            pushConstantEntry.buffer.type = wgpu::BufferBindingType::Uniform;

            wgpu::BindGroupLayoutDescriptor bglDesc2{};
            bglDesc2.entryCount = 1;
            bglDesc2.entries = &pushConstantEntry;
            bindGroupLayout2 = m_internalData->device.CreateBindGroupLayout(&bglDesc2);

            const uint64_t bufferSize = SR_MAX(uint64_t(16), (pushConstantSize + 15) & ~uint64_t(15));
            wgpu::BufferDescriptor bufferDesc{};
            bufferDesc.size = bufferSize;
            bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            pushConstantBuffer = m_internalData->device.CreateBuffer(&bufferDesc);

            wgpu::BindGroupEntry pushConstantBindGroupEntry{};
            pushConstantBindGroupEntry.binding = 0;
            pushConstantBindGroupEntry.buffer = pushConstantBuffer;
            pushConstantBindGroupEntry.offset = 0;
            pushConstantBindGroupEntry.size = bufferSize;

            wgpu::BindGroupDescriptor pushConstantBindGroupDesc{};
            pushConstantBindGroupDesc.layout = bindGroupLayout2;
            pushConstantBindGroupDesc.entryCount = 1;
            pushConstantBindGroupDesc.entries = &pushConstantBindGroupEntry;
            pushConstantBindGroup = m_internalData->device.CreateBindGroup(&pushConstantBindGroupDesc);
        }

        // Build the pipeline layout covering all used bind group indices.
        // If group 1 is used, we must include both group 0 and group 1 layouts.
        wgpu::BindGroupLayout bglArray[3] = { bindGroupLayout, bindGroupLayout1, bindGroupLayout2 };

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = hasPushConstants ? 3u : (bglEntries1.empty() ? 1u : 2u);
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
        program.bindGroupLayout2 = bindGroupLayout2;
        program.pushConstantBuffer = pushConstantBuffer;
        program.pushConstantBindGroup = pushConstantBindGroup;
        program.pipelineLayout  = pipelineLayout;
        program.bufferSlots     = std::move(bufferSlots);
        program.samplerSlots    = std::move(samplerSlots);
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
            const bool hasVertexLayout = createInfo.vertexLayoutDescriptions.GetLayoutsCount() > 0;
            const auto* pVertexLayout = hasVertexLayout
                ? &createInfo.vertexLayoutDescriptions.GetLayout(0)
                : nullptr;

            createInfo.vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& attr, uint32_t location) {
                wgpu::VertexAttribute vkAttr{};
                vkAttr.format         = VertexAttributeFormatToWGPU(attr.format, attr.count);
                vkAttr.offset         = attr.offset;
                vkAttr.shaderLocation = location;
                vertexAttributes.push_back(vkAttr);
            });

            const bool isInstanced = pVertexLayout && pVertexLayout->instanced;

            wgpu::VertexBufferLayout vertexBufferLayout{};
            vertexBufferLayout.arrayStride    = pVertexLayout ? pVertexLayout->GetStride() : 0;
            vertexBufferLayout.stepMode       = isInstanced
                ? wgpu::VertexStepMode::Instance
                : wgpu::VertexStepMode::Vertex;
            vertexBufferLayout.attributeCount = static_cast<size_t>(vertexAttributes.size());
            vertexBufferLayout.attributes     = vertexAttributes.data();

            // ---- Determine color target formats from FBO or surface ----
            // The RenderPipeline must declare exactly the same color attachment formats
            // as the RenderPass it will be used with. We get them from the FBO that was
            // passed to AllocateShaderProgram (fbo argument = the FBO id from the pipeline).
            SR_UTILS_NS::Vector<wgpu::TextureFormat> colorFormats;

            if (fbo > 0 && m_internalData->frameBuffers.IsAlive(fbo)) {
                // FBO exists: collect its color attachment formats
                auto&& fboData = m_internalData->frameBuffers.At(fbo);
                for (auto&& att : fboData.colorAttachments) {
                    colorFormats.push_back(att.format);
                }
            }

            if (colorFormats.empty()) {
                // Fallback: swapchain format (also used by SwapchainPass shaders where fbo == 0)
                colorFormats.push_back(m_internalData->surfaceFormat != wgpu::TextureFormat::Undefined
                    ? m_internalData->surfaceFormat
                    : wgpu::TextureFormat::BGRA8Unorm);
            }

            // Determine depth format from FBO
            wgpu::TextureFormat depthFormat = wgpu::TextureFormat::Depth32Float; // sensible default
            if (fbo > 0 && m_internalData->frameBuffers.IsAlive(fbo)) {
                auto&& fboData = m_internalData->frameBuffers.At(fbo);
                if (fboData.depthAttachment.format != wgpu::TextureFormat::Undefined) {
                    depthFormat = fboData.depthAttachment.format;
                }
            }

            const bool hasDepthAttachment = fbo > 0 && m_internalData->frameBuffers.IsAlive(fbo)
                ? static_cast<bool>(m_internalData->frameBuffers.At(fbo).depthAttachment.view)
                : static_cast<bool>(m_internalData->surfaceDepthTextureView);

            // Build per-target blend state (shared value, same for all targets)
            wgpu::BlendState blendState{};
            if (createInfo.blendEnabled) {
                blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
                blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                blendState.color.operation = wgpu::BlendOperation::Add;
                blendState.alpha.srcFactor = wgpu::BlendFactor::One;
                blendState.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                blendState.alpha.operation = wgpu::BlendOperation::Add;
            }

            SR_UTILS_NS::Vector<wgpu::ColorTargetState> colorTargets;
            colorTargets.reserve(colorFormats.size());
            for (auto fmt : colorFormats) {
                wgpu::ColorTargetState ct{};
                ct.format = fmt;
                ct.writeMask = wgslSource.find("struct FragmentOutput") != SR_UTILS_NS::String::npos
                    ? wgpu::ColorWriteMask::All
                    : wgpu::ColorWriteMask::None;
                /// Блендинг включаем только для тех вложений, формат которых его поддерживает.
                /// Вспомогательные вложения (глубина в R32Float, id объекта и т.п.) блендинг не поддерживают.
                if (createInfo.blendEnabled && IsBlendableFormat(fmt)) {
                    ct.blend = &blendState;
                }
                colorTargets.push_back(ct);
            }

            wgpu::FragmentState fragmentState{};
            fragmentState.module      = shaderModule;
            fragmentState.entryPoint  = "fragment";
            fragmentState.targetCount = static_cast<size_t>(colorTargets.size());
            fragmentState.targets     = colorTargets.data();

            wgpu::DepthStencilState depthStencilState{};
            if (hasDepthAttachment) {
                depthStencilState.format              = depthFormat;
                depthStencilState.depthWriteEnabled   = createInfo.depthWrite;
                depthStencilState.depthCompare        = createInfo.depthTest
                    ? DepthCompareToWGPU(createInfo.depthCompare)
                    : wgpu::CompareFunction::Always;
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
#if defined(SR_EMSCRIPTEN)
            rpDesc.primitive.frontFace         = wgpu::FrontFace::CW;
#else
            rpDesc.primitive.frontFace         = wgpu::FrontFace::CCW;
#endif

            rpDesc.fragment = &fragmentState;

            if (hasDepthAttachment) {
                rpDesc.depthStencil = &depthStencilState;
            }

            rpDesc.multisample.count = (fbo > 0 && m_internalData->frameBuffers.IsAlive(fbo))
                ? m_internalData->frameBuffers.At(fbo).sampleCount
                : (IsMultiSamplingSupported() ? GetSamplesCount() : 1);
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
        if (format == wgpu::TextureFormat::Undefined || createInfo.width > m_internalData->limits.maxTextureDimension2D
            || createInfo.height > m_internalData->limits.maxTextureDimension2D) { return SR_ID_INVALID; }
        SR_UTILS_NS::Vector<uint8_t> converted;
        const uint8_t* uploadData = nullptr;
        uint64_t uploadSize = 0;
        if (!isDepth && !ConvertTextureUpload(createInfo, format, converted, uploadData, uploadSize)) {
            SR_ERROR("Texture upload data does not match its dimensions/format.");
            return SR_ID_INVALID;
        }

        uint32_t channelCount = 0;
        switch (createInfo.format) {
            case SR_GRAPH_NS::ImageFormat::R8_UNORM: channelCount = 1; break;
            case SR_GRAPH_NS::ImageFormat::RG8_UNORM: channelCount = 2; break;
            case SR_GRAPH_NS::ImageFormat::RGBA8_UNORM:
            case SR_GRAPH_NS::ImageFormat::BGRA8_UNORM:
            case SR_GRAPH_NS::ImageFormat::RGBA8_SRGB: channelCount = 4; break;
            case SR_GRAPH_NS::ImageFormat::RGB8_UNORM:
            case SR_GRAPH_NS::ImageFormat::RGB8_SRGB: channelCount = 4; break;
            case SR_GRAPH_NS::ImageFormat::R16_UNORM:
            case SR_GRAPH_NS::ImageFormat::R16_SFLOAT:
            case SR_GRAPH_NS::ImageFormat::R32_SFLOAT: channelCount = 1; break;
            case SR_GRAPH_NS::ImageFormat::RGBA16_UNORM:
            case SR_GRAPH_NS::ImageFormat::RGBA16_SFLOAT:
            case SR_GRAPH_NS::ImageFormat::RGB16_UNORM:
            case SR_GRAPH_NS::ImageFormat::B10G11R11_UFLOAT_PACK32: channelCount = 4; break;
            default: break;
        }
        const uint32_t channelBytes = channelCount ? TexelBytes(format) / channelCount : 0;
        const uint64_t baseLevelSize = static_cast<uint64_t>(createInfo.width) * createInfo.height * TexelBytes(format);
        const bool canGenerateMipmaps = !isDepth && uploadData
            && channelCount > 0 && uploadSize >= baseLevelSize;
        uint32_t maxMipLevelCount = 1;
        for (uint32_t maxDimension = std::max(createInfo.width, createInfo.height); maxDimension > 1; maxDimension /= 2) {
            ++maxMipLevelCount;
        }

        const uint32_t requestedMipLevelCount = createInfo.mipLevels == 0
            ? maxMipLevelCount
            : std::min<uint32_t>(createInfo.mipLevels, maxMipLevelCount);
        const uint32_t mipLevelCount = canGenerateMipmaps ? requestedMipLevelCount : 1;

        wgpu::TextureUsage usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
        if (isDepth) {
            usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
        }

        wgpu::TextureDescriptor texDesc{};
        texDesc.size            = { createInfo.width, createInfo.height, 1 };
        texDesc.format          = format;
        texDesc.usage           = usage;
        texDesc.mipLevelCount   = mipLevelCount;
        texDesc.sampleCount     = 1;
        texDesc.dimension       = wgpu::TextureDimension::e2D;

        wgpu::Texture texture = m_internalData->device.CreateTexture(&texDesc);
        if (!texture) {
            SR_ERROR("WebGPUPipeline::AllocateTexture() : failed to create texture!");
            return SR_ID_INVALID;
        }

        // Upload data if provided
        if (uploadData && uploadSize > 0 && !isDepth) {
            wgpu::TexelCopyTextureInfo dst{};
            dst.texture  = texture;
            dst.mipLevel = 0;
            dst.origin   = { 0, 0, 0 };
            dst.aspect   = wgpu::TextureAspect::All;

            const uint32_t uploadChannelCount = TexelBytes(format);

            wgpu::TexelCopyBufferLayout layout{};
            layout.offset       = 0;
            layout.bytesPerRow  = createInfo.width * uploadChannelCount;
            layout.rowsPerImage = createInfo.height;

            wgpu::Extent3D extent{ createInfo.width, createInfo.height, 1 };
            m_internalData->queue.WriteTexture(&dst, uploadData, uploadSize, &layout, &extent);

            if (canGenerateMipmaps && mipLevelCount > 1) {
                SR_UTILS_NS::Vector<uint8_t> previousMip;
                SR_UTILS_NS::Vector<uint8_t> currentMip;
                previousMip.resize(static_cast<size_t>(baseLevelSize));
                std::memcpy(previousMip.data(), uploadData, static_cast<size_t>(baseLevelSize));

                uint32_t previousWidth = createInfo.width;
                uint32_t previousHeight = createInfo.height;

                for (uint32_t mipLevel = 1; mipLevel < mipLevelCount; ++mipLevel) {
                    const uint32_t mipWidth = std::max(1u, previousWidth / 2);
                    const uint32_t mipHeight = std::max(1u, previousHeight / 2);
                    currentMip.resize(static_cast<size_t>(mipWidth) * mipHeight * channelCount * channelBytes);

                    for (uint32_t y = 0; y < mipHeight; ++y) {
                        for (uint32_t x = 0; x < mipWidth; ++x) {
                            for (uint32_t channel = 0; channel < channelCount; ++channel) {
                                double_t sum = 0;
                                uint32_t samples = 0;
                                for (uint32_t sourceY = y * previousHeight / mipHeight; sourceY < (y + 1) * previousHeight / mipHeight; ++sourceY) {
                                    for (uint32_t sourceX = x * previousWidth / mipWidth; sourceX < (x + 1) * previousWidth / mipWidth; ++sourceX) {
                                        const size_t sourceIndex = (static_cast<size_t>(sourceY) * previousWidth + sourceX) * channelCount + channel;
                                        double_t sample = 0;
                                        if (channelBytes == 1) { sample = previousMip[sourceIndex] / 255.0; }
                                        else if (channelBytes == 2) {
                                            uint16_t half; std::memcpy(&half, previousMip.data() + sourceIndex * 2, 2); sample = HalfToFloat(half);
                                        }
                                        else {
                                            float_t value; std::memcpy(&value, previousMip.data() + sourceIndex * 4, 4); sample = value;
                                        }
                                        const bool srgb = format == wgpu::TextureFormat::RGBA8UnormSrgb && channel < 3;
                                        sum += srgb ? (sample <= 0.04045 ? sample / 12.92 : std::pow((sample + 0.055) / 1.055, 2.4)) : sample;
                                        ++samples;
                                    }
                                }

                                const size_t targetIndex = (static_cast<size_t>(y) * mipWidth + x) * channelCount + channel;
                                double_t average = sum / samples;
                                if (format == wgpu::TextureFormat::RGBA8UnormSrgb && channel < 3) {
                                    average = average <= 0.0031308 ? average * 12.92 : 1.055 * std::pow(average, 1.0 / 2.4) - 0.055;
                                }
                                if (channelBytes == 1) {
                                    currentMip[targetIndex] = static_cast<uint8_t>(std::clamp(average * 255.0 + 0.5, 0.0, 255.0));
                                }
                                else if (channelBytes == 2) {
                                    const uint16_t half = FloatToHalf(static_cast<float_t>(average));
                                    std::memcpy(currentMip.data() + targetIndex * 2, &half, 2);
                                }
                                else {
                                    const float_t value = static_cast<float_t>(average);
                                    std::memcpy(currentMip.data() + targetIndex * 4, &value, 4);
                                }
                            }
                        }
                    }

                    wgpu::TexelCopyTextureInfo mipDst{};
                    mipDst.texture = texture;
                    mipDst.mipLevel = mipLevel;
                    mipDst.aspect = wgpu::TextureAspect::All;

                    wgpu::TexelCopyBufferLayout mipLayout{};
                    mipLayout.bytesPerRow = mipWidth * channelCount * channelBytes;
                    mipLayout.rowsPerImage = mipHeight;

                    const wgpu::Extent3D mipExtent{ mipWidth, mipHeight, 1 };
                    m_internalData->queue.WriteTexture(&mipDst, currentMip.data(), currentMip.size(), &mipLayout, &mipExtent);

                    std::swap(previousMip, currentMip);
                    previousWidth = mipWidth;
                    previousHeight = mipHeight;
                    emscripten_sleep(0);
                }
            }
        }

        // Create view
        wgpu::TextureViewDescriptor viewDesc{};
        viewDesc.format          = format;
        viewDesc.dimension       = wgpu::TextureViewDimension::e2D;
        viewDesc.mipLevelCount   = mipLevelCount;
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
        if (w > m_internalData->limits.maxTextureDimension2D || h > m_internalData->limits.maxTextureDimension2D
            || (createInfo.colors && createInfo.colors->size() > m_internalData->limits.maxColorAttachments)) {
            SR_ERROR("Framebuffer exceeds WebGPU device limits.");
            return SR_ID_INVALID;
        }

        WebGPUFrameBuffer fbo;
        fbo.width  = w;
        fbo.height = h;
        fbo.sampleCount = ResolveSupportedSampleCount(createInfo.sampleCount);
        const uint32_t depthLayers = createInfo.pDepth
            ? SR_MAX(1u, ((createInfo.layersCount > 1) ? createInfo.layersCount : createInfo.arrayLayersCount))
            : 1;
        if (createInfo.pDepth && (createInfo.features.depthShaderRead || depthLayers > 1)) {
            fbo.sampleCount = 1;
        }

        // Color attachments
        if (createInfo.colors) {
            for (auto&& colorLayer : *createInfo.colors) {
                WebGPUFrameBuffer::Attachment att;
                // For color layers, resolve Auto/Unknown/None to RGBA8_UNORM (not depth)
                att.format = ImageFormatToWGPU(colorLayer.format, false);
                if (att.format == wgpu::TextureFormat::Undefined || IsDepthFormat(att.format)) { return SR_ID_INVALID; }

                wgpu::TextureDescriptor texDesc{};
                texDesc.size          = { w, h, 1 };
                texDesc.format        = att.format;
                texDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc;
                texDesc.mipLevelCount = 1;
                texDesc.sampleCount   = fbo.sampleCount;
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
                    if (fbo.sampleCount > 1) {
                        texDesc.sampleCount = 1;
                        att.resolveTexture = m_internalData->device.CreateTexture(&texDesc);
                        att.resolveView = att.resolveTexture.CreateView();
                        webTexture.view = att.resolveTexture.CreateView();
                    }
                    else {
                        webTexture.view = att.texture.CreateView();
                    }
                    webTexture.texture = att.resolveTexture ? att.resolveTexture : att.texture;
                }

                for (auto&& oldTexture : colorLayer.texture) {
                    if (oldTexture != SR_ID_INVALID) {
                        m_internalData->textures.At(oldTexture).Destroy();
                        m_internalData->textures.RemoveByIndex(oldTexture);
                        InvalidateTextureBindGroups();
                    }
                }
                colorLayer.texture.clear();

                webTexture.sampler = m_internalData->device.CreateSampler(&samplerDesc);

                const auto colorId = m_internalData->textures.Add(std::move(webTexture));
                colorLayer.texture.emplace_back(colorId);

                fbo.colorAttachments.push_back(std::move(att));
            }
        }

        // Depth attachment
        if (createInfo.pDepth) {
            for (int32_t textureId : createInfo.pDepth->texture) {
                if (textureId >= 0 && m_internalData->textures.IsAlive(textureId)) {
                    m_internalData->textures.At(textureId).Destroy();
                    m_internalData->textures.RemoveByIndex(textureId);
                }
            }
            createInfo.pDepth->texture.clear();
            for (auto&& subLayer : createInfo.pDepth->subLayers) {
                for (int32_t textureId : subLayer) {
                    if (textureId >= 0 && m_internalData->textures.IsAlive(textureId)) {
                        m_internalData->textures.At(textureId).Destroy();
                        m_internalData->textures.RemoveByIndex(textureId);
                    }
                }
            }
            createInfo.pDepth->subLayers.clear();
            InvalidateTextureBindGroups();

            // Always use isDepthHint=true so Auto/Unknown/None resolves to Depth32Float
            wgpu::TextureFormat depthFmt = ImageFormatToWGPU(createInfo.pDepth->format, true);

            fbo.depthAttachment.format = depthFmt;

            wgpu::TextureDescriptor depthDesc{};
            depthDesc.size          = { w, h, depthLayers };
            depthDesc.format        = depthFmt;
            depthDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
            depthDesc.mipLevelCount = 1;
            depthDesc.sampleCount   = fbo.sampleCount;
            depthDesc.dimension     = wgpu::TextureDimension::e2D;

            fbo.depthAttachment.texture = m_internalData->device.CreateTexture(&depthDesc);
            if (fbo.depthAttachment.texture) {
                for (uint32_t layer = 0; layer < depthLayers; ++layer) {
                    wgpu::TextureViewDescriptor layerViewDesc{};
                    layerViewDesc.format          = depthFmt;
                    layerViewDesc.dimension       = wgpu::TextureViewDimension::e2D;
                    layerViewDesc.baseArrayLayer  = layer;
                    layerViewDesc.mipLevelCount   = 1;
                    layerViewDesc.arrayLayerCount = 1;
                    layerViewDesc.aspect          = wgpu::TextureAspect::DepthOnly;
                    fbo.depthAttachment.layerViews.push_back(fbo.depthAttachment.texture.CreateView(&layerViewDesc));
                }
                fbo.depthAttachment.view = fbo.depthAttachment.layerViews.front();

                if (fbo.sampleCount == 1) {
                    wgpu::TextureViewDescriptor arrayViewDesc{};
                    arrayViewDesc.format          = depthFmt;
                    arrayViewDesc.dimension       = wgpu::TextureViewDimension::e2DArray;
                    arrayViewDesc.mipLevelCount   = 1;
                    arrayViewDesc.arrayLayerCount = depthLayers;
                    arrayViewDesc.aspect          = wgpu::TextureAspect::DepthOnly;

                    WebGPUTexture sampledDepth;
                    sampledDepth.view = fbo.depthAttachment.texture.CreateView(&arrayViewDesc);
                    const int32_t sampledDepthId = m_internalData->textures.Add(std::move(sampledDepth));
                    const auto frameCount = createInfo.pFBO ? createInfo.pFBO->size() : 1;
                    createInfo.pDepth->texture.resize(frameCount, sampledDepthId);

                    createInfo.pDepth->subLayers.resize(depthLayers);
                    for (uint32_t layer = 0; layer < depthLayers; ++layer) {
                        WebGPUTexture sampledLayer;
                        sampledLayer.view = fbo.depthAttachment.layerViews[layer];
                        const int32_t sampledLayerId = m_internalData->textures.Add(std::move(sampledLayer));
                        createInfo.pDepth->subLayers[layer].resize(frameCount, sampledLayerId);
                    }
                }
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
            texDesc.usage         = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc;
            texDesc.mipLevelCount = 1;
            texDesc.sampleCount   = fbo.sampleCount;
            texDesc.dimension     = wgpu::TextureDimension::e2D;

            att.texture = m_internalData->device.CreateTexture(&texDesc);
            if (att.texture) {
                att.view = att.texture.CreateView();
                if (fbo.sampleCount > 1) {
                    texDesc.sampleCount = 1;
                    att.resolveTexture = m_internalData->device.CreateTexture(&texDesc);
                    att.resolveView = att.resolveTexture.CreateView();
                }
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
            depthDesc.sampleCount   = fbo.sampleCount;
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

        ++m_state.operations;
        ++m_state.allocations;

        int32_t fboId = SR_ID_INVALID;
        if (createInfo.pFBO) {
            for (const int32_t oldFboId : *createInfo.pFBO) {
                if (oldFboId > 0 && m_internalData->frameBuffers.IsAlive(oldFboId)) {
                    fboId = oldFboId;
                    break;
                }
            }
        }

        if (fboId != SR_ID_INVALID) {
            auto&& oldFbo = m_internalData->frameBuffers.At(fboId);
            oldFbo.Destroy();
            oldFbo = std::move(fbo);
        }
        else {
            fboId = m_internalData->frameBuffers.Add(std::move(fbo));
        }

        // Write the allocated id back into every slot of pFBO so that Framebuffer::GetId() works.
        if (createInfo.pFBO) {
            for (auto& slot : *createInfo.pFBO) {
                slot = fboId;
            }
        }

        return fboId;
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

        const auto format = ImageFormatToWGPU(createInfo.format);
        if (format == wgpu::TextureFormat::Undefined || IsDepthFormat(format) || createInfo.width != createInfo.height
            || createInfo.width > m_internalData->limits.maxTextureDimension2D) {
            SR_ERROR("Cubemap requires a supported color format and square faces within device limits.");
            return SR_ID_INVALID;
        }
        uint32_t maxMips = 1;
        for (uint32_t dimension = createInfo.width; dimension > 1; dimension /= 2) { ++maxMips; }
        const uint32_t mips = createInfo.mipLevels ? std::min<uint32_t>(maxMips, createInfo.mipLevels) : maxMips;
        wgpu::TextureDescriptor texDesc{};
        texDesc.size            = { createInfo.width, createInfo.height, 6 };
        texDesc.format          = format;
        texDesc.usage           = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc;
        texDesc.mipLevelCount   = mips;
        texDesc.sampleCount     = 1;
        texDesc.dimension       = wgpu::TextureDimension::e2D;

        wgpu::Texture texture = m_internalData->device.CreateTexture(&texDesc);
        if (!texture) {
            SR_ERROR("WebGPUPipeline::AllocateCubeMap() : failed to create cube texture!");
            return SR_ID_INVALID;
        }

        for (uint32_t face = 0; face < 6; ++face) {
            if (!createInfo.data[face]) { continue; }
            uint32_t sourceBytes = TexelBytes(format);
            if (createInfo.format == ImageFormat::RGB8_UNORM || createInfo.format == ImageFormat::RGB8_SRGB) { sourceBytes = 3; }
            if (createInfo.format == ImageFormat::RGB16_UNORM) { sourceBytes = 6; }
            if (createInfo.format == ImageFormat::B10G11R11_UFLOAT_PACK32) { sourceBytes = 4; }
            SRTextureCreateInfo faceInfo;
            faceInfo.width = createInfo.width; faceInfo.height = createInfo.height;
            faceInfo.format = createInfo.format; faceInfo.mipLevels = mips;
            faceInfo.pData = createInfo.data[face];
            faceInfo.filter = createInfo.filter;
            faceInfo.imageSize = createInfo.faceSizes[face] ? createInfo.faceSizes[face]
                : static_cast<uint64_t>(createInfo.width) * createInfo.height * sourceBytes;
            const int32_t faceId = AllocateTexture(faceInfo);
            if (faceId == SR_ID_INVALID) { return SR_ID_INVALID; }
            const auto source = m_internalData->textures.At(faceId).texture;
            auto encoder = m_internalData->device.CreateCommandEncoder();
            for (uint32_t mip = 0; mip < std::min(mips, source.GetMipLevelCount()); ++mip) {
                wgpu::TexelCopyTextureInfo src{}; src.texture = source; src.mipLevel = mip;
                wgpu::TexelCopyTextureInfo dst{}; dst.texture = texture; dst.mipLevel = mip; dst.origin = {0, 0, face};
                const wgpu::Extent3D extent{std::max(1u, createInfo.width >> mip), std::max(1u, createInfo.height >> mip), 1};
                encoder.CopyTextureToTexture(&src, &dst, &extent);
            }
            const auto commands = encoder.Finish();
            m_internalData->queue.Submit(1, &commands);
            // Submitted copies retain their resources; release the temporary pool slot.
            m_internalData->textures.RemoveByIndex(faceId);
            InvalidateTextureBindGroups();
        }

        // Cube view
        wgpu::TextureViewDescriptor viewDesc{};
        viewDesc.format          = format;
        viewDesc.dimension       = wgpu::TextureViewDimension::Cube;
        viewDesc.mipLevelCount   = mips;
        viewDesc.arrayLayerCount = 6;
        viewDesc.aspect          = wgpu::TextureAspect::All;

        wgpu::TextureView view = texture.CreateView(&viewDesc);

        wgpu::SamplerDescriptor samplerDesc{};
        samplerDesc.addressModeU  = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeV  = wgpu::AddressMode::ClampToEdge;
        samplerDesc.addressModeW  = wgpu::AddressMode::ClampToEdge;
        samplerDesc.magFilter     = TextureFilterToWGPU(createInfo.filter);
        samplerDesc.minFilter     = TextureFilterToWGPU(createInfo.filter);
        samplerDesc.mipmapFilter  = TextureFilterToMipWGPU(createInfo.filter);
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

        // The types array is positional and says nothing about the actual bindings, while a WebGPU
        // BindGroup must exactly match the layout of the shader it is used with. So only a slot is
        // reserved here: the BindGroup is built from the accumulated bindings before the draw call
        // (see UpdateDescriptorSets and FlushDescriptorBindGroup).

        ++m_state.operations;
        ++m_state.allocations;

        return m_internalData->bindGroups.Add(WebGPUBindGroup());
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
            if (size > buf.GetSize() || (sizesMustBeEqual && ((size + 15) & ~uint64_t(15)) != buf.GetSize())) {
                SR_ERROR("WebGPU uniform update size {} does not match capacity {}", size, buf.GetSize());
                return;
            }
            wgpu::BufferDescriptor desc{};
            desc.size = buf.GetSize();
            desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            buf = m_internalData->device.CreateBuffer(&desc);
            WriteAlignedBuffer(m_internalData->queue, buf, 0, pData, size);
            InvalidateBufferBindGroups();
        }
    }

    void WebGPUPipeline::UpdateSSBO(uint32_t SSBO, void* pData, uint64_t size) {
        if (!m_internalData->queue || !pData || size == 0) {
            return;
        }
        if (m_internalData->SSBOs.IsAlive(static_cast<int32_t>(SSBO))) {
            auto&& buf = m_internalData->SSBOs.At(static_cast<int32_t>(SSBO));
            if (size > buf.GetSize()) { return; }
            WriteBufferOrdered(buf, 0, pData, size);
            for (auto& host : m_internalData->hostBuffers) {
                if (host.id == static_cast<int32_t>(SSBO)) { std::memcpy(host.data.data(), pData, size); }
            }
        }
    }

    bool WebGPUPipeline::MapSSBO(uint32_t SSBO, void** ppData) {
        if (!ppData) { return false; }
        *ppData = nullptr;
        const int32_t id = static_cast<int32_t>(SSBO);
        if (m_internalData->destroyed || !m_internalData->SSBOs.IsAlive(id)) { return false; }
        for (auto& host : m_internalData->hostBuffers) {
            if (host.id == id) {
                host.mapped = true;
                *ppData = host.data.data();
                return true;
            }
        }
        m_internalData->instance.ProcessEvents();
        for (auto& request : m_internalData->pixelReadbacks) {
            if (request.ssboId != id) { continue; }
            if (request.failed) { UnMapSSBO(SSBO); return false; }
            if (!request.ready || request.failed) { return false; }
            if (!request.mapped) { request.mapped = const_cast<void*>(request.buffer.GetConstMappedRange(0, request.bytesPerRow)); }
            *ppData = request.mapped;
            return *ppData != nullptr;
        }
        auto& source = m_internalData->SSBOs.At(id);
        wgpu::BufferDescriptor desc{};
        desc.size = source.GetSize();
        desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        WebGPUPipelineInternalData::PixelReadback request;
        request.id = m_internalData->nextPixelReadbackId++;
        request.ssboId = id;
        request.height = 1;
        request.bytesPerRow = static_cast<uint32_t>(desc.size);
        request.buffer = m_internalData->device.CreateBuffer(&desc);
        request.copyPending = true;
        m_internalData->pixelReadbacks.emplace_back(std::move(request));
        if (!m_internalData->activeRenderPass && !m_internalData->activeComputePass) {
            auto encoder = m_internalData->activeEncoder;
            const bool frameEncoder = static_cast<bool>(encoder);
            if (!encoder) { encoder = m_internalData->device.CreateCommandEncoder(); }
            auto& pending = m_internalData->pixelReadbacks.back();
            const uint64_t requestId = pending.id;
            encoder.CopyBufferToBuffer(source, 0, pending.buffer, 0, desc.size);
            pending.copyPending = false;
            const auto commands = encoder.Finish();
            m_internalData->queue.Submit(1, &commands);
            if (frameEncoder) { m_internalData->activeEncoder = m_internalData->device.CreateCommandEncoder(); }
            StartReadbackMappings();
            const double_t deadline = emscripten_get_now() + 30000.0;
            while (!m_internalData->failed && emscripten_get_now() < deadline) {
                m_internalData->instance.ProcessEvents();
                for (auto& result : m_internalData->pixelReadbacks) {
                    if (result.id != requestId) { continue; }
                    if (result.failed) { return false; }
                    if (result.ready) {
                        if (!result.mapped) { result.mapped = const_cast<void*>(result.buffer.GetConstMappedRange(0, desc.size)); }
                        *ppData = result.mapped;
                        return *ppData != nullptr;
                    }
                }
                // Asyncify suspends the WASM stack and returns control to the browser.
                emscripten_sleep(1);
            }
            UnMapSSBO(SSBO);
            SR_ERROR("Timed out reading WebGPU storage buffer {}", SSBO);
        }
        return false;
    }

    void WebGPUPipeline::UnMapSSBO(uint32_t SSBO) {
        for (auto& host : m_internalData->hostBuffers) {
            if (host.id == static_cast<int32_t>(SSBO)) { host.mapped = false; return; }
        }
        for (size_t i = 0; i < m_internalData->pixelReadbacks.size(); ++i) {
            auto& request = m_internalData->pixelReadbacks[i];
            if (request.ssboId == static_cast<int32_t>(SSBO)) {
                if (request.ready && !request.failed) { request.buffer.Unmap(); }
                m_internalData->pixelReadbacks.erase(m_internalData->pixelReadbacks.begin() + i);
                return;
            }
        }
    }

    void WebGPUPipeline::FlushSSBO(uint32_t SSBO, uint64_t offset, uint64_t size) {
        for (auto& host : m_internalData->hostBuffers) {
            if (host.id != static_cast<int32_t>(SSBO) || !host.mapped || offset > host.data.size()) { continue; }
            if (size == SR_UINT64_MAX || size == SR_UINT32_MAX) { size = host.data.size() - offset; }
            if (size > host.data.size() - offset) { return; }
            WriteBufferOrdered(m_internalData->SSBOs.At(host.id), offset, host.data.data() + offset, size);
            return;
        }
        for (auto& request : m_internalData->pixelReadbacks) {
            if (request.ssboId != static_cast<int32_t>(SSBO) || !request.ready || request.failed || offset > request.bytesPerRow) { continue; }
            if (size == SR_UINT64_MAX || size == SR_UINT32_MAX) { size = request.bytesPerRow - offset; }
            if (size > request.bytesPerRow - offset) { return; }
            const auto* data = static_cast<const uint8_t*>(request.mapped);
            if (!data) { return; }
            WriteBufferOrdered(m_internalData->SSBOs.At(SSBO), offset, data + offset, size);
            return;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Handles (for external use / debugging)
    // ----------------------------------------------------------------------------------------------------------------

    void WebGPUPipeline::GetShaderHandles(SR_UTILS_NS::Vector<void*>& handles) const {
        SR_TRACY_ZONE;

        handles.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_internalData->shaderPrograms.GetCapacity()); ++i) {
            if (!m_internalData->shaderPrograms.IsAlive(i)) { continue; }
            auto&& prog = m_internalData->shaderPrograms.At(i);
            void* h = prog.isCompute
                ? reinterpret_cast<void*>(prog.computePipeline.Get())
                : reinterpret_cast<void*>(prog.renderPipeline.Get());
            handles.emplace_back(h);
        }

        /// Значение по умолчанию для GetCurrentShaderHandle(), когда шейдер не привязан
        handles.emplace_back(reinterpret_cast<void*>(1));

        /// Обязательно: по этому списку ищут бинарным поиском (см. UBOManager::CollectUnused
        /// и DescriptorManager::CollectUnused), поэтому он должен быть отсортирован.
        std::sort(handles.begin(), handles.end());
    }

    void WebGPUPipeline::GetFBOHandles(SR_UTILS_NS::Vector<void*>& handles) const {
        SR_TRACY_ZONE;

        handles.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_internalData->frameBuffers.GetCapacity()); ++i) {
            if (!m_internalData->frameBuffers.IsAlive(i)) { continue; }
            auto&& fbo = m_internalData->frameBuffers.At(i);
            if (!fbo.colorAttachments.empty() && fbo.colorAttachments[0].texture) {
                handles.emplace_back(reinterpret_cast<void*>(fbo.colorAttachments[0].texture.Get()));
            }
        }

        /// Идентификатор swapchain'а: GetCurrentFBOHandle() возвращает его, когда кадровый буфер
        /// не привязан, иначе шейдеры SwapchainPass будут собираться как неиспользуемые каждый кадр
        handles.emplace_back(reinterpret_cast<void*>(1));

        /// Обязательно: по этому списку ищут бинарным поиском (см. ShaderProgramManager::CollectUnused)
        std::sort(handles.begin(), handles.end());
    }

    UsedVideoMemoryInfo WebGPUPipeline::GetUsedVideoMemoryInfo() const {
        UsedVideoMemoryInfo info{};
        if (!m_internalData) {
            return info;
        }

        auto addBuffers = [&info](auto& pool) {
            for (uint32_t i = 0; i < pool.GetCapacity(); ++i) {
                if (pool.IsAlive(i) && pool.At(i)) { info.videoMemoryUsed += pool.At(i).GetSize(); }
            }
        };
        addBuffers(m_internalData->VBOs); addBuffers(m_internalData->IBOs);
        addBuffers(m_internalData->UBOs); addBuffers(m_internalData->SSBOs);
        SR_UTILS_NS::Vector<WGPUTexture> seen;
        auto addTexture = [&info, &seen](const wgpu::Texture& texture) {
            if (!texture || std::find(seen.begin(), seen.end(), texture.Get()) != seen.end()) { return; }
            seen.emplace_back(texture.Get());
            uint32_t width = texture.GetWidth(), height = texture.GetHeight();
            uint32_t bytes = TexelBytes(texture.GetFormat());
            if (!bytes) { bytes = texture.GetFormat() == wgpu::TextureFormat::Depth32FloatStencil8 ? 8 : 4; }
            for (uint32_t mip = 0; mip < texture.GetMipLevelCount(); ++mip) {
                info.videoMemoryUsed += static_cast<uint64_t>(width) * height * bytes * texture.GetDepthOrArrayLayers() * texture.GetSampleCount();
                width = std::max(1u, width / 2); height = std::max(1u, height / 2);
            }
        };
        for (uint32_t i = 0; i < m_internalData->textures.GetCapacity(); ++i) {
            if (m_internalData->textures.IsAlive(i)) { addTexture(m_internalData->textures.At(i).texture); }
        }
        for (uint32_t i = 0; i < m_internalData->frameBuffers.GetCapacity(); ++i) {
            if (!m_internalData->frameBuffers.IsAlive(i)) { continue; }
            const auto& fbo = m_internalData->frameBuffers.At(i);
            for (const auto& attachment : fbo.colorAttachments) { addTexture(attachment.texture); addTexture(attachment.resolveTexture); }
            addTexture(fbo.depthAttachment.texture);
        }
        addTexture(m_internalData->surfaceColorTexture);
        addTexture(m_internalData->surfaceDepthTexture); addTexture(m_internalData->surfaceMultisampleTexture);
        for (const auto& request : m_internalData->pixelReadbacks) { if (request.buffer) info.videoMemoryUsed += request.buffer.GetSize(); }
        info.descriptorSetsCount = m_internalData->bindGroups.GetAliveCount();
        info.shaderProgramsCount = m_internalData->shaderPrograms.GetAliveCount();
        info.UBOsCount = m_internalData->UBOs.GetAliveCount();
        info.VBOsCount = m_internalData->VBOs.GetAliveCount();
        info.IBOsCount = m_internalData->IBOs.GetAliveCount();
        info.SSBOsCount = m_internalData->SSBOs.GetAliveCount();
        info.FBOsCount = m_internalData->frameBuffers.GetAliveCount() > 0
            ? m_internalData->frameBuffers.GetAliveCount() - 1
            : 0;
        info.texturesCount = m_internalData->textures.GetAliveCount();
        return info;
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
        m_internalData->indexSizes.erase(*id);
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
            InvalidateBufferBindGroups();
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeFBO(int32_t* id) {
        if (*id >= 0 && m_internalData->frameBuffers.IsAlive(*id)) {
            m_internalData->frameBuffers.At(*id).Destroy();
            m_internalData->frameBuffers.RemoveByIndex(*id);
            InvalidateTextureBindGroups();
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeSSBO(int32_t* id) {
        UnMapSSBO(*id);
        for (size_t i = 0; i < m_internalData->hostBuffers.size(); ++i) {
            if (m_internalData->hostBuffers[i].id == *id) {
                m_internalData->hostBuffers.erase(m_internalData->hostBuffers.begin() + i);
                break;
            }
        }
        if (*id >= 0 && m_internalData->SSBOs.IsAlive(*id)) {
            m_internalData->SSBOs.At(*id).Destroy();
            m_internalData->SSBOs.RemoveByIndex(*id);
            InvalidateBufferBindGroups();
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
            InvalidateTextureBindGroups();
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeShader(int32_t* id) {
        if (*id >= 0 && m_internalData->shaderPrograms.IsAlive(*id)) {
            m_internalData->shaderPrograms.At(*id).Destroy();
            m_internalData->shaderPrograms.RemoveByIndex(*id);
            InvalidateTextureBindGroups();
        }
        *id = SR_ID_INVALID;
        ++m_state.deletions;
        return true;
    }

    bool WebGPUPipeline::FreeTexture(int32_t* id) {
        if (*id >= 0 && m_internalData->textures.IsAlive(*id)) {
            m_internalData->textures.At(*id).Destroy();
            m_internalData->textures.RemoveByIndex(*id);
            InvalidateTextureBindGroups();
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
