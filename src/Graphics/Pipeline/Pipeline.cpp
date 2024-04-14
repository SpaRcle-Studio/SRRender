//
// Created by Monika on 07.12.2022.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Overlay/Overlay.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Render/RenderContext.h>

#include <Utils/Types/Time.h>
#include <Utils/Common/Features.h>

#ifdef SR_DEBUG
    #define SR_PIPELINE_RENDER_GUARD(ret)                   \
        if (!m_isRenderState) {                             \
            SRHaltOnce("Missing call \"BeginRender()\"!");  \
            return ret;                                     \
        }                                                   \

#else
    #define SR_PIPELINE_RENDER_GUARD(ret)
#endif

namespace SR_GRAPH_NS {
    Pipeline::Pipeline(const RenderContextPtr& pContext)
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Manually)
        , m_renderContext(pContext)
    { }

    Pipeline::~Pipeline() {
        SRAssert(m_overlays.empty());
    }

    void Pipeline::DrawFrame() {
        SR_TRACY_ZONE;

        ++m_state.operations;
        m_previousState = m_state;
        m_state = PipelineState();

        ++m_frames;

        auto&& now = SR_HTYPES_NS::Time::ClockT::now();

        if (!m_lastSecond.has_value()) {
            m_lastSecond = now;
        }

        if (now - m_lastSecond.value() >= std::chrono::seconds(1)) {
            m_framesPerSecond = m_frames;
            m_frames = 0;
            m_lastSecond = now;
        }
    }

    bool Pipeline::BeginRender() {
        ++m_state.operations;

        if (!m_isCmdState) {
            SRHalt("Pipeline::BeginRender() : missing call \"BeginCmdBuffer\"!");
            return false;
        }

        if (m_isRenderState) {
            SRHalt("Pipeline::BeginRender() : missing call \"EndRender\"!");
            return false;
        }

        m_isRenderState = true;

        return true;
    }

    void Pipeline::EndRender() {
        SR_PIPELINE_RENDER_GUARD(void())
        m_isRenderState = false;
        ++m_state.operations;
    }

    void Pipeline::DrawIndices(uint32_t count) {
        SR_PIPELINE_RENDER_GUARD(void())
        ++m_state.operations;
        ++m_state.drawCalls;
        m_state.vertices += count;
    }

    void Pipeline::Draw(uint32_t count) {
        SR_PIPELINE_RENDER_GUARD(void())
        ++m_state.operations;
        ++m_state.drawCalls;
        m_state.vertices += count;
    }

    bool Pipeline::BeginCmdBuffer() {
        ++m_state.operations;

        if (m_isRenderState) {
            SRHalt("Pipeline::BeginCmdBuffer() : is render state now!");
            return false;
        }

        if (m_isCmdState) {
            SRHalt("Pipeline::BeginCmdBuffer() : missing call \"EndCmdBuffer\"!");
            return false;
        }

        m_isCmdState = true;
        return true;
    }

    void Pipeline::EndCmdBuffer() {
        ++m_state.operations;
        if (!m_isCmdState) {
            SRHalt("Pipeline::EndCmdBuffer() : missing call \"BeginCmdBuffer\"!");
        }
        m_isCmdState = false;
    }

    void Pipeline::ClearFrameBuffersQueue() {
        m_fboQueue.Clear();
        ++m_state.operations;
    }

    void Pipeline::ClearBuffers() {
        ++m_state.operations;
    }

    void Pipeline::ClearBuffers(float_t r, float_t g, float_t b, float_t a, float_t depth, uint8_t colorCount) {
        ++m_state.operations;
    }

    void Pipeline::ClearBuffers(const ClearColors& clearColors, std::optional<float_t> depth) {
        ++m_state.operations;
    }

    void Pipeline::PrepareFrame() {
        ++m_state.operations;
        UpdateMultiSampling();
    }

    void Pipeline::BindVBO(uint32_t VBO) {
        ++m_state.operations;
    }

    void Pipeline::BindIBO(uint32_t IBO) {
        ++m_state.operations;
    }

    void Pipeline::BindUBO(uint32_t UBO) {
        ++m_state.operations;
        m_state.UBOId = static_cast<int32_t>(UBO);
    }

    void Pipeline::UpdateUBO(uint32_t UBO, void* pData, uint64_t size) {
        ++m_state.operations;
        m_state.transferredMemory += size;
        ++m_state.transferredCount;
    }

    void Pipeline::PushConstants(void* pData, uint64_t size) {
        ++m_state.operations;
        m_state.transferredMemory += size;
        ++m_state.transferredCount;
    }

    void Pipeline::BindTexture(uint8_t activeTexture, uint32_t textureId) {
        ++m_state.operations;
        ++m_state.usedTextures;
    }

    void Pipeline::BindAttachment(uint8_t activeTexture, uint32_t textureId) {
        ++m_state.operations;
        ++m_state.usedTextures;
    }

    void Pipeline::BindDescriptorSet(uint32_t descriptorSet) {
        ++m_state.operations;
        m_state.descriptorSetId = static_cast<int32_t>(descriptorSet);
    }

    void Pipeline::UseShader(uint32_t shaderProgram) {
        ++m_state.operations;
        ++m_state.usedShaders;
        m_state.shaderId = static_cast<int32_t>(shaderProgram);
    }

    void Pipeline::OnResize(const SR_MATH_NS::UVector2& size) {

    }

    void Pipeline::DestroyOverlay() {
        for (auto&& [type, pOverlay] : m_overlays) {
            pOverlay.AutoFree([](auto&& pData) {
                pData->Destroy();
                delete pData;
            });
        }

        m_overlays.clear();
    }

    void Pipeline::OnMultiSampleChanged() {
        SR_INFO("Pipeline::OnMultiSampleChanged() : samples count was changed to {}", GetSamplesCount());
        SetDirty(true);
        m_renderContext->OnMultiSampleChanged();
    }

    uint8_t Pipeline::GetSamplesCount() const {
        SRAssert(m_currentSampleCount >= 1 && m_currentSampleCount <= 64);
        return m_currentSampleCount;
    }

    void Pipeline::UpdateMultiSampling() {
        const bool isMultiSampleSupported = m_isMultiSampleSupported;

        m_isMultiSampleSupported = true;

        if (m_supportedSampleCount <= 1) {
            m_isMultiSampleSupported = false;
        }

        const bool multiSampleSupportsChanged = isMultiSampleSupported != m_isMultiSampleSupported;

        if (m_newSampleCount.has_value() || multiSampleSupportsChanged) {
            const uint8_t oldSampleCount = m_currentSampleCount;

            if (multiSampleSupportsChanged) {
                if (!IsMultiSamplingSupported()) {
                    m_currentSampleCount = 1;
                }
                else if (m_newSampleCount.has_value()) {
                    m_currentSampleCount = m_newSampleCount.value();
                }
                else {
                    m_currentSampleCount = m_requiredSampleCount;
                }
            }
            else if (m_newSampleCount.has_value()) {
                m_currentSampleCount = m_newSampleCount.value();
            }

            m_currentSampleCount = SR_MIN(m_currentSampleCount, m_supportedSampleCount);

            if (oldSampleCount != m_currentSampleCount) {
                OnMultiSampleChanged();
            }

            m_newSampleCount = std::nullopt;
        }
    }

    void Pipeline::SetBuildIteration(uint8_t iteration) {
        m_state.buildIteration = iteration;
        ++m_state.operations;
    }

    bool Pipeline::BeginDrawOverlay(OverlayType overlayType) {
        ++m_state.operations;
        ++m_state.drawCalls;

        auto&& pIt = m_overlays.find(overlayType);
        if (pIt == m_overlays.end() || !pIt->second) {
            return false;
        }

        return pIt->second->BeginDraw();
    }

    void Pipeline::EndDrawOverlay(OverlayType overlayType) {
        ++m_state.operations;

        auto&& pIt = m_overlays.find(overlayType);
        if (pIt == m_overlays.end() || !pIt->second) {
            return;
        }

        pIt->second->EndDraw();
    }

    bool Pipeline::InitOverlay() {
        return true;
    }

    void Pipeline::ReCreateOverlay() {
        ++m_state.operations;

        for (auto&& [type, pOverlay] : m_overlays) {
            if (pOverlay->ReCreate()) {
                continue;
            }

            PipelineError("Pipeline::ReCreateOverlay() : failed to re-create \"" + pOverlay->GetName() + "\" overlay!");
        }
    }

    void Pipeline::SetOverlaySurfaceDirty() {
        ++m_state.operations;

        for (auto&& [type, pOverlay] : m_overlays) {
            pOverlay->SetSurfaceDirty();
        }
    }

    void Pipeline::SetDirty(bool dirty) {
        m_dirty = dirty;

        if (!m_dirty) {
            m_buildState = m_state;
        }
    }

    bool Pipeline::PreInit(const PipelinePreInitInfo& info) {
        m_requiredSampleCount = info.samplesCount;

        m_preInitInfo = info;
        SRAssert2(m_requiredSampleCount >= 1 && m_requiredSampleCount <= 64, "Sample count must be greater 0 and less or equals 64!");

        return true;
    }

    const SR_HTYPES_NS::SharedPtr<Overlay>& Pipeline::GetOverlay(OverlayType overlayType) const {
        SR_TRACY_ZONE;
        auto&& pIt = m_overlays.find(overlayType);
        if (pIt == m_overlays.end() || !pIt->second) {
            static SR_HTYPES_NS::SharedPtr<Overlay> pEmpty;
            return pEmpty;
        }
        return pIt->second;
    }

    void Pipeline::PipelineError(const std::string& msg) const {
        ++m_errorsCount;
        SR_ERROR(msg);
    }

    void Pipeline::ResetDescriptorSet() {
        ++m_state.operations;
        m_state.descriptorSetId = SR_ID_INVALID;
    }

    void Pipeline::ResetLastShader() {
        ++m_state.operations;
    }

    void Pipeline::UnUseShader() {
        ++m_state.operations;
        m_state.shaderId = SR_ID_INVALID;
        m_state.pShader = nullptr;
    }

    void Pipeline::UpdateDescriptorSets(uint32_t descriptorSet, const SRDescriptorUpdateInfos& updateInfo) {
        ++m_state.operations;
    }

    void Pipeline::SetOverlayEnabled(OverlayType overlayType, bool enabled) {
        ++m_state.operations;

        auto&& pIt = m_overlays.find(overlayType);
        if (pIt == m_overlays.end() || !pIt->second) {
            return;
        }
        return pIt->second->SetEnabled(enabled);
    }

    void Pipeline::SetCurrentFrameBuffer(Pipeline::FramebufferPtr pFrameBuffer) {
        ++m_state.operations;
        m_state.pFrameBuffer = pFrameBuffer;

        if (m_state.pFrameBuffer) {
            SRAssert(!m_state.pFrameBuffer->IsDirty());
        }
        else {
            SetFrameBufferLayer(0);
        }
    }

    void Pipeline::BindFrameBuffer(Pipeline::FramebufferPtr pFBO) {
        ++m_state.operations;
        m_state.pFrameBuffer = pFBO;
    }

    void* Pipeline::GetOverlayTextureDescriptorSet(uint32_t textureId, OverlayType overlayType) const {
        ++m_state.operations;
        auto&& pIt = m_overlays.find(overlayType);
        if (pIt == m_overlays.end() || !pIt->second) {
            return nullptr;
        }
        return pIt->second->GetTextureDescriptorSet(textureId);
    }

    void Pipeline::SetSampleCount(uint8_t count) {
        ++m_state.operations;
        m_newSampleCount = m_requiredSampleCount = count;
    }

    void Pipeline::ClearDepthBuffer(float_t depth) {
        ++m_state.operations;
    }

    void Pipeline::ClearColorBuffer(const ClearColors& clearColors) {
        ++m_state.operations;
    }

    bool Pipeline::Init() {
        return true;
    }

    bool Pipeline::IsMultiSamplingSupported() const noexcept {
        if (!SR_UTILS_NS::Features::Instance().Enabled("Multisampling", true)) {
            return false;
        }

        return m_isMultiSampleSupported;
    }

    void Pipeline::SwitchWindow(const Pipeline::WindowPtr& pWindow) {
        ++m_state.operations;
        m_window = pWindow;
    }
}
