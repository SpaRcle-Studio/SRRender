//
// Created by Monika on 17.07.2022.
//

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Pass/SwapchainPass.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Framebuffer.h>

#include <Codegen/BasePass.generated.hpp>

namespace SR_GRAPH_NS {
    BasePass::BasePass()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_uboManager(&Memory::UBOManager::Instance())
        , m_descriptorManager(&DescriptorManager::Instance())
    { }

    void BasePass::OnMultisampleChanged() {

    }

    void BasePass::OnCameraParamsChanged() {

    }

    void BasePass::OnResize(const SR_MATH_NS::UVector2& size) {

    }

    FrameBufferPass* BasePass::GetFrameBufferPass() const {
        static const auto name = FrameBufferPass::GetClassStaticName();

        BasePass* pParent = GetParent();
        while (pParent) {
            if (pParent->GetMeta()->IsSameOrInherited(name)) {
                return static_cast<FrameBufferPass*>(pParent);
            }
            pParent = pParent->GetParent();
        }

        return nullptr;
    }

    bool BasePass::PreInit() {
        return true;
    }

    bool BasePass::Init() {
        SRAssert2(!m_isInit, "Pass already initialized!");

        m_isInit = true;

        return true;
    }

    bool BasePass::Prepare() {
        return true;
    }

    void BasePass::DeInit() {
        //SRAssert2(m_isInit, "Pass isn't initialized!");
        m_isInit = false;
    }

    const BasePass::RenderScenePtr& BasePass::GetRenderScene() const {
        static RenderScenePtr nullScene = nullptr;
        return m_technique ? m_technique->GetRenderScene() : nullScene;
    }


    const BasePass::CameraPtr& BasePass::GetCamera() const {
        static CameraPtr nullCamera = nullptr;
        return m_technique ? m_technique->GetCamera() : nullCamera;
    }

    const BasePass::RenderContextPtr& BasePass::GetRenderContext() const {
        static BasePass::RenderContextPtr nullContext = nullptr;
        return m_technique ? m_technique->GetRenderContext() : nullContext;
    }

    const BasePass::PipelinePtr& BasePass::GetPipeline() const {
        static PipelinePtr nullPipeline = nullptr;
        return m_technique ? m_technique->GetPipeline() : nullPipeline;
    }

    void BasePass::SetRenderTechnique(IRenderTechnique* pRenderTechnique) {
        SRAssert(pRenderTechnique);
        m_technique = pRenderTechnique;
    }

    void BasePass::UseSamplers(SR_GTYPES_NS::Shader& shader) {

    }

    bool BasePass::IsActive() const {
        return !GetCamera() || GetCamera()->IsActive();
    }

    SR_UTILS_NS::StringAtom BasePass::GetPassName() const {
        return m_customName.Empty() ? GetMeta()->GetFactoryName() : m_customName;
    }

    BasePass* BasePass::FindPass(SR_UTILS_NS::StringAtom name) {
        return GetPassName() == name ? this : nullptr;
    }

    void BasePass::ForEachPass(const SR_HTYPES_NS::Function<void(BasePass&)>& func) {
        func(*this);
    }

    uint32_t BasePass::GetColorLayersCount() const {
        SR_TRACY_ZONE;

        if (auto&& pFrameBufferPass = GetFrameBufferPass()) {
            if (auto&& pFrameBuffer = pFrameBufferPass->GetFrameBufferPassData().GetFramebuffer()) {
                return SR_MIN(SR_SRSL_NS::SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO.size(), pFrameBuffer->GetColorLayersCount());
            }
            else {
                SR_ERROR("MeshDrawerPass::Init() : framebuffer is null in \"{}\" pass!", pFrameBufferPass->GetPassName());
                return false;
            }
        }

        static const auto name = SwapchainPass::GetClassStaticName();

        BasePass* pParent = GetParent();
        while (pParent) {
            if (pParent->GetMeta()->IsSameOrInherited(name)) {
                return 1;
            }
            pParent = pParent->GetParent();
        }

        return 0;
    }
}