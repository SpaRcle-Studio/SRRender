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
#include <Graphics/Memory/SSBOManager.h>
#include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#include <Graphics/Pass/FrameBufferPass.h>

#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/RenderTexture.h>
#include <Graphics/Types/Skybox.h>

namespace SR_GRAPH_NS {
    RenderContext::RenderContext()
        : Super(this)
    {
        m_pipeline = new VulkanPipeline(GetThis());
    }

    bool RenderContext::Update() noexcept {
        SR_TRACY_ZONE;

        /**
         * Все ресурсы при завершении работы рендера должны остаться только с одним use-point'ом.
         * В противном случае память никогда не освободится.
        */

        bool dirty = false;

        m_updateState = static_cast<RCUpdateQueueState>(static_cast<uint8_t>(m_updateState) + 1);

        switch (m_updateState) {
            case RCUpdateQueueState::Framebuffers: dirty |= Update(m_framebuffers); break;
            case RCUpdateQueueState::Shaders: dirty |= Update(m_shaders); break;
            case RCUpdateQueueState::Textures: dirty |= Update(m_textures); break;
            case RCUpdateQueueState::Techniques: dirty |= Update(m_techniques); break;
            case RCUpdateQueueState::Skyboxes: dirty |= Update(m_skyboxes); break;
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

            Update(m_techniques);

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

        m_isOptimizedUpdateEnabled = SR_UTILS_NS::Features::Instance().Enabled("OptimizedRenderUpdate", true);

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

        Memory::TextureConfig config;

        config.m_format = ImageFormat::RGBA8_UNORM;
        config.m_filter = TextureFilter::NEAREST;
        config.m_compression = TextureCompression::None;
        config.m_mipLevels = 1;
        config.m_alpha = SR_UTILS_NS::BoolExt::None;
        config.m_cpuUsage = false;

        /// так как вписать в код данные текстуры невозможно, то она хранится в виде base64, текстура размером 1x1 белого цвета формата png
        const std::string image = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAABmJLR0QA/wD/AP+gvaeTAAAADUlEQVQI12N48eIFOwAINALALwGcPAAAAABJRU5ErkJggg==";

        if ((m_noneTexture = SR_GTYPES_NS::Texture::LoadFromMemory(SR_UTILS_NS::StringUtils::Base64Decode(image), config))) {
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

        if (pScene.RecursiveLockIfValid()) {
            auto&& dataStorage = pScene->GetDataStorage();

            /// У каждой сцены может быть только одна сцена рендера
            if (dataStorage.GetValueDef<RenderScenePtr>(RenderScenePtr())) {
                SR_ERROR("RenderContext::CreateScene() : render scene is already exists!");
                pScene.Unlock();
                return pRenderScene;
            }

            pRenderScene = new RenderScene(pScene, this);
            pRenderScene->Init();

            m_scenes.emplace_back(std::make_pair(
                pScene,
                pRenderScene
            ));

            dataStorage.SetValue<RenderScenePtr>(pRenderScene);
            pScene.Unlock();
        }
        else {
            SR_ERROR("RenderContext::CreateScene() : scene is invalid!");
        }

        return pRenderScene;
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
        if (m_defaultTexture || m_noneTexture || m_defaultMaterial || m_defaultUIMaterial) {
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

    void RenderContext::SetDirty() {
        for (auto&& [pScene, pRenderScene] : m_scenes) {
            pRenderScene->SetDirty();
        }
    }

    RenderContext::TexturePtr RenderContext::GetDefaultTexture() const {
        return m_defaultTexture ? m_defaultTexture : m_noneTexture;
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

        for (auto&& pFrameBuffer : m_framebuffers) {
            pFrameBuffer->Update();
            m_hasChangedFrameBuffers = true;
        }

        if (m_hasChangedFrameBuffers) {
            if (m_isOptimizedUpdateEnabled) {
                for (auto&& [pScene, pRenderScene] : m_scenes) {
                    pRenderScene->GetRenderStrategy()->ForEachMesh([](SR_GTYPES_NS::Mesh* pMesh) {
                        pMesh->MarkUniformsDirty();
                    });
                }
            }

            m_hasChangedFrameBuffers = false;
            m_isNeedGarbageCollection = true;
        }

        if (m_isNeedGarbageCollection) {
            SR_GRAPH_NS::Memory::ShaderProgramManager::Instance().CollectUnused();
            SR_GRAPH_NS::Memory::UBOManager::Instance().CollectUnused();
            SR_GRAPH_NS::DescriptorManager::Instance().CollectUnused();
            m_isNeedGarbageCollection = false;
        }
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
        Memory::TextureConfig config;

        config.m_format = ImageFormat::RGBA8_UNORM;
        config.m_filter = TextureFilter::NEAREST;
        config.m_compression = TextureCompression::None;
        config.m_mipLevels = 1;
        config.m_alpha = SR_UTILS_NS::BoolExt::None;
        config.m_cpuUsage = false;

        if ((m_defaultTexture = SR_GTYPES_NS::Texture::Load("Engine/Textures/default_improved.png", config))) {
            m_defaultTexture->AddUsePoint();
        }
        else {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to load default texture!");
        }

        /// ----------------------------------------------------------------------------

        if (!((m_defaultMaterial = FileMaterial::Load("Engine/Materials/default.mat")))) {
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

        PipelinePreInitInfo pipelinePreInitInfo;
        pipelinePreInitInfo.appName = "SpaRcle Engine";
        pipelinePreInitInfo.engineName = "SREngine";
        pipelinePreInitInfo.samplesCount = 64;
        pipelinePreInitInfo.vsync = false;
    #if defined(SR_WIN32)
        pipelinePreInitInfo.GLSLCompilerPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Utilities/glslc.exe");
    #elif defined(SR_LINUX)
        pipelinePreInitInfo.GLSLCompilerPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Utilities/glslc");
    #endif

        if (!m_pipeline->PreInit(pipelinePreInitInfo)) {
            SR_ERROR("Engine::InitializeRender() : failed to pre-initialize the pipeline!");
            return false;
        }

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
}
