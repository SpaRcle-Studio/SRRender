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
#include <Graphics/Pass/FramebufferPass.h>

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
            case RCUpdateQueueState::Materials: dirty |= Update(m_materials); break;
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

            /// Синхронизируем и проверяем, есть ли еще на сцене объекты
            if (!pRenderScene.Do<bool>([](RenderScene* pRScene) -> bool {
                pRScene->Synchronize();
                return pRScene->IsEmpty();
            }, false)) {
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

        if (m_defaultMaterial) {
            m_defaultMaterial->RemoveUsePoint();
            m_defaultMaterial = nullptr;
        }

        if (m_defaultUIMaterial) {
            m_defaultUIMaterial->RemoveUsePoint();
            m_defaultUIMaterial = nullptr;
        }

        uint32_t syncStep = 0;
        const uint32_t maxErrStep = 50;

        SR_UTILS_NS::ResourceManager::Instance().UpdateWatchers(0.f);
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
                SR_ERROR("RenderContext::Close() : [FATAL] resources can not be released!");
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

    void RenderContext::Register(SR_GTYPES_NS::Framebuffer* pResource) {
        if (!RegisterResource(pResource)) {
            return;
        }
        m_framebuffers.emplace_back(pResource);
    }

    void RenderContext::Register(SR_GTYPES_NS::Shader *pResource) {
        if (!RegisterResource(pResource)) {
            return;
        }
        m_shaders.emplace_back(pResource);
    }

    void RenderContext::Register(SR_GTYPES_NS::Texture* pResource) {
        if (!RegisterResource(pResource)) {
            return;
        }
        m_textures.emplace_back(pResource);
    }

    void RenderContext::Register(IRenderTechnique* pResource) {
        if (!RegisterResource(pResource)) {
            return;
        }
        m_techniques.emplace_back(pResource);
    }

    void RenderContext::Register(RenderContext::MaterialPtr pResource) {
        if (!RegisterResource(pResource)) {
            return;
        }
        m_materials.emplace_back(pResource);
    }

    void RenderContext::Register(RenderContext::SkyboxPtr pResource) {
        if (!RegisterResource(pResource)) {
            return;
        }
        m_skyboxes.emplace_back(pResource);
    }

    bool RenderContext::IsEmpty() const {
        if (m_defaultTexture || m_noneTexture || m_defaultMaterial || m_defaultUIMaterial) {
            return false;
        }

        return
            m_shaders.empty() &&
            m_framebuffers.empty() &&
            m_textures.empty() &&
            m_materials.empty() &&
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
            auto&&[pScene, pRenderScene] = *pIt;

            if (!pScene.Valid()) {
                continue;
            }

            pRenderScene.Do([&size](RenderScene *pRScene) {
                pRScene->OnResize(size);
            });
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
        m_pipeline->SetCurrentShader(pShader);

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
            if (!pFrameBuffer->IsDirty() && pFrameBuffer->IsCalculated()) {
                continue;
            }

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

    const std::vector<SR_GTYPES_NS::Shader*>& RenderContext::GetShaders() const noexcept {
        return m_shaders;
    }

    const std::vector<SR_GTYPES_NS::Framebuffer*>& RenderContext::GetFramebuffers() const noexcept {
        return m_framebuffers;
    }

    const std::vector<SR_GTYPES_NS::Texture*>& RenderContext::GetTextures() const noexcept {
        return m_textures;
    }

    const std::vector<IRenderTechnique*>& RenderContext::GetRenderTechniques() const noexcept {
        return m_techniques;
    }

    const std::vector<RenderContext::MaterialPtr>& RenderContext::GetMaterials() const noexcept {
        return m_materials;
    }

    const std::vector<SR_GTYPES_NS::Skybox*>& RenderContext::GetSkyboxes() const noexcept {
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

        if ((m_defaultUIMaterial = FileMaterial::Load("Engine/Materials/UI/ui.mat"))) {
            m_defaultUIMaterial->AddUsePoint();
        }
        else {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to load default UI material!");
        }

        /// ----------------------------------------------------------------------------

        if ((m_defaultMaterial = FileMaterial::Load("Engine/Materials/default.mat"))) {
            m_defaultMaterial->AddUsePoint();
        }
        else {
            SR_ERROR("RenderContext::LoadDefaultResources() : failed to load default material!");
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
