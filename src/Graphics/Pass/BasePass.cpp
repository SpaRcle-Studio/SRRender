//
// Created by Monika on 17.07.2022.
//

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Camera.h>

#include <Codegen/BasePass.generated.hpp>

namespace SR_GRAPH_NS {
    BasePass::BasePass()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_uboManager(Memory::UBOManager::Instance())
        , m_descriptorManager(DescriptorManager::Instance())
    { }

    void BasePass::OnMultisampleChanged() {

    }

    void BasePass::OnResize(const SR_MATH_NS::UVector2& size) {

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
        SRAssert2(m_isInit, "Pass isn't initialized!");

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

    void BasePass::UseSamplers(SR_GTYPES_NS::Shader* pShader) {

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

    void BasePass::ForEachPass(const std::function<void(BasePass&)>& func) {
        func(*this);
    }
}