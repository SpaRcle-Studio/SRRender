//
// Created by Monika on 06.05.2022.
//

#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/Framebuffer.generated.hpp>

namespace SR_GTYPES_NS {
    Framebuffer::Framebuffer() {
        SR_UTILS_NS::ResourceManager::Instance().RegisterResource(this);
    }

    Framebuffer::~Framebuffer() {
        for (auto&& frameBuffer : m_frameBuffer) {
            SRAssert(frameBuffer == SR_ID_INVALID);
        }

    #ifdef SR_DEBUG
        for (auto&& [textures, format] : m_colors) {
            for (auto&& texture : textures) {
                SRAssert(texture == SR_ID_INVALID);
            }
            SR_UNUSED_VARIABLE(format);
        }

        for (auto&& texture : m_depth.texture) {
            SRAssert(texture == SR_ID_INVALID);
        }
    #endif
    }

    Framebuffer::Ptr Framebuffer::Create(uint32_t images, const SR_MATH_NS::IVector2 &size) {
        std::vector<ImageFormat> colors;

        for (uint32_t i = 0; i < images; ++i) {
            colors.emplace_back(ImageFormat::RGBA8_UNORM);
        }

        return Create(colors, ImageFormat::Auto, size);
    }

    Framebuffer::Ptr Framebuffer::Create(const std::vector<ImageFormat> &colors, ImageFormat depth, const SR_MATH_NS::IVector2 &size) {
        return Create(colors, depth, size, 0);
    }

    Framebuffer::Ptr Framebuffer::Create(const std::vector<ImageFormat>& colors, ImageFormat depth, const SR_MATH_NS::IVector2& size, uint8_t samples) {
        return Create(colors, depth, size, samples, 1);
    }

    Framebuffer::Ptr Framebuffer::Create(const std::vector<ImageFormat> &colors, ImageFormat depth, const SR_MATH_NS::IVector2 &size, uint8_t samples, uint32_t layersCount) {
        return Create(colors, depth, size, samples, 1, ImageAspect::DepthStencil);
    }

    Framebuffer::Ptr Framebuffer::Create(const std::vector<ImageFormat> &colors, ImageFormat depth, const SR_MATH_NS::IVector2 &size, uint8_t samples, uint32_t layersCount, ImageAspect depthAspect) {
        auto&& pFBO = Framebuffer::MakeShared<Framebuffer>();

        SRAssert(!size.HasZero() && !size.HasNegative());

        if (depth == ImageFormat::Unknown) {
            SRHalt("Framebuffer::Create() : depth format is unknown!");
            return nullptr;
        }

        pFBO->SetSize(size);
        pFBO->m_depth.format = depth;
        pFBO->m_depth.aspect = depthAspect;
        pFBO->m_sampleCount = samples;
        pFBO->m_layersCount = layersCount;

        for (auto&& color : colors) {
            ColorLayer layer;
            layer.format = color;
            pFBO->m_colors.emplace_back(layer);
        }

        return pFBO;
    }

    Framebuffer::Ptr Framebuffer::Create(const std::vector<ImageFormat> &colors, ImageFormat depth) {
        return Create(colors, depth, SR_MATH_NS::IVector2(0, 0));
    }

    bool Framebuffer::Bind() {
        if (m_hasErrors) {
            return false;
        }

        if (m_dirty) {
            return false;
        }

        GetPipeline()->BindFrameBuffer(this);
        GetPipeline()->SetCurrentFrameBuffer(this);

        return true;
    }

    bool Framebuffer::Update() {
        if (!m_dirty) {
            return true;
        }

        if (m_hasErrors) {
            return false;
        }

        m_wasRendered = false;

        if (m_size.HasZero() || m_size.HasNegative()) {
            SR_ERROR("FrameBuffer::Update() : incorrect framebuffer size!");
            m_hasErrors = true;
            return false;
        }

        if (m_sampleCount == 0) {
            m_currentSampleCount = GetPipeline()->GetSamplesCount();
        }
        else {
            m_currentSampleCount = m_sampleCount;
        }

        /// если устройство не поддерживает, то не будем пытаться использовать
        if (!GetPipeline()->IsMultiSamplingSupported()) {
            m_currentSampleCount = 1;
        }
        else {
            m_currentSampleCount = SR_MIN(m_currentSampleCount, GetPipeline()->GetSupportedSamples());
        }

        if (m_frameBuffer.empty()) {
            if (m_forEachSwapchainImage) {
                m_frameBuffer.resize(GetPipeline()->GetSwapchainImagesCount());
            }
            else {
                m_frameBuffer.resize(1);
            }
            std::ranges::fill(m_frameBuffer, SR_ID_INVALID);
        }

        SRFrameBufferCreateInfo createInfo;
        createInfo.size = m_size;
        createInfo.pFBO = &m_frameBuffer;
        createInfo.pDepth = &m_depth;
        createInfo.colors = &m_colors;
        createInfo.sampleCount = m_currentSampleCount;
        createInfo.layersCount = m_layersCount;
        createInfo.arrayLayersCount = m_arrayLayersCount;
        createInfo.features = m_features;

        if (!GetPipeline()->AllocateFrameBuffer(createInfo)) {
            SR_ERROR("FrameBuffer::Update() : failed to allocate frame buffer!");
            m_hasErrors = true;
            return false;
        }

        m_hasErrors = false;
        m_dirty = false;

        GetPipeline()->SetDirty(true);

        return true;
    }

    void Framebuffer::FreeVMemory() {
        for (int32_t& frameBuffer : m_frameBuffer) {
            if (frameBuffer != SR_ID_INVALID) {
                SRVerifyFalse(!GetPipeline()->FreeFBO(&frameBuffer));
                frameBuffer = SR_ID_INVALID;
            }
        }

        for (auto&& texture : m_depth.texture) {
            if (texture != SR_ID_INVALID) {
                SRVerifyFalse(!GetPipeline()->FreeTexture(&texture));
            }
        }

        for (auto&& textures : m_depth.subLayers) {
            for (auto&& texture : textures) {
                if (texture != SR_ID_INVALID) {
                    SRVerifyFalse(!GetPipeline()->FreeTexture(&texture));
                }
            }
        }

        for (auto&& [textures, format] : m_colors) {
            for (auto&& texture : textures) {
                if (texture != SR_ID_INVALID) {
                    SRVerifyFalse(!GetPipeline()->FreeTexture(&texture));
                }
            }
        }

        IGraphicsResource::FreeVMemory();
    }

    void Framebuffer::SetSize(const SR_MATH_NS::IVector2 &size) {
        m_size = size;
        SetDirty();
    }

    bool Framebuffer::BeginCmdBuffer(uint32_t frame, const ClearColors& clearColors, std::optional<float_t> depth) {
        if (IsDepthEnabled() && !depth.has_value()) {
            SR_ERROR("Framebuffer::BeginCmdBuffer() : depth is not set!");
            depth = 1.0f;
        }

        GetPipeline()->ClearBuffers(clearColors, depth);

        if (!GetPipeline()->BeginCmdBuffer()) {
            return false;
        }

        SR_NOOP;

        return true;
    }

    bool Framebuffer::BeginCmdBuffer(uint32_t frame) {
        GetPipeline()->ClearBuffers();

        if (!GetPipeline()->BeginCmdBuffer()) {
            return false;
        }

        SR_NOOP;

        return true;
    }

    bool Framebuffer::BeginRender() {
        if (!GetPipeline()->BeginRender()) {
            return false;
        }

        m_wasRendered = true;

        return true;
    }

    void Framebuffer::EndRender() {
        GetPipeline()->EndRender();
    }

    void Framebuffer::EndCmdBuffer() {
        GetPipeline()->EndCmdBuffer();
    }

    bool Framebuffer::IsValid() const {
        return !m_hasErrors && !IsDirty() && GetId() != SR_ID_INVALID;
    }

    int32_t Framebuffer::GetId() const {
        if (m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return SR_ID_INVALID;
        }

        if (m_dirty) {
            return SR_ID_INVALID;
        }

        if (m_frameBuffer.empty()) {
            return SR_ID_INVALID;
        }

        return m_frameBuffer[SR_MIN(GetPipeline()->GetCurrentFrameIndex(), static_cast<uint32_t>(m_frameBuffer.size() - 1))];
    }

    uint64_t Framebuffer::GetFileHash() const {
        return 0;
    }

    int32_t Framebuffer::GetColorTexture(uint32_t layer, uint8_t frame) {
        SR_TRACY_ZONE;

        if (m_dirty) {
            return SR_ID_INVALID;
        }

        if (layer >= m_colors.size() || m_hasErrors) {
            return SR_ID_INVALID;
        }

        auto&& frames = m_colors.at(layer).texture;
        if (frames.empty()) {
            return SR_ID_INVALID;
        }

        return frames[SR_MIN(frame, static_cast<uint8_t>(frames.size() - 1))];
    }

    bool Framebuffer::BeginCmdBuffer(uint32_t frame, const SR_MATH_NS::FColor &clearColor, float_t depth) {
        return BeginCmdBuffer(frame, Framebuffer::ClearColors{ clearColor }, depth);
    }

    uint32_t Framebuffer::GetWidth() const {
        return m_size.x;
    }

    uint32_t Framebuffer::GetHeight() const {
        return m_size.y;
    }

    void Framebuffer::SetDepthEnabled(bool depthEnabled) {
        m_depthEnabled = depthEnabled;
        m_dirty = true;
    }

    void Framebuffer::SetSampleCount(uint8_t samples) {
        m_sampleCount = samples;
        m_dirty = true;
    }

    int32_t Framebuffer::GetDepthTexture(int32_t layer, uint8_t frame) {
        if (!m_depthEnabled) {
            return SR_ID_INVALID;
        }

        if (m_dirty) {
            return SR_ID_INVALID;
        }

        if (layer < 0) {
            if (m_depth.texture.empty()) {
                return SR_ID_INVALID;
            }
            return m_depth.texture[SR_MIN(frame, static_cast<uint8_t>(m_depth.texture.size() - 1))];
        }

        if (layer >= m_depth.subLayers.size()) {
            SRHalt0();
            return SR_ID_INVALID;
        }

        auto&& frames = m_depth.subLayers[layer];
        if (frames.empty()) {
            return SR_ID_INVALID;
        }
        return frames[SR_MIN(frame, static_cast<uint8_t>(frames.size() - 1))];
    }

    uint8_t Framebuffer::GetSamplesCount() const {
        SRAssert(m_currentSampleCount >= 1 && m_currentSampleCount <= 64);
        return m_currentSampleCount;
    }

    void Framebuffer::SetDirty() {
        m_dirty = true;

        if (GetPipeline()) {
            GetPipeline()->SetDirty(true);
        }
    }

    void Framebuffer::SetLayersCount(uint32_t layersCount) {
        m_layersCount = layersCount;
        m_dirty = true;
    }

    void Framebuffer::SetArrayLayersCount(uint32_t arrayLayersCount) {
        m_arrayLayersCount = arrayLayersCount;
        m_dirty = true;
    }

    void Framebuffer::SetDepthAspect(ImageAspect depthAspect) {
        m_depth.aspect = depthAspect;
        m_dirty = true;
    }

    void Framebuffer::SetViewportScissor() {
        GetPipeline()->SetViewport(m_size.x, m_size.y);
        GetPipeline()->SetScissor(m_size.x, m_size.y);
    }

    void Framebuffer::SetFeatures(const FrameBufferFeatures& features) {
        m_features = features;
        m_dirty = true;
    }
}