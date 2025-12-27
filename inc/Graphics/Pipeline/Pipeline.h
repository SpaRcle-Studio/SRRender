//
// Created by Monika on 07.12.2022.
//

#ifndef SR_ENGINE_PIPELINE_H
#define SR_ENGINE_PIPELINE_H

#include <Graphics/Pipeline/PipelineState.h>
#include <Graphics/Pipeline/FrameBufferQueue.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Overlay/OverlayType.h>
#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Memory/SSBOUsage.h>
#include <Graphics/Utils/FrameBufferAccessMode.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Types/SafePointer.h>
#include <Utils/Types/PoolSet.h>

namespace SR_GTYPES_NS {
    class Shader;
    class Camera;
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class RenderStrategy;
    class RenderContext;
    class Overlay;
    class Window;

    struct UsedVideoMemoryInfo {
        uint64_t videoMemoryUsed = 0;
        uint64_t videoMemoryHeaps = 0;
        uint32_t descriptorSetsCount = 0;
        uint32_t shaderProgramsCount = 0;
        uint32_t UBOsCount = 0;
        uint32_t VBOsCount = 0;
        uint32_t IBOsCount = 0;
        uint32_t SSBOsCount = 0;
        uint32_t FBOsCount = 0;
        uint32_t texturesCount = 0;
    };

    class Pipeline : public SR_HTYPES_NS::SharedPtr<Pipeline> {
    public:
        using Super = SR_HTYPES_NS::SharedPtr<Pipeline>;
        using ClearColors = std::vector<SR_MATH_NS::FColor>;
        using Ptr = SR_HTYPES_NS::SharedPtr<Pipeline>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using FramebufferPtr = SR_GTYPES_NS::Framebuffer*;
        using RenderContextPtr = SR_HTYPES_NS::SafePtr<SR_GRAPH_NS::RenderContext>;
        using WindowPtr = SR_HTYPES_NS::SharedPtr<Window>;
        using ShaderProgram = int32_t;
    public:
        explicit Pipeline(const RenderContextPtr& pContext);
        virtual ~Pipeline();

        /// ---------------------------------------- Инициализация рендера ---------------------------------------------

        /// Предназначено для инициализации всех структур и классов
        virtual bool PreInit(const PipelinePreInitInfo& info);

        /// Подключаем окно и настраиваем взаимодействие рендера с ним
        virtual bool Init();

        /// Профайлинг и прочие пост-штучки
        virtual bool PostInit() { return true; }

        /// Чистим все данные графического конфейера и де-инициализируем его
        virtual bool Destroy() { return true; }

        SR_NODISCARD virtual PipelineType GetType() const noexcept = 0;

        /// --------------------------------------- Главные методы рендера ---------------------------------------------

        /// Вызывается перед началом рендера, подготовка к рендеру
        virtual void PrepareFrame();

        /// Вызывается в начале построения сцены рендера, чистит очередь рендера.
        virtual void ClearFrameBuffersQueue();
        virtual void ResetSubmitQueue();

        /// Отрисовка кадра на экран
        /// После вызова функции кадр считается законченным и PipelineState очищается
        virtual void DrawFrame();

        /// Начало записи в буфер команд. Разделение необходимо некоторым графическим API
        virtual bool BeginCmdBuffer();

        /// Конец записи в буфер команд. Разделение необходимо некоторым графическим API
        virtual void EndCmdBuffer();

        /// Начало рендера в кадровый буфер или в SwapChain
        virtual bool BeginRender();

        /// Начало вычислений в Compute Shader
        virtual bool BeginCompute();

        /// Конец вычислений в Compute Shader
        virtual void EndCompute();

        /// Обязательно нужно вызвать после успешного вызова BeginRender
        virtual void EndRender();

        virtual void SetViewport(int32_t width = -1, int32_t height = -1) { ++m_state.operations; };
        virtual void SetScissor(int32_t width = -1, int32_t height = -1) { ++m_state.operations; };

        virtual void SwitchWindow(const WindowPtr& pWindow);

        virtual void WaitDeviceIdle();
        virtual void WaitComputeIdle();
        virtual void WaitRenderIdle();

        virtual void OnFrameBuildEnd();
        virtual void OnFrameBuildBegin();

        /// ------------------------------------------ Работа с Overlay ------------------------------------------------

        virtual bool InitOverlay();
        virtual void DestroyOverlay();
        virtual void ReCreateOverlay();
        virtual void SetOverlaySurfaceDirty();

        virtual const SR_HTYPES_NS::SharedPtr<Overlay>& GetOverlay(OverlayType overlayType) const;
        virtual void PrepareOverlay(OverlayType overlayType);
        virtual bool BeginDrawOverlay(OverlayType overlayType);
        virtual void EndDrawOverlay(OverlayType overlayType);
        virtual bool HasActiveOverlay() const;

        virtual void SetOverlayEnabled(OverlayType overlayType, bool enabled);

        /// --------------------------------------- Вспомогательные методы ---------------------------------------------

        SR_NODISCARD virtual std::string GetVendor() const { return "None"; }
        SR_NODISCARD virtual std::string GetRenderer() const { return "None"; }
        SR_NODISCARD virtual std::string GetVersion() const { return "None"; }

        SR_NODISCARD RenderContextPtr GetRenderContext() const noexcept { return m_renderContext; }
        SR_NODISCARD WindowPtr GetWindow() const { return m_window; }
        SR_NODISCARD ShaderPtr GetCurrentShader() const { ++m_state.operations; return m_state.pShader; }
        SR_NODISCARD FramebufferPtr GetCurrentFrameBuffer() const noexcept { ++m_state.operations; return m_state.pFrameBuffer; }
        SR_NODISCARD int32_t GetCurrentShaderId() const { ++m_state.operations; return m_state.shaderId; }
        SR_NODISCARD int32_t GetCurrentFrameBufferId() const noexcept { ++m_state.operations; return m_state.frameBufferId; }
        SR_NODISCARD int32_t GetCurrentUBO() const { ++m_state.operations; return m_state.UBOId; }
        SR_NODISCARD int32_t GetCurrentDescriptorSet() const noexcept { ++m_state.operations; return m_state.descriptorSetId; }
        SR_NODISCARD uint32_t GetCurrentFrameBufferLayer() const noexcept { ++m_state.operations; return m_state.frameBufferLayer; }
        SR_NODISCARD bool IsDirty() const noexcept { ++m_state.operations; return m_dirty; }
        SR_NODISCARD FrameBufferQueue& GetQueue() noexcept { ++m_state.operations; return m_fboQueue; }
        SR_NODISCARD RenderStrategy* GetCurrentRenderStrategy() const noexcept { ++m_state.operations; return m_state.pRenderStrategy; }
        SR_NODISCARD SR_UTILS_NS::StringAtom GetRenderStageId() const { return m_renderStageId; }
        SR_NODISCARD SR_GTYPES_NS::Camera* GetCurrentCamera() const { ++m_state.operations; return m_state.pCamera; }

        SR_NODISCARD virtual uint8_t GetCurrentFrameIndex() const { return 0; }
        SR_NODISCARD virtual uint8_t GetCurrentImageIndex() const { return 0; }
        SR_NODISCARD virtual void* GetCurrentShaderHandle() const { return nullptr; }
        SR_NODISCARD virtual void* GetCurrentFBOHandle() const { return nullptr; }
        SR_NODISCARD virtual void GetFBOHandles(std::vector<void*>& handles) const { }
        SR_NODISCARD virtual void GetShaderHandles(std::vector<void*>& handles) const { }
        SR_NODISCARD virtual uint8_t GetFrameBufferSampleCount() const { ++m_state.operations; return 0; }
        SR_NODISCARD virtual uint8_t GetBuildIterationsCount() const noexcept { ++m_state.operations; return 0; }
        SR_NODISCARD virtual uint8_t GetSupportedSamples() const noexcept { return m_supportedSampleCount; }
        SR_NODISCARD virtual bool IsShaderConstantSupport() const { ++m_state.operations; return false; }
        SR_NODISCARD virtual bool IsShaderViewportIndexLayerSupported() const { ++m_state.operations; return false; }
        SR_NODISCARD virtual SR_MATH_NS::FColor GetPixelColor(uint32_t textureId, uint32_t x, uint32_t y) { return SR_MATH_NS::FColor(0.f); }
        SR_NODISCARD virtual uint16_t GetSwapchainImagesCount() const { return 0; }
        SR_NODISCARD virtual uint16_t GetMaxFramesInFlight() const { return 3; }

        SR_FORCE_INLINE void SetCurrentShader(ShaderPtr pShader) { ++m_state.operations; m_state.pShader = pShader; }
        SR_FORCE_INLINE void SetCurrentShaderId(int32_t id) { ++m_state.operations; m_state.shaderId = id; }
        SR_FORCE_INLINE void SetCurrentCamera(SR_GTYPES_NS::Camera* pCamera) { ++m_state.operations; m_state.pCamera = pCamera; }

        virtual void SetSwapchainImagesCount(uint16_t count) { }
        virtual void SetRenderStageId(SR_UTILS_NS::StringAtom id) { m_renderStageId = id; }
        virtual void SetCurrentFrameBufferLayer(uint32_t layer) { ++m_state.operations; m_state.frameBufferLayer = layer; }
        virtual void SetCurrentFrameBuffer(FramebufferPtr pFrameBuffer);
        virtual void SetCurrentRenderStrategy(RenderStrategy* pStrategy) { ++m_state.operations; m_state.pRenderStrategy = pStrategy; }
        virtual void SetFrameBufferAccessMode(FrameBufferAccessMode mode) { ++m_state.operations; }

        virtual void* GetOverlayTextureDescriptorSet(uint32_t textureId, OverlayType overlayType) const;

        virtual void PipelineError(const std::string& msg) const;

        virtual void OnResize(const SR_MATH_NS::UVector2& size);

        /// Очистка кадрового буфера цветом. Если у буфера несколько attachment'ов,
        /// то в colorCount нужно задать их количество
        virtual void ClearBuffers();
        virtual void ClearBuffers(float_t r, float_t g, float_t b, float_t a, float_t depth, uint8_t colorCount);
        virtual void ClearBuffers(const ClearColors& clearColors, std::optional<float_t> depth);

        virtual void ClearDepthBuffer(float_t depth);
        virtual void ClearColorBuffer(const ClearColors& clearColors);

        /// Clear depth attachment inside active RenderPass using vkCmdClearAttachments
        /// This is the correct way to clear attachments during rendering without ending RenderPass
        virtual void ClearDepthAttachment(float_t depth);

        /// Устанавливает состояние графического конвейера.
        /// Если грязный, то будет перестроена сцена
        /// Если чистый, то считаем, что постороение сцены завершено
        virtual void SetDirty(bool dirty);

        /// ---------------------------------------- Мультисемплинг и VSync --------------------------------------------

        virtual void OnMultiSampleChanged();
        virtual void UpdateMultiSampling();
        virtual void SetSampleCount(uint8_t count);

        virtual void SetVSyncEnabled(bool enabled) { }

        SR_NODISCARD uint32_t GetFramesPerSecond() const noexcept { return m_framesPerSecond; }
        SR_NODISCARD const PipelineState& GetPreviousState() const { return m_previousState; }
        SR_NODISCARD const PipelineState& GetBuildState(uint8_t frameIndex) const;
        SR_NODISCARD const PipelineState& GetState() const { return m_state; }
        SR_NODISCARD uint8_t GetSamplesCount() const;
        SR_NODISCARD bool IsMultiSamplingSupported() const noexcept;
        SR_NODISCARD virtual bool IsVSyncEnabled() const { return false; }
        /// Изменился ли текущий шейдер после UseShader. Даже если был вызван UnUseShader. Низкоуровневая проверка.
        SR_NODISCARD bool IsShaderChanged() const noexcept { return m_isShaderChanged; }
        SR_NODISCARD bool IsRenderState() const noexcept { return m_isRenderState; }
        SR_NODISCARD bool IsFBOQueueValid() const noexcept;

        /// ------------------------------------------ Работа с памятью ------------------------------------------------

        SR_NODISCARD virtual UsedVideoMemoryInfo GetUsedVideoMemoryInfo() const { return UsedVideoMemoryInfo(); }

        SR_NODISCARD virtual int32_t AllocateVBO(const void* pVertices, Vertices::VertexType type, size_t count) { return SR_ID_INVALID; }
        /// Продвинутая версия AllocateVBO, может сама выполнить преобразование типа памяти базовых вершин к нужному выравниванию.
        SR_NODISCARD virtual int32_t AllocateVBO(const SR_UTILS_NS::Vertex* pVertices, Vertices::VertexType type, size_t count);
        SR_NODISCARD virtual int32_t AllocateIBO(const void* pIndices, uint32_t indexSize, size_t count, int32_t VBO) { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t AllocateUBO(uint32_t uboSize) { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t AllocateSSBO(uint32_t ssboSize, SSBOUsage usage) { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t AllocDescriptorSet(const std::vector<DescriptorType>& types) { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t AllocateShaderProgram(const SRShaderCreateInfo& createInfo, int32_t fbo) { return SR_ID_INVALID; };
        SR_NODISCARD virtual int32_t AllocateTexture(const SRTextureCreateInfo& createInfo) { return SR_ID_INVALID; };
        SR_NODISCARD virtual int32_t AllocateFrameBuffer(const SRFrameBufferCreateInfo& createInfo) { return SR_ID_INVALID; };
        SR_NODISCARD virtual int32_t AllocateCubeMap(const SRCubeMapCreateInfo& createInfo) { return SR_ID_INVALID; };

        virtual bool FreeDescriptorSet(int32_t* id) { return false; }
        virtual bool FreeVBO(int32_t* id) { return false; }
        virtual bool FreeIBO(int32_t* id) { return false; }
        virtual bool FreeUBO(int32_t* id) { return false; }
        virtual bool FreeFBO(int32_t* id) { return false; }
        virtual bool FreeSSBO(int32_t* id) { return false; }
        virtual bool FreeCubeMap(int32_t* id) { return false; }
        virtual bool FreeShader(int32_t* id) { return false; }
        virtual bool FreeTexture(int32_t* id) { return false; }

        virtual bool IsSamplerValid(int32_t id) const { return false; }

        /// ------------------------------------------ Вызовы отрисовки ------------------------------------------------

        /// Отрисовка вершин по индексам
        virtual void DrawIndices(uint32_t count);

        /// Обычная отрисовка вершин
        virtual void Draw(uint32_t count);

        /// -------------------------------------------- Вычисления ----------------------------------------------------

        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        /// --------------------------------------------- Биндинги -----------------------------------------------------

        virtual void UseShader(uint32_t shaderProgram);
        virtual void UnUseShader();

        virtual void BindFrameBuffer(FramebufferPtr pFBO);

        /// Vertex Buffer Object - биндими для рендера вершин
        virtual void BindVBO(uint32_t VBO);

        /// Index Buffer Object - биндим для рендера вершин по индексам
        virtual void BindIBO(uint32_t IBO);

        /// Uniform Buffer Object - обеспечивает привязку для передачм данных в шейдеры
        virtual void BindUBO(uint32_t UBO);

        /// Shader Storage Buffer Object - обеспечивает привязку для передачм данных в шейдеры
        virtual void BindSSBO(uint32_t SSBO);

        virtual bool MapSSBO(uint32_t SSBO, void** ppData) { return false; }
        virtual void UnMapSSBO(uint32_t SSBO) {}

        virtual void FlushSSBO(uint32_t SSBO, uint64_t offset, uint64_t size);

        /// Обеспечивает обновление данных в шейдере
        virtual void UpdateUBO(uint32_t UBO, void* pData, uint64_t size);
        void UpdateCurrentUBO(void* pData, uint64_t size);

        /// Обеспечивает обновление данных в шейдере
        virtual void UpdateSSBO(uint32_t SSBO, void* pData, uint64_t size);

        /// Читает данные из SSBO в память
        virtual void ReadSSBO(uint32_t SSBO, void* pData, uint64_t size);

        /// Привязываем к дескриптору юниформы. Работает не во всех API
        virtual void UpdateDescriptorSets(uint32_t descriptorSet, const SRDescriptorUpdateInfos& updateInfo);

        /// Передает данные в шейдер, которые не будут обновляться до следующего перерисовывания сцены.
        /// Поддерживается не всеми API
        virtual void PushConstants(void* pData, uint64_t size);

        virtual void BindTexture(uint8_t activeTexture, uint32_t textureId);
        virtual void BindAttachment(uint8_t activeTexture, uint32_t textureId);

        /// Привязка UBO к набору дескрипторов. Поддерживается не всеми API
        virtual bool BindDescriptorSet(uint32_t descriptorSet);

        virtual void ResetLastShader();

        void SetDrawInstancesCount(uint32_t count, uint32_t start = 0);
        void ResetDrawInstancesCount();

    protected:
        std::map<OverlayType, SR_HTYPES_NS::SharedPtr<Overlay>> m_overlays;

        PipelinePreInitInfo m_preInitInfo;

        FrameBufferQueue m_fboQueue;

        bool m_isComputeState = false;
        bool m_isRenderState = false;
        bool m_isCmdState = false;
        bool m_enableGPUAssist = false;
        bool m_enableValidationLayers = false;
        bool m_enableValidationDebug = false;

        mutable uint64_t m_errorsCount = 0;

        std::atomic<bool> m_dirty = false;

        SR_UTILS_NS::StringAtom m_renderStageId;

        WindowPtr m_window;
        RenderContextPtr m_renderContext;

        SR_HTYPES_NS::PoolSet<bool> m_bindedDescriptors;

        PipelineState m_state;
        PipelineState m_previousState;
        /// Состояние, которое было на момент постоения сцены рендера
        std::vector<PipelineState> m_buildStates;

        /// Все параметры, относящиется к мультисемплингу
        std::optional<uint8_t> m_newSampleCount;
        uint8_t m_currentSampleCount = 1;
        uint8_t m_requiredSampleCount = 1;
        uint8_t m_supportedSampleCount = 0;
        bool m_isMultiSampleSupported = false;

        uint32_t m_frames = 0;
        uint32_t m_drawInstancesCount = 1;
        uint32_t m_drawInstancesStart = 0;
        uint32_t m_framesPerSecond = 0;
        std::optional<SR_UTILS_NS::TimePointType> m_lastSecond;

        bool m_isShaderChanged = true;

    };
}

#endif //SR_ENGINE_PIPELINE_H
