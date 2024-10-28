//
// Created by Monika on 16.05.2022.
//

#include <Utils/DebugDraw.h>
#include <Utils/Types/SafePtrLockGuard.h>

#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Memory/CameraManager.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Geometry/DebugLine.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Render/DebugRenderer.h>
#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Window/Window.h>

namespace SR_GRAPH_NS {
    RenderScene::RenderScene(const ScenePtr& scene, RenderContext* pContext)
        : SR_HTYPES_NS::SafePtr<RenderScene>(this)
        , m_lightSystem(new LightSystem(GetThis()))
        , m_scene(scene)
        , m_debugRender(new DebugRenderer(this))
        , m_context(pContext)
    {
        m_renderStrategy = new RenderStrategy(this);
        m_debugRender->Init();
    }

    RenderScene::~RenderScene() {
        SRAssert(!m_lightSystem && !m_debugRender && !m_technique);
        m_renderStrategy.AutoFree();
        SRAssert(IsEmpty());
    }

    void RenderScene::DeInit() {
        SR_SAFE_DELETE_PTR(m_lightSystem);

        if (m_debugRender) {
            m_debugRender->DeInit();
            delete m_debugRender;
            m_debugRender = nullptr;
        }

        if (m_technique) {
            if (auto&& pResource = dynamic_cast<SR_UTILS_NS::IResource*>(m_technique)) {
                pResource->RemoveUsePoint();
            }
            m_technique = nullptr;
        }
    }

    void RenderScene::Render() {
        SR_TRACY_ZONE_N("Render scene");

        PrepareFrame();

        PrepareRender();

        /// ImGui будет нарисован поверх независимо от порядка отрисовки.
        /// Однако, если его нарисовать в конце, то пользователь может
        /// изменить данные отрисовки сцены и сломать уже нарисованную сцену
        Overlay();

        auto&& pPipeline = GetPipeline();

        if (IsDirty() || pPipeline->IsDirty()) {
            Build();
            if (pPipeline->IsFBOQueueValid()) {
                pPipeline->SetDirty(false);
            }
            else {
                pPipeline->ResetSubmitQueue();
                m_hasDrawData = false;
            }
        }

        Update();
        PostUpdate();

        if (!m_hasDrawData) {
            RenderBlackScreen();
        }
    }

    void RenderScene::SetDirty() {
        m_dirty.Increment();
    }

    bool RenderScene::IsDirty() const noexcept {
        return m_dirty.Get() > 0;
    }

    void RenderScene::SetDirtyCameras() {
        m_dirtyCameras = true;
    }

    bool RenderScene::IsEmpty() const {
        if (m_debugRender && !m_debugRender->IsEmpty()) {
            return false;
        }

        return m_cameras.empty();
    }

    RenderContext* RenderScene::GetContext() const {
        return m_context;
    }

    void RenderScene::BuildQueue() {
        m_queues.clear();

        ForEachTechnique([&](IRenderTechnique* pTechnique) {
            auto&& queues = pTechnique->GetQueues();
            for (uint32_t depth = 0; depth < queues.size(); ++depth) {
                if (m_queues.size() < depth + 1) {
                    m_queues.resize(depth + 1);
                }
                for (auto&& pPass : queues[depth]) {
                    m_queues[depth].emplace_back(pPass);
                    for (auto&& pFrameBuffer : pPass->GetFrameBuffers()) {
                        GetPipeline()->GetQueue().AddQueue(pFrameBuffer, depth);
                    }
                }
                SRAssert(!m_queues[depth].empty());
            }
        });

        /// if (!m_queues.empty()) {
        ///     std::string log = "RenderScene::BuildQueue() : \n";
        ///     for (auto&& queue : m_queues) {
        ///         log += "============================================\n";
        ///         for (auto&& pPass : queue) {
        ///             log += "\t" + std::string(pPass->GetName()) + "\n";
        ///         }
        ///     }
        ///     log += "============================================\n";
        ///     SR_LOG(log);
        /// }
    }

    void RenderScene::Build() {
        SR_TRACY_ZONE_N("Build render");

        if (m_renderStrategy) {
            m_renderStrategy->ClearErrors();
        }

        GetPipeline()->ClearFrameBuffersQueue();

        m_hasDrawData = false;

        SR_RENDER_TECHNIQUES_RETURN_CALL(Render)

        BuildQueue();

        m_dirty.Do([](uint32_t& data) {
            data = data > 1 ? 1 : 0;
        });
    }

    void RenderScene::Update() {
        SR_TRACY_ZONE_N("Update render");

        SR_RENDER_TECHNIQUES_CALL(Update)
    }

    void RenderScene::PostUpdate() {
        SR_TRACY_ZONE_N("Post update render");

        //if (m_renderStrategy) {
        //    m_renderStrategy->PostUpdate();
        //}

        SR_RENDER_TECHNIQUES_CALL(PostUpdate)
    }

    void RenderScene::Submit() {
        SR_TRACY_ZONE_N("Submit frame");

        GetPipeline()->DrawFrame();
    }

    void RenderScene::SetTechnique(IRenderTechnique *pTechnique) {
        if (m_technique) {
            if (auto&& pResource = dynamic_cast<SR_UTILS_NS::IResource*>(m_technique)) {
                pResource->RemoveUsePoint();
            }
            m_technique = nullptr;
        }

        if ((m_technique = pTechnique)) {
            if (auto&& pResource = dynamic_cast<SR_UTILS_NS::IResource*>(m_technique)) {
                pResource->AddUsePoint();
            }
            m_technique->SetRenderScene(GetThis());
        }
        else {
            SR_ERROR("RenderScene::SetTechnique() : failed to load render technique!");
        }

        SetDirty();
    }

    void RenderScene::SetTechnique(const SR_UTILS_NS::Path &path) {
        SRAssert2(GetContext()->GetPipeline(), "RenderScene::SetTechnique() : pipeline is nullptr!");
        SetTechnique(RenderTechnique::Load(path));
    }

    const RenderScene::WidgetManagers &RenderScene::GetWidgetManagers() const {
        return m_widgetManagers;
    }

    void RenderScene::Overlay() {
        SR_TRACY_ZONE;

        GetPipeline()->SetOverlayEnabled(OverlayType::ImGui, m_bOverlay);

        if (!m_bOverlay) {
            return;
        }

        SR_RENDER_TECHNIQUES_RETURN_CALL(Overlay)
    }

    void RenderScene::PrepareFrame() {
        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->PrepareFrame();
        }

        m_currentSkeleton = nullptr;

        m_context->PrepareFrame();
    }

    void RenderScene::PrepareRender() {
        SR_TRACY_ZONE;

        GetPipeline()->SetCurrentRenderStrategy(m_renderStrategy.Get());

        if (m_debugRender) {
            m_debugRender->Prepare();
        }

        if (m_dirtyCameras) {
            SortCameras();
        }

        if (m_renderStrategy) {
            m_renderStrategy->Prepare();
        }

        SR_RENDER_TECHNIQUES_CALL(Prepare)
    }

    void RenderScene::Register(RenderScene::WidgetManagerPtr pWidgetManager) {
        if (!pWidgetManager) {
            return;
        }

        pWidgetManager->SetRenderScene(GetThis());

        m_widgetManagers.emplace_back(pWidgetManager);
    }

    void RenderScene::Register(RenderScene::MeshPtr pMesh) {
        if (!pMesh) {
            SRHalt("RenderScene::Register() : mesh is nullptr!");
            return;
        }

        if (!pMesh->GetMaterial()) {
            SetMeshMaterial(pMesh);
        }

        if (!pMesh->GetMaterial()) {
            SR_ERROR("RenderScene::Register() : mesh material and default material are nullptr! Mesh: " + pMesh->GetGeometryName());
            return;
        }

        if (auto&& pText = dynamic_cast<SR_GTYPES_NS::ITextComponent*>(pMesh); pText && !pText->GetFont()) {
            pText->SetFont("Engine/Fonts/CalibriL.ttf");
        }

        if (!pMesh->GetMaterial()->GetShader()) {
            SR_ERROR("RenderScene::Register() : mesh have not shader!");
            return;
        }

        pMesh->SetRenderContext(m_context);

        m_renderStrategy->RegisterMesh(pMesh);
        SetDirty();
    }

    void RenderScene::Remove(RenderScene::WidgetManagerPtr pWidgetManager) {
        if (!pWidgetManager) {
            return;
        }

        for (auto&& pIt = m_widgetManagers.begin(); pIt != m_widgetManagers.end(); ) {
            if (*pIt == pWidgetManager) {
                pIt = m_widgetManagers.erase(pIt);
                return;
            }
            else {
                ++pIt;
            }
        }

        SRHalt("RenderScene::RemoveWidgetManager() : the widget manager not found!");
    }

    void RenderScene::Register(const CameraPtr& pCamera) {
        CameraInfo info;

        if (auto&& pWindow = GetWindow()) {
            pCamera->UpdateProjection(pWindow->GetSize().x, pWindow->GetSize().y);
        }
        else {
            pCamera->UpdateProjection(m_surfaceSize.x, m_surfaceSize.y);
        }

        info.pCamera = pCamera;

        m_cameras.emplace_back(info);

        m_dirtyCameras = true;
    }

    void RenderScene::Remove(const CameraPtr& pCamera) {
        for (auto&& cameraInfo : m_cameras) {
            if (cameraInfo.pCamera != pCamera) {
                continue;
            }

            cameraInfo.pCamera = CameraPtr();
            m_dirtyCameras = true;

            return;
        }

        SRHalt("RenderScene::DestroyCamera() : the camera not found!");
    }

    void RenderScene::SortCameras() {
        SR_TRACY_ZONE;

        SetDirty();

        m_dirtyCameras = false;
        m_mainCamera = nullptr;

        const uint64_t offScreenCamerasCount = m_offScreenCameras.size();
        m_offScreenCameras.clear();
        m_offScreenCameras.reserve(offScreenCamerasCount);

        const uint64_t editorCamerasCount = m_editorCameras.size();
        m_editorCameras.clear();
        m_editorCameras.reserve(editorCamerasCount);

        /// Удаляем уничтоженные камеры
        for (auto pIt = m_cameras.begin(); pIt != m_cameras.end(); ) {
            if (!pIt->pCamera) {
                SR_LOG("RenderScene::SortCameras() : remove destroyed camera...");
                pIt = m_cameras.erase(pIt);
            }
            else {
                ++pIt;
            }
        }

        /// Ищем главную камеру и закадровые камеры
        for (auto&& cameraInfo : m_cameras) {
            if (!cameraInfo.pCamera->IsActive()) {
                continue;
            }

            if (cameraInfo.pCamera->IsEditorCamera()) {
                m_editorCameras.emplace_back(cameraInfo.pCamera);
            }

            if (cameraInfo.pCamera->GetPriority() < 0) {
                m_offScreenCameras.emplace_back(cameraInfo.pCamera);
                continue;
            }

            /// Выбирается камера, чей приоритет выше
            if (!m_mainCamera || cameraInfo.pCamera->GetPriority() > m_mainCamera->GetPriority()) {
                m_mainCamera = cameraInfo.pCamera;
            }
        }

        /// TODO: убедиться, что сортируется так, как нужно
        std::stable_sort(m_offScreenCameras.begin(), m_offScreenCameras.end(), [](CameraPtr lhs, CameraPtr rhs) {
            return lhs->GetPriority() < rhs->GetPriority();
        });

        std::stable_sort(m_editorCameras.begin(), m_editorCameras.end(), [](CameraPtr lhs, CameraPtr rhs) {
            return lhs->GetPriority() < rhs->GetPriority();
        });

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Full) {
            SR_LOG("RenderScene::SortCameras() : cameras was sorted");
        }
    }

    const RenderScene::PipelinePtr& RenderScene::GetPipeline() const {
        return GetContext()->GetPipeline();
    }

    RenderScene::WindowPtr RenderScene::GetWindow() const {
        return GetContext()->GetWindow();
    }

    void RenderScene::RenderBlackScreen() {
        SR_TRACY_ZONE;

        auto&& pPipeline = GetPipeline();

        pPipeline->SetCurrentFrameBuffer(nullptr);

        for (uint8_t i = 0; i < pPipeline->GetBuildIterationsCount(); ++i) {
            pPipeline->SetBuildIteration(i);

            pPipeline->BindFrameBuffer(nullptr);
            pPipeline->ClearBuffers(0.5f, 0.5f, 0.5f, 1.f, 1.f, 1);

            pPipeline->BeginCmdBuffer();
            {
                pPipeline->BeginRender();
                pPipeline->SetViewport();
                pPipeline->SetScissor();
                pPipeline->EndRender();
            }
            pPipeline->EndCmdBuffer();
        }
    }

    bool RenderScene::IsOverlayEnabled() const {
        return m_bOverlay;
    }

    void RenderScene::SetOverlayEnabled(bool enabled) {
        m_bOverlay = enabled;
    }

    RenderScene::CameraPtr RenderScene::GetMainCamera() const {
        if (!m_editorCameras.empty()) {
            return m_editorCameras.front();
        }

        return m_mainCamera ? m_mainCamera : GetFirstOffScreenCamera();
    }

    RenderScene::CameraPtr RenderScene::GetFirstOffScreenCamera() const {
        if (m_offScreenCameras.empty()) {
            return nullptr;
        }

        return m_offScreenCameras.front();
    }

    void RenderScene::Synchronize() {
        SR_TRACY_ZONE;

        /// отладочной геометрией ничто не управляет, она уничтожается по истечению времени.
        /// ее нужно принудительно освобождать при закрытии сцены.
        if (m_debugRender && !m_scene.Valid()) {
            m_debugRender->Clear();
        }

        SortCameras();
    }

    void RenderScene::OnResize(const SR_MATH_NS::UVector2 &size) {
        m_surfaceSize = size;

        if (!m_context->GetWindow()->IsWindowCollapsed()) {
            for (auto&& cameraInfo : m_cameras) {
                if (!cameraInfo.pCamera) {
                    continue;
                }

                cameraInfo.pCamera->UpdateProjection(m_surfaceSize.x, m_surfaceSize.y);
            }
        }

        if (m_technique) {
            m_technique->OnResize(size);
        }
    }

    SR_MATH_NS::UVector2 RenderScene::GetSurfaceSize() const {
        return m_surfaceSize;
    }

    DebugRenderer* RenderScene::GetDebugRenderer() const {
        return m_debugRender;
    }

    void RenderScene::OnResourceReloaded(SR_UTILS_NS::IResource::Ptr pResource) {
        m_renderStrategy->OnResourceReloaded(pResource);

        SetDirty();
    }

    void RenderScene::ForEachTechnique(const SR_HTYPES_NS::Function<void(IRenderTechnique*)>& callback) {
        for (auto&& pCamera : m_offScreenCameras) {
            if (auto&& pRenderTechnique = pCamera->GetRenderTechnique()) {
                callback(pRenderTechnique);
            }
        }

        if (m_mainCamera) {
            if (auto &&pRenderTechnique = m_mainCamera->GetRenderTechnique()) {
                callback(pRenderTechnique);
            }
        }

        if (m_technique) {
            callback(m_technique);
        }
    }

    void RenderScene::SetMeshMaterial(RenderScene::MeshPtr pMesh) {
        if (pMesh->IsFlatMesh()) {
            if (auto&& pText2D = dynamic_cast<SR_GTYPES_NS::ITextComponent*>(pMesh)) {
                pText2D->SetMaterial("Engine/Materials/UI/ui_text_white.mat");
            }
            else if (auto&& pDefaultMat = GetContext()->GetDefaultUIMaterial()) {
                pMesh->SetMaterial(pDefaultMat);
            }
        }
        else {
            if (auto&& pText3D = dynamic_cast<SR_GTYPES_NS::ITextComponent*>(pMesh)) {
                pText3D->SetMaterial("Engine/Materials/text.mat");
            }
            else if (auto&& pDefaultMat = GetContext()->GetDefaultMaterial()) {
                pMesh->SetMaterial(pDefaultMat);
            }
        }
    }

    void RenderScene::Remove(RenderScene::MeshPtr pMesh) {
        m_renderStrategy->UnRegisterMesh(pMesh);
        SetDirty();
    }

    void RenderScene::ReRegister(const MeshRegistrationInfo& info) {
        m_renderStrategy->ReRegisterMesh(info);
        SetDirty();
    }
}
