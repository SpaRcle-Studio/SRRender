//
// Created by Monika on 10.10.2023.
//

#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Pass/GroupPass.h>
#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Pass/IColorBufferPass.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Loaders/RenderTechniquePostProcess.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/RenderTarget.h>

#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/IRenderTechnique.generated.hpp>

namespace SR_GRAPH_NS {
    void RenderTechniqueData::SetRenderStagesSettingsPath(const SR_UTILS_NS::Path& path) {
        renderStagesSettings = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
    }

    void RenderTechniqueData::AddPassBack(BasePass::Ptr pPass) {
        if (SRVerify(pPass)) {
            if (!pass) {
                pass = new GroupPass();
            }
            pass.StaticCast<GroupPass>()->InsertPass(pPass, pass.StaticCast<GroupPass>()->GetPasses().size());
        }
    }

    void RenderTechniqueData::AddPassFront(BasePass::Ptr pPass) {
        if (SRVerify(pPass)) {
            if (!pass) {
                pass = new GroupPass();
            }
            pass.StaticCast<GroupPass>()->InsertPass(pPass, 0);
        }
    }

    IRenderTechnique::IRenderTechnique()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    IRenderTechnique::~IRenderTechnique() {
        SRAssert(m_attachedRenderTargets.empty());
        m_data.pass.AutoFree();
        ReleaseFrameBuffers();
    }

    bool IRenderTechnique::Render() {
        SR_TRACY_ZONE;

        if (m_dirty || !m_data.pass || !m_data.pass->IsActive()) {
            return false;
        }

        bool hasDrawData = false;

        hasDrawData |= m_data.pass->PreRender();
        hasDrawData |= m_data.pass->Render();
        hasDrawData |= m_data.pass->PostRender();

        return hasDrawData;
    }

    void IRenderTechnique::PrepareFrame() {
        SR_TRACY_ZONE;

        UpdateDataIfNeeded();

        if (!m_data.pass) {
            return;
        }

        if (!BuildTechnique() || !m_data.pass->IsActive()) {
            return;
        }
    }

    void IRenderTechnique::PrepareRender() {
        SR_TRACY_ZONE;

        m_data.pass->Prepare();

        UpdateFrustumCulling();
    }

    void IRenderTechnique::Update() {
        SR_TRACY_ZONE;

        if (!m_data.pass || m_dirty || !m_data.pass->IsActive()) {
            return;
        }

        m_data.pass->Update();
    }

    void IRenderTechnique::PostUpdate() {
        SR_TRACY_ZONE;

        if (!m_data.pass || m_dirty || !m_data.pass->IsActive()) {
            return;
        }

        m_data.pass->PostUpdate();
    }

    bool IRenderTechnique::Overlay() {
        SR_TRACY_ZONE;

        if (m_dirty) {
            return false;
        }

        return m_data.pass->Overlay();
    }

    void IRenderTechnique::KillTechnique() {
        SR_TRACY_ZONE;

        SRAssert(!m_isDead);
        m_isDead = true;

        if (!m_attachedRenderTargets.empty()) {
            SR_LOG("IRenderTechnique::KillTechnique() : technique \"{}\" has attached render targets, deactivating them...", m_data.name);
            while (!m_attachedRenderTargets.empty()) {
                (*m_attachedRenderTargets.begin())->Deactivate();
            }
        }

        if (!IsGraphicsResourceRegistered()) {
            AutoFree();
            return;
        }
    }

    void IRenderTechnique::SetDirty() {
        m_dirty = true;

        /// Авось что-то изменилось, нужно попробовать еще раз сбилдить
        m_hasErrors = false;

        if (m_renderScene) {
            m_renderScene->SetDirty();
        }
    }

    void IRenderTechnique::SetCamera(IRenderTechnique::CameraPtr pCamera) {
        if (!pCamera) {
            SRHalt("IRenderTechnique::SetCamera() : pCamera is nullptr!");
            return;
        }

        m_camera = pCamera;
        OnResize(m_camera->GetViewportSize());
        SetRenderScene(m_camera->GetRenderScene());
    }

    void IRenderTechnique::SetRenderScene(const IRenderTechnique::RenderScenePtr& pRScene) {
        if (!m_renderScene) {
            m_renderScene = pRScene;
            RegisterGraphicsResource();
        }
        else {
            SRHalt("RenderTechnique already has a render scene!");
        }
    }

    void IRenderTechnique::FreeVMemory() {
        if (m_data.pass && m_data.pass->IsInit()) {
            m_data.pass->DeInit();
        }
        ReleaseFrameBuffers();
        Memory::IGraphicsResource::FreeVMemory();
    }

    bool IRenderTechnique::IsEmpty() const {
        return !m_data.pass && m_data.frameBuffers.empty();
    }

    void IRenderTechnique::DeInitPasses() {
        if (m_data.pass && m_data.pass->IsInit()) {
            m_data.pass->DeInit();
        }
        m_data.pass.AutoFree();
        ReleaseFrameBuffers();
    }

    bool IRenderTechnique::BuildTechnique() {
        if (!m_dirty) {
            return true;
        }

        if (m_hasErrors) {
            return false;
        }

        if (!m_data.pass) {
            SR_WARN("IRenderTechnique::BuildTechnique() : technique \"{}\" does not have a passes!", m_data.name);
            m_hasErrors = true;
            return false;
        }

        if (!Init()) {
            SR_ERROR("IRenderTechnique::BuildTechnique() : failed to initialize technique \"{}\"!", m_data.name);
            m_hasErrors = true;
            return false;
        }

        m_dirty = false;
        return true;
    }

    void IRenderTechnique::OnResize(const SR_MATH_NS::UVector2& size) {
        if (m_surfaceSize && *m_surfaceSize == size) {
            return;
        }

        m_surfaceSize = size;

        SR_LOG("IRenderTechnique::OnResize() : resizing technique \"{}\" to {}x{}", m_data.name, size.x, size.y);

        for (auto&& pController : m_data.frameBuffers) {
            pController->OnResize(size);
        }

        if (m_data.pass) {
            m_data.pass->OnResize(size);
        }
    }

    void IRenderTechnique::OnMultisampleChanged() {
        if (m_data.pass) {
            m_data.pass->OnMultisampleChanged();
        }
    }

    void IRenderTechnique::ForEachPass(const SR_HTYPES_NS::Function<void(BasePass&)>& func) {
        if (m_data.pass) {
            m_data.pass->ForEachPass(func);
        }
    }

    const SR_UTILS_NS::Vector<FrameBufferController::Ptr>& IRenderTechnique::GetFrameBufferControllers() const {
        return m_data.frameBuffers;
    }

    const FrameBufferController::Ptr& IRenderTechnique::GetFrameBufferController(SR_UTILS_NS::StringAtom name) const {
        for (auto&& pController : m_data.frameBuffers) {
            if (pController->GetName() == name) {
                return pController;
            }
        }
        static const FrameBufferController::Ptr emptyPtr;
        return emptyPtr;
    }

    bool IRenderTechnique::Init() {
        SR_TRACY_ZONE;

        //if (!m_modulesApplied && m_camera && GetRenderScene()) {
        //    //SR_GRAPH_NS::Details::PostProcessRenderTechnique(this, GetRenderScene()->GetContext(), m_camera->GetCameraType());
        //    m_modulesApplied = true;
        //}

        for (auto&& pController : m_data.frameBuffers) {
            if (!pController->InitializeFramebuffer(GetRenderContext())) {
                SR_ERROR("RenderTechnique::Init() : failed to initialize \"{}\" framebuffer controller!", pController->GetName());
                m_hasErrors = true;
                return false;
            }

            if (m_surfaceSize) {
                pController->OnResize(*m_surfaceSize);
            }
        }

        if (m_data.pass && m_data.pass->IsInit()) {
            m_data.pass->DeInit();
        }

        if (m_data.pass && !m_data.pass->PreInit()) {
            SR_ERROR("RenderTechnique::Init() : failed to pre-initialize pass \"{}\"!", m_data.pass->GetPassName());
            m_hasErrors = true;
            return false;
        }

        if (m_data.pass && !m_data.pass->Init()) {
            SR_ERROR("RenderTechnique::Init() : failed to initialize pass \"{}\"!", m_data.pass->GetPassName());
            m_hasErrors = true;
            return false;
        }

        return true;
    }

    void IRenderTechnique::UpdateFrustumCulling() {
        SR_TRACY_ZONE;

        if (!m_camera || !GetRenderContext()->IsFrustumCullingEnabled()) {
            return;
        }

        if (m_data.pass && m_data.pass->UpdateFrustum()) {
            GetPipeline()->SetDirty(true);
        }
    }

    void IRenderTechnique::ReleaseFrameBuffers() {
        for (auto&& pController : m_data.frameBuffers) {
            pController.AutoFree();
        }
        m_data.frameBuffers.clear();
    }

    bool IRenderTechnique::IsTechniqueDead() const {
        return m_isDead;
    }

    void IRenderTechnique::OnHierarchyChanged() {
        if (auto&& pContext = GetRenderContext()) {
            pContext->SetDirty();
        }

        if (m_data.pass) {
            m_data.pass->SetRenderTechnique(this);
            m_data.pass->SetParent(nullptr);
        }

        m_dirty = true;
    }

    void IRenderTechnique::SetRenderTechniqueData(RenderTechniqueData&& data) {
        SR_TRACY_ZONE;
        DeInitPasses();
        m_hasErrors = false;
        m_data = std::move(data);
        //m_modulesApplied = false;

        OnHierarchyChanged();
    }

    void IRenderTechnique::AttachRenderTarget(SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::RenderTarget> pRenderTarget) {
        if (SRVerify(pRenderTarget)) {
            if (!m_attachedRenderTargets.insert(pRenderTarget).second) {
                SRHalt("IRenderTechnique::AttachRenderTarget() : render target \"{}\" is already attached to technique \"{}\"!", pRenderTarget->GetName(), m_data.name);
            }
        }
        OnHierarchyChanged();
    }

    void IRenderTechnique::DetachRenderTarget(SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::RenderTarget> pRenderTarget) {
        if (SRVerify(pRenderTarget))  {
            if (m_attachedRenderTargets.erase(pRenderTarget) == 0) {
                SRHalt("IRenderTechnique::DetachRenderTarget() : render target \"{}\" is not attached to technique \"{}\"!", pRenderTarget->GetName(), m_data.name);
            }
        }
        OnHierarchyChanged();
    }

    BasePass* IRenderTechnique::FindPass(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;
        return m_data.pass ? m_data.pass->FindPass(name) : nullptr;
    }

    void IRenderTechnique::OnCameraParamsChanged() {
        SR_TRACY_ZONE;
        if (m_data.pass) {
            m_data.pass->OnCameraParamsChanged();
        }
    }
}