//
// Created by Monika on 18.07.2022.
//

#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>

#include <Graphics/Window/Window.h>
#include <Graphics/Memory/ShaderProgramManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/CameraManager.h>
#include <Graphics/Memory/SSBOManager.h>

#include <Graphics/Pipeline/HeadlessPipeline.h>
#include <Graphics/Pass/FrameBufferPass.h>

#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Mesh.h>
#include <Graphics/Types/Skybox.h>
#include <Graphics/Types/RenderTarget.h>

#if defined(SR_USE_VULKAN)
    #include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#elif defined(SR_EMSCRIPTEN)
    #include <Graphics/Pipeline/WebGPU/WebGPUPipeline.h>
#endif

#include <Utils/Common/StoreUtils.h>
#include <Utils/Common/StringUtils.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/CLIManager.h>
#include <Utils/Events/Broadcaster.h>
#include <Utils/Serialization/SRASerialization.h>

namespace SR_GRAPH_NS {
    template<typename T> bool UpdateRenderResource(RenderContext* pRenderContext, T& resourceList) noexcept {
        SR_TRACY_ZONE;

        bool dirty = false;

        if constexpr (std::is_same_v<T, std::vector<SR_HTYPES_NS::SharedPtr<IRenderTechnique>>>) {
            for (auto&& pIt = std::begin(resourceList); pIt != std::end(resourceList); ) {
                SR_HTYPES_NS::SharedPtr<IRenderTechnique> pRenderTechnique = *pIt;

                if (!pRenderTechnique) {
                    SRHalt("Render technique is nullptr!");
                    pIt = resourceList.erase(pIt);
                    dirty |= true;
                    continue;
                }

                if (pRenderTechnique->IsTechniqueDead()) {
                    pRenderTechnique->DeInitGraphicsResource();
                    pIt = resourceList.erase(pIt);
                    pRenderTechnique.AutoFree();
                    dirty |= true;
                }
                else {
                    ++pIt;
                }
            }
        }
        else if constexpr (std::is_same_v<T, std::vector<SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>>>) {
            for (auto&& pIt = std::begin(resourceList); pIt != std::end(resourceList); ) {
                SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer> pFramebuffer = *pIt;

                if (!pFramebuffer) {
                    SRHalt("Framebuffer is nullptr!");
                    pIt = resourceList.erase(pIt);
                    dirty |= true;
                    continue;
                }

                if (pFramebuffer->IsFramebufferDead()) {
                    pFramebuffer->DeInitGraphicsResource();
                    pIt = resourceList.erase(pIt);
                    pFramebuffer.AutoFree();
                    dirty |= true;
                }
                else {
                    ++pIt;
                }
            }
        }
        else {
            for (auto pIt = std::begin(resourceList); pIt != std::end(resourceList); ) {
                if (auto pResource = *pIt) {
                    const bool removed = pResource->Execute([&]() -> bool {
                        if (pResource->GetCountUses() == 1) {
                            SRAssert(pResource->GetContainerParents().empty());

                            /// Ресурс необязательно имеет видеопамять, а лишь содержит другие ресурсы, например материал.
                            if (auto&& pGraphicsResource = dynamic_cast<Memory::IGraphicsResource*>(pResource.Get())) {
                                pGraphicsResource->DeInitGraphicsResource();
                            }
                            else {
                                SRHalt("Resource is not IGraphicsResource!");
                            }

                            pResource->RemoveUsePoint();
                            pIt = resourceList.erase(pIt);
                            /// После освобождения ресурса необходимо перестроить все контекстные сцены рендера.
                            dirty |= true;
                            return true;
                        }

                        return false;
                    });

                    /// TODO: это безопасно?
                    if (!removed) {
                        ++pIt;
                    }
                }
                else {
                    SRHalt("Resource is nullptr!");
                    pIt = resourceList.erase(pIt);
                }
            }
        }

        return dirty;
    }

    RenderContext::RenderContext()
        : Super(this)
    { }

    bool RenderContext::Update() noexcept {
        SR_TRACY_ZONE;

        /**
         * Все ресурсы при завершении работы рендера должны остаться только с одним use-point'ом.
         * В противном случае память никогда не освободится.
        */

        bool dirty = false;

        m_updateState = static_cast<RCUpdateQueueState>(static_cast<uint8_t>(m_updateState) + 1);

        switch (m_updateState) {
            case RCUpdateQueueState::Framebuffers: dirty |= UpdateRenderResource(this, m_framebuffers); break;
            case RCUpdateQueueState::Shaders: dirty |= UpdateRenderResource(this, m_shaders); break;
            case RCUpdateQueueState::Textures: dirty |= UpdateRenderResource(this, m_textures); break;
            case RCUpdateQueueState::Techniques: dirty |= UpdateRenderResource(this, m_techniques); break;
            case RCUpdateQueueState::Skyboxes: dirty |= UpdateRenderResource(this, m_skyboxes); break;
            case RCUpdateQueueState::End:
                m_updateState = RCUpdateQueueState::Begin;
                break;
            default:
                SRHaltOnce0();
                break;
        }

        for (auto pIt = std::begin(m_scenes); pIt != std::end(m_scenes); ) {
            auto&& [pScene, pRenderScene] = *pIt;

            /// Нет смысла синхронизировать сцену рендера, так как она еще способна сама позаботиться о себе
            if (pScene.Valid()) {
                if (dirty) {
                    pRenderScene->SetDirty();
                }

                ++pIt;
                continue;
            }

            if (!pRenderScene) {
                ++pIt;
                continue;
            }

            /// Синхронизируем и проверяем, есть ли еще на сцене объекты
            pRenderScene->Synchronize();

            if (!pRenderScene->IsEmpty()) {
                ++pIt;
                continue;
            }

            pRenderScene->DeInit();

            UpdateRenderResource(this, m_techniques);

            /// Как только уничтожается основная сцена, уничтожаем сцену рендера
            SR_LOG("RenderContext::Update() : destroy render scene...");
            pRenderScene.AutoFree();
            pIt = m_scenes.erase(pIt);
        }

        return dirty;
    }

    bool RenderContext::Init() {
        SR_TRACY_ZONE;

        SR_INFO("RenderContext::Init() : initializing render context...");

        ReloadGraphicsSettings();

        m_activePreset = SR_UTILS_NS::StoreUtils::User::GetString("RenderPreset", "Default");
        m_isOptimizedUpdateEnabled = SR_UTILS_NS::Features::Instance().Enabled("OptimizedRenderUpdate", true);
        m_isFrustumCullingEnabled = SR_UTILS_NS::Features::Instance().Enabled("FrustumCulling");

        if (!InitPipeline()) {
            SR_ERROR("RenderContext::Init() : failed to initialize pipeline!");
            return false;
        }

        SR_INFO("RenderContext::Init() : initializing overlay...");

        if (m_window && !m_pipeline->InitOverlay()) {
            SR_ERROR("RenderContext::Init() : failed to initialize overlay!");
            return false;
        }

        Memory::UBOManager::Instance().SetPipeline(m_pipeline);
        Memory::CameraManager::Instance().SetPipeline(m_pipeline);
        Memory::ShaderProgramManager::Instance().SetPipeline(m_pipeline);

        SR_GRAPH_NS::SSBOManager::Instance().SetPipeline(m_pipeline);
        SR_GRAPH_NS::DescriptorManager::Instance().SetPipeline(m_pipeline);

        /// ----------------------------------------------------------------------------

        ImageMetaInfo imageMetaInfo;

        imageMetaInfo.format = ImageFormat::RGBA8_UNORM;
        imageMetaInfo.filter = TextureFilter::NEAREST;
        imageMetaInfo.compression = TextureCompression::None;
        imageMetaInfo.mipLevels = 1;
        imageMetaInfo.cpuUsage = false;

        /// так как вписать в код данные текстуры невозможно, то она хранится в виде base64, текстура размером 1x1 белого цвета формата png
        const std::string image = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABmJLR0QA/wD/AP+gvaeTAAAADUlEQVQI12N48eIFOwAINALALwGcPAAAAABJRU5ErkJggg==";

        if ((m_noneTexture = SR_GTYPES_NS::Texture::LoadFromMemory(SR_UTILS_NS::StringUtils::Base64Decode(image), imageMetaInfo))) {
            m_noneTexture->AddUsePoint();
        }
        else {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to create none texture!");
            return false;
        }

        /// ----------------------------------------------------------------------------

        if (SR_UTILS_NS::Features::Instance().Enabled("LoadDefaultGraphicsResources", true)) {
            if (!LoadDefaultResources()) {
                SR_ERROR("RenderContext::Init() : failed to load default resources!");
                return false;
            }
        }

        return true;
    }

    void RenderContext::Close() {
        SR_LOG("RenderContext::Close() : closing render context...");

        if (m_pipeline) {
            m_pipeline->WaitRenderIdle();
        }

        SRAssert2(!m_isClosed, "Render context is already closed!");
        m_isClosed = true;

        if (m_noneTexture) {
            m_noneTexture->RemoveUsePoint();
            m_noneTexture = nullptr;
        }

        if (m_defaultTexture) {
            m_defaultTexture->RemoveUsePoint();
            m_defaultTexture = nullptr;
        }

        m_defaultMaterial.Reset();
        m_defaultUIMaterial.Reset();

        if (m_renderSettings) {
            m_onSettingsReloaded.Reset();
            m_renderSettings->RemoveUsePoint();
            m_renderSettings.Reset();
        }

        uint32_t syncStep = 0;
        const uint32_t maxErrStep = 50;

        SR_UTILS_NS::ResourceManager::Instance().PullWatchers();
        SR_UTILS_NS::ResourceManager::Instance().ReloadResources(0.f);

        SR_UTILS_NS::ResourceManager::Instance().Synchronize(true);

        while (!IsEmpty()) {
            SR_TRACY_ZONE_N("Sync free resources iteration");

            SR_SYSTEM_LOG("RenderContext::Close() : synchronizing resources (step " + std::to_string(++syncStep) + ")");

            while (Update()) {
                SR_NOOP;
            }

            SR_UTILS_NS::ResourceManager::Instance().Synchronize(true);

            if (maxErrStep == syncStep) {
                SR_ERROR("RenderContext::Close() : [FATAL] resources can not be released! Render resources:\n"
                    "\tSkyboxes: {}\n"
                    "\tShaders: {}\n"
                    "\tRender techniques: {}\n"
                    "\tFrameBuffers: {}\n"
                    "\tTextures: {}",
                    m_skyboxes.size(), m_shaders.size(), m_techniques.size(), m_framebuffers.size(), m_textures.size()
                );

                SR_UTILS_NS::ResourceManager::Instance().PrintMemoryDump();
                SR_PLATFORM_NS::Terminate();
                break;
            }

            SR_HTYPES_NS::Thread::Sleep(50);
        }

        SR_LOG("RenderContext::Close() : render context successfully closed!");
    }

    RenderContext::RenderScenePtr RenderContext::CreateScene(const SR_WORLD_NS::Scene::Ptr &pScene) {
        SR_TRACY_ZONE;

        SRAssert2(!m_isClosed, "Render context is closed!");

        RenderScenePtr pRenderScene;

        if (pScene) {
            auto&& dataStorage = pScene->GetDataStorage();

            /// У каждой сцены может быть только одна сцена рендера
            if (dataStorage.GetValueDef<RenderScenePtr>(RenderScenePtr())) {
                SR_ERROR("RenderContext::CreateScene() : render scene is already exists!");
                return pRenderScene;
            }

            pRenderScene = new RenderScene(pScene, this);
            pRenderScene->Init();

            m_scenes.emplace_back(std::make_pair(
                pScene,
                pRenderScene
            ));

            dataStorage.SetValue<RenderScenePtr>(pRenderScene);
        }
        else {
            SR_ERROR("RenderContext::CreateScene() : scene is invalid!");
        }

        return pRenderScene;
    }

    void RenderContext::RegisterRenderTarget(SR_GTYPES_NS::RenderTarget* pRenderTarget) {
        if (auto&& pIt = std::ranges::find_if(m_renderTargets, [pRenderTarget](const auto pExistingRenderTarget) {
            return pExistingRenderTarget == pRenderTarget;
        }); pIt != m_renderTargets.end()) {
            SRHalt("RenderContext::RegisterRenderTarget() : render target {} is already registered!", pRenderTarget->GetName());
            return;
        }
        m_renderTargets.emplace_back(pRenderTarget);
    }

    void RenderContext::UnRegisterRenderTarget(SR_GTYPES_NS::RenderTarget* pRenderTarget) {
        auto&& pIt = std::ranges::find_if(m_renderTargets, [pRenderTarget](const auto pExistingRenderTarget) {
            return pExistingRenderTarget == pRenderTarget;
        });
        if (pIt != m_renderTargets.end()) {
            m_renderTargets.erase(pIt);
        }
        else {
            SRHalt("RenderContext::UnRegisterRenderTarget() : render target {} is not registered!", pRenderTarget->GetName());
        }
    }

    SR_GTYPES_NS::RenderTarget* RenderContext::FindRenderTarget(SR_UTILS_NS::StringAtom name) const {
        for (auto&& pRenderTarget : m_renderTargets) {
            if (pRenderTarget->GetName() == name) {
                return pRenderTarget;
            }
        }
        return nullptr;
    }

    void RenderContext::Register(Memory::IGraphicsResource* pResource, SR_UTILS_NS::PassKey<Memory::IGraphicsResource>) {
        SRAssert2(!m_isClosed, "RenderContext is closed");

        if (auto&& pIResource = dynamic_cast<SR_UTILS_NS::IResource*>(pResource)) {
            pIResource->AddUsePoint();
        }

        if (auto&& pFrameBuffer = dynamic_cast<SR_GTYPES_NS::Framebuffer*>(pResource)) {
            m_framebuffers.emplace_back(pFrameBuffer);
        }
        else if (auto&& pShader = dynamic_cast<SR_GTYPES_NS::Shader*>(pResource)) {
            m_shaders.emplace_back(pShader);
        }
        else if (auto&& pTexture = dynamic_cast<SR_GTYPES_NS::Texture*>(pResource)) {
            m_textures.emplace_back(pTexture);
        }
        else if (auto&& pTechnique = dynamic_cast<IRenderTechnique*>(pResource)) {
            m_techniques.emplace_back(pTechnique);
        }
        else if (auto&& pSkybox = dynamic_cast<SR_GTYPES_NS::Skybox*>(pResource)) {
            m_skyboxes.emplace_back(pSkybox);
        }
        else {
            SRHalt("RenderContext::Register() : unknown resource type!");
        }
    }

    bool RenderContext::IsEmpty() const {
        if (m_defaultTexture || m_noneTexture || m_defaultMaterial || m_defaultUIMaterial || m_renderSettings) {
            return false;
        }

        return
            m_shaders.empty() &&
            m_framebuffers.empty() &&
            m_textures.empty() &&
            m_skyboxes.empty() &&
            m_scenes.empty() &&
            m_techniques.empty();
    }

    const RenderContext::PipelinePtr& RenderContext::GetPipeline() const {
        return m_pipeline;
    }

    RenderContext::PipelinePtr& RenderContext::GetPipeline() {
        return m_pipeline;
    }

    PipelineType RenderContext::GetPipelineType() const {
        return m_pipeline->GetType();
    }

    RenderContext::MaterialPtr RenderContext::GetDefaultMaterial() const {
        return m_defaultMaterial;
    }

    RenderContext::MaterialPtr RenderContext::GetDefaultUIMaterial() const {
        return m_defaultUIMaterial;
    }

    void RenderContext::SetDirty() {
        for (auto&& [pScene, pRenderScene] : m_scenes) {
            pRenderScene->SetDirty();
        }
        m_pipeline->SetDirty(true);
    }

    RenderContext::TexturePtr RenderContext::GetDefaultTexture() const {
        return m_defaultTexture && m_defaultTexture->CanBeUsed() ? m_defaultTexture : m_noneTexture;
    }

    RenderContext::TexturePtr RenderContext::GetNoneTexture() const {
        return m_noneTexture;
    }

    void RenderContext::OnResize(const SR_MATH_NS::UVector2& size) {
        SR_TRACY_ZONE;

        m_hasChangedFrameBuffers = true;

        if (m_pipeline) {
            m_pipeline->OnResize(size);
        }

        for (auto pIt = std::begin(m_scenes); pIt != std::end(m_scenes); ++pIt) {
            auto&& [pScene, pRenderScene] = *pIt;

            if (!pScene) {
                continue;
            }

            if (pRenderScene) {
                pRenderScene->OnResize(size);
            }
        }
    }

    SR_MATH_NS::UVector2 RenderContext::GetWindowSize() const {
        return m_window->GetSize();
    }

    RenderContext::FramebufferPtr RenderContext::FindFramebuffer(SR_UTILS_NS::StringAtom name, CameraPtr pCamera) const {
        SR_TRACY_ZONE;

        for (auto&& pTechnique : m_techniques) {
            if (pTechnique->GetCamera() != pCamera) {
                continue;
            }

            auto&& pController = pTechnique->GetFrameBufferController(name);
            if (pController) {
                return pController->GetFramebuffer();
            }
        }

        return nullptr;
    }

    RenderContext::FramebufferPtr RenderContext::FindFramebuffer(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        for (auto&& pTechnique : m_techniques) {
            auto&& pController = pTechnique->GetFrameBufferController(name);
            if (pController) {
                return pController->GetFramebuffer();
            }
        }

        return nullptr;
    }

    RenderContext::ShaderPtr RenderContext::GetCurrentShader() const noexcept {
        return m_pipeline->GetCurrentShader();
    }

    bool RenderContext::SetCurrentShader(RenderContext::ShaderPtr pShader) {
        m_pipeline->SetCurrentShader(pShader.Get());

        if (pShader && !pShader->IsAvailable()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("The shader was not bound and not available!");
            return false;
        }

        return true;
    }

    RenderContext::WindowPtr RenderContext::GetWindow() const {
        return m_window;
    }

    void RenderContext::PrepareFrame() {
        SR_TRACY_ZONE;

        if (m_pipeline) {
            m_pipeline->PrepareFrame();
        }

        for (auto&& pFrameBuffer : m_framebuffers) {
            m_hasChangedFrameBuffers |= pFrameBuffer->IsDirty();
            pFrameBuffer->Update();
        }

        if (m_noneTexture) {
            m_noneTexture->PrepareFrame();
        }

        if (m_defaultTexture) {
            m_defaultTexture->PrepareFrame();
        }

        for (auto&& pTexture : m_textures) {
            pTexture->PrepareFrame();
        }

        if (m_hasChangedFrameBuffers) {
            m_hasChangedFrameBuffers = false;
            m_isNeedGarbageCollection = true;
        }

        if (m_isNeedGarbageCollection) {
            SR_GRAPH_NS::Memory::ShaderProgramManager::Instance().CollectUnused();
            SR_GRAPH_NS::Memory::UBOManager::Instance().CollectUnused();
            SR_GRAPH_NS::DescriptorManager::Instance().CollectUnused();
            m_isNeedGarbageCollection = false;
        }

        SR_UTILS_NS::Broadcaster::Instance().Broadcast(SR_UTILS_NS::Events::EVENT_ON_PREPARE_FRAME);
    }

    const std::vector<SR_GTYPES_NS::Shader::Ptr>& RenderContext::GetShaders() const noexcept {
        return m_shaders;
    }

    const std::vector<SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>>& RenderContext::GetFramebuffers() const noexcept {
        return m_framebuffers;
    }

    const std::vector<RenderContext::TexturePtr>& RenderContext::GetTextures() const noexcept {
        return m_textures;
    }

    const std::vector<IRenderTechnique::Ptr>& RenderContext::GetRenderTechniques() const noexcept {
        return m_techniques;
    }

    const std::vector<SR_GTYPES_NS::Skybox::Ptr>& RenderContext::GetSkyboxes() const noexcept {
        return m_skyboxes;
    }

    void RenderContext::OnMultiSampleChanged() {
        SR_TRACY_ZONE;

        for (auto&& pFrameBuffer : m_framebuffers) {
            pFrameBuffer->SetDirty();
        }

        for (auto&& pRenderTechnique : m_techniques) {
            pRenderTechnique->OnMultisampleChanged();
        }
    }

    RenderContext::~RenderContext() {
        SRAssert2(IsEmpty(), "Render context is not empty!");

        if (m_pipeline) {
            m_pipeline->Destroy();
        }

        m_pipeline.AutoFree();
    }

    bool RenderContext::LoadDefaultResources() {
        if ((m_defaultTexture = CoreResLoader::Load<SR_GTYPES_NS::Texture>("Engine/Textures/default_improved.png"))) {
            m_defaultTexture->AddUsePoint();
        }
        else {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to load default texture!");
        }

        /// ----------------------------------------------------------------------------

        if (!((m_defaultMaterial = FileMaterial::Load(GetSettings().defaultMaterial)))) {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to load default material!");
        }

        /// ----------------------------------------------------------------------------

        if (!((m_defaultUIMaterial = FileMaterial::Load("Engine/Materials/UI/ui.mat")))) {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to load default UI material!");
        }

        /// ----------------------------------------------------------------------------

        if (!m_defaultMaterial || !m_defaultUIMaterial) {
            static const SR_UTILS_NS::Path templateMaterialPath = "Engine/Materials/template.mat";

            SR_INFO("RenderContext::LoadDefaultResources() : failed to load default materials! Creating template material at: " + templateMaterialPath.ToString());

            if (!SR_GRAPH_NS::FileMaterialResource::CreateTemplateMaterial(templateMaterialPath)) {
                SR_ERROR("RenderContext::LoadDefaultResources() : failed to create template material!");
            }
            else {
                SR_INFO("RenderContext::LoadDefaultResources() : template material created successfully! Use it for creating materials.");
            }
        }

        return true;
    }

    bool RenderContext::InitPipeline() {
        SR_TRACY_ZONE;

        SR_GRAPH("RenderContext::InitPipeline() : initializing the render pipeline...");

        if (!m_pipeline->Init()) {
            SR_ERROR("Engine::InitializeRender() : failed to initialize the pipeline!");
            return false;
        }

        if (!m_pipeline->PostInit()) {
            SR_ERROR("Engine::InitializeRender() : failed to post-initialize pipeline!");
            return false;
        }

        SR_LOG("Engine::InitializeRender() : vendor is "   + m_pipeline->GetVendor());
        SR_LOG("Engine::InitializeRender() : renderer is " + m_pipeline->GetRenderer());
        SR_LOG("Engine::InitializeRender() : version is "  + m_pipeline->GetVersion());

        return true;
    }

    bool RenderContext::IsDirty() const {
        for (auto&& [pScene, pRenderScene] : m_scenes) {
            if (pRenderScene->IsDirty()) {
                return true;
            }
        }

        return false;
    }

    void RenderContext::SwitchWindow(RenderContext::WindowPtr pWindow) {
        m_window = std::move(pWindow);
        m_pipeline->SwitchWindow(m_window);
    }

    const RenderSettingsPreset& RenderContext::GetSettingsPreset() const noexcept {
        SRAssert(m_renderSettings);
        return m_renderSettings->GetPreset(m_activePreset);
    }

    RenderContext::Definitions RenderContext::GetShaderMacros() const {
        Definitions macros = m_definitions;
        const auto& preset = GetSettingsPreset();
        for (const auto define : preset.shaderDefines) {
            macros[define];
        }
        return macros;
    }

    const RenderSettings& RenderContext::GetSettings() const noexcept {
        SRAssert(m_renderSettings);
        return *m_renderSettings;
    }

    void RenderContext::SetActivePreset(SR_UTILS_NS::StringAtom name) {
        if (m_activePreset == name) {
            return;
        }
        m_activePreset = name;
        SR_UTILS_NS::StoreUtils::User::SetString("RenderPreset", m_activePreset.ToString());
        ReloadShaders();
    }

    void RenderContext::SetMacro(SR_UTILS_NS::StringAtom define, std::optional<std::string> value) {
        if (auto&& pIt = m_definitions.find(define); pIt != m_definitions.end()) {
            if (value) {
                pIt->second = *value;
            }
            else {
                pIt->second.clear();
            }
        }
        else if (value) {
            m_definitions[define] = *value;
        }
        else {
            m_definitions[define];
        }
    }

    void RenderContext::RemoveMacro(SR_UTILS_NS::StringAtom define) {
        if (auto it = m_definitions.find(define); it != m_definitions.end()) {
            m_definitions.erase(it);
        }
    }

    void RenderContext::ReloadShaders() {
        SR_UTILS_NS::ResourceManager::Instance().ReloadAll(SR_GTYPES_NS::Shader::GetClassStaticName());
        SR_UTILS_NS::Broadcaster::Instance().Broadcast(SR_UTILS_NS::Events::EVENT_ON_RENDER_SETTINGS_CHANGED_ID);
        SetDirty();
        m_isNeedGarbageCollection = true;
    }

    void RenderContext::ReloadTextures() {
        SR_UTILS_NS::ResourceManager::Instance().ReloadAll(SR_GTYPES_NS::Texture::GetClassStaticName());
        SR_UTILS_NS::Broadcaster::Instance().Broadcast(SR_UTILS_NS::Events::EVENT_ON_RENDER_SETTINGS_CHANGED_ID);
        SetDirty();
        m_isNeedGarbageCollection = true;
    }

    bool RenderContext::PreInit() {
        SR_TRACY_ZONE;
        SR_LOG("RenderContext::PreInit() : pre-initializing render context...");

        if (SR_UTILS_NS::CLIManager::Instance().IsHeadlessMode() || SR_UTILS_NS::Features::Instance().Enabled("HeadlessPipeline", false)) {
            SR_LOG("RenderContext::PreInit() : creating headless pipeline...");
            m_pipeline = new HeadlessPipeline(GetThis());
        }
        else {
        #if defined(SR_USE_VULKAN)
            SR_LOG("RenderContext::PreInit() : creating vulkan pipeline...");
            m_pipeline = new VulkanPipeline(GetThis());
        #elif defined(SR_EMSCRIPTEN)
            SR_LOG("RenderContext::PreInit() : creating webgpu pipeline...");
            m_pipeline = new WebGPUPipeline(GetThis());
        #else
            SR_WARN("RenderContext::PreInit() : no suitable pipeline found for this platform! Falling back to headless pipeline...");
            m_pipeline = new HeadlessPipeline(GetThis());
        #endif
        }

        SR_LOG("RenderContext::PreInit() : loading render settings...");

        m_renderSettings = RenderSettings::LoadOrCreate<RenderSettings>("Engine/Configs/RenderSettings.sras");
        if (!m_renderSettings) {
            SR_ERROR("RenderContext::PreInit() : failed to load render settings!");
            return false;
        }
        m_renderSettings->AddUsePoint();
        m_onSettingsReloaded = m_renderSettings->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](auto&&) {
            ReloadShaders();
        });

        SR_LOG("RenderContext::PreInit() : pre-initializing the pipeline...");

        PipelinePreInitInfo pipelinePreInitInfo;
        pipelinePreInitInfo.appName = m_renderSettings->appName;
        pipelinePreInitInfo.engineName = m_renderSettings->engineName;
        pipelinePreInitInfo.samplesCount = SR_UTILS_NS::StoreUtils::User::GetInt("MultiSampling", 64);
        pipelinePreInitInfo.multisampling = SR_UTILS_NS::Features::Instance().Enabled("Multisampling", true);
        pipelinePreInitInfo.vsync = false;

        if (pipelinePreInitInfo.samplesCount < 1) {
            SR_WARN("Engine::PreInit() : invalid multisampling count: {}. Set to 1.", pipelinePreInitInfo.samplesCount);
            pipelinePreInitInfo.samplesCount = 1;
        }

    #if defined(SR_WIN32)
        pipelinePreInitInfo.GLSLCompilerPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Utilities/glslc.exe");
    #elif defined(SR_LINUX)
        pipelinePreInitInfo.GLSLCompilerPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Utilities/glslc");
    #endif

        if (!m_pipeline->PreInit(pipelinePreInitInfo)) {
            SR_ERROR("Engine::PreInit() : failed to pre-initialize the pipeline!");
            return false;
        }

        return true;
    }

    void RenderContext::ReloadGraphicsSettings() {
        if (auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat(ActiveGraphicsSettings::SETTINGS_PATH); path.IsFile()) {
            SR_LOG("RenderContext::ReloadGraphicsSettings() : loading active graphics settings from path: {}", path);
            SR_UTILS_NS::SRADeserializer deserializer;
            ActiveGraphicsSettings settings;
            if (!deserializer.LoadFromFile(path) || !settings.Load(deserializer)) {
                SR_ERROR("RenderContext::ReloadGraphicsSettings() : failed to load active graphics settings from path: {}", path);
            }
            else {
                SetGraphicsSettings(settings, false);
            }
        }
        else {
            SR_LOG("RenderContext::ReloadGraphicsSettings() : active graphics settings file not found at path: {}", path);
            SetGraphicsSettings(ActiveGraphicsSettings(), false);
        }
    }

    void RenderContext::SetGraphicsSettings(const ActiveGraphicsSettings& settings, bool reload) {
        SR_TRACY_ZONE;

        const bool needReloadTextures = m_activeGraphicsSettings.sRGB != settings.sRGB
            || m_activeGraphicsSettings.textureCompression != settings.textureCompression;

        const bool needReloadShaders = m_activeGraphicsSettings != settings;
        m_activeGraphicsSettings = settings;

        SwitchMacro("SR_SRGB", m_activeGraphicsSettings.sRGB);
        SwitchMacro("SR_HDR", m_activeGraphicsSettings.hdr);
        SwitchMacro("SR_AUTO_EXPOSURE", m_activeGraphicsSettings.autoExposure);
        SwitchMacro("SR_SHADOWS_QUALITY_EXTREME", m_activeGraphicsSettings.shadowsQuality == Quality::Extreme);

        if (!reload) {
            return;
        }

        if (needReloadShaders) {
            ReloadShaders();
        }
        if (needReloadTextures) {
            ReloadTextures();
        }
    }

    bool RenderContext::IsAsyncEarlyInit() {
        return m_pipeline && m_pipeline->IsAsyncEarlyInit();
    }
}
