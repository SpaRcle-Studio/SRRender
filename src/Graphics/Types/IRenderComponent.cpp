//
// Created by Monika on 22.05.2023.
//

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Memory/DescriptorManager.h>

#include <Utils/World/Scene.h>
#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/TransformRect.h>

#include <Codegen/IRenderComponent.generated.hpp>

namespace SR_GTYPES_NS {
    IRenderComponent::~IRenderComponent() {
        SRAssert(!m_registrationInfo.has_value());
        SRAssert2(!m_isWaitReRegister, "Application may will crash if you delete mesh with waiting re-register!");
    }

    void IRenderComponent::OnEnable() {
        if (!IsRenderObjectRegistered()) {
            ReRegisterRenderObject();
        }
        MarkUniformsDirty();
        Super::OnEnable();
    }

    void IRenderComponent::OnDisable() {
        UnRegisterRenderObject();
        Super::OnDisable();
    }

    IRenderComponent::CameraPtr IRenderComponent::GetCamera() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene->GetMainCamera();
        }
        return CameraPtr();
    }

    RenderScene* IRenderComponent::TryGetRenderScene() const {
        if (m_renderScene) {
            return m_renderScene;
        }

        auto&& pScene = TryGetScene();
        if (!pScene) {
            return m_renderScene;
        }

        m_renderScene = pScene->GetDataStorage().GetPointer<RenderScene>();

        return m_renderScene;
    }

    RenderScene* IRenderComponent::GetRenderScene() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene;
        }
        SRHalt("Invalid render scene!");
        return nullptr;
    }

    std::optional<int32_t> IRenderComponent::GetIBO() const {
        return std::nullopt;
    }

    std::optional<int32_t> IRenderComponent::GetVBO() const {
        return std::nullopt;
    }

    void IRenderComponent::SetRegistrationInfo(const std::optional<RenderObjectRegistrationInfoInternal>& info) {
        m_registrationInfo = info;
    }

    void IRenderComponent::OnMatrixDirty() {
        MarkUniformsDirty();
        m_isAABBDirty = true;
        Super::OnMatrixDirty();
    }

    void IRenderComponent::OnDestroy() {
        m_isDestroyingState = true;

        if (IsRenderObjectRegistered()) {
            GetRenderScene()->Remove(this);
        }

        FreeVideoMemory();
        SetMaterial(SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::BaseMaterial>());

        Super::OnDestroy();
    }

    RenderObjectRegistrationInfo IRenderComponent::CreateRegistrationInfo() const {
        SR_TRACY_ZONE;

        RenderObjectRegistrationInfo info = { };
        info.pObject = const_cast<IRenderComponent*>(this);
        info.internal.pMaterial = const_cast<BaseMaterial*>(GetMaterial().Get());
        info.internal.VBO = GetVBO();

        if (auto&& pSO = GetSceneObject()) {
            info.internal.layer = pSO->GetLayer();
        }

        if (auto&& pRectTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            info.internal.priority = pRectTransform->GetPriority();
        }

        return info;
    }

    void IRenderComponent::OnLayerChanged() {
        ReRegisterRenderObject();
        Super::OnLayerChanged();
    }

    void IRenderComponent::OnPriorityChanged() {
        ReRegisterRenderObject();
        Super::OnPriorityChanged();
    }

    bool IRenderComponent::IsRenderObjectRegistered() const noexcept {
        return m_registrationInfo.has_value();
    }

    const RenderObjectRegistrationInfoInternal& IRenderComponent::GetRegistrationInfo() const noexcept {
        return m_registrationInfo.value();
    }

    void IRenderComponent::OnReRegistered() {
        m_isWaitReRegister = false;
    }

    void IRenderComponent::UnRegisterRenderObject() {
        if (IsRenderObjectRegistered()) {
            GetRenderScene()->Remove(this);
        }
    }

    void IRenderComponent::ReRegisterRenderObject() {
        SR_TRACY_ZONE;

        if (!IsActive()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (m_isDestroyingState) {
            return;
        }

        auto&& pRenderScene = TryGetRenderScene();
        if (IsRenderObjectRegistered()) {
            if (m_isWaitReRegister) {
                return;
            }

            m_isWaitReRegister = true;

            RenderObjectRegistrationInfo info;
            info.internal = m_registrationInfo.value();
            info.pObject = this;
            pRenderScene->ReRegister(info);
        }
        else if (pRenderScene) {
            pRenderScene->Register(this);
        }
    }

    const SR_MATH_NS::AABB& IRenderComponent::GetAABB() const {
        auto&& internalData = GetInternalData();
        if (m_isAABBDirty) {
            if (auto&& pTransform = GetTransform()) {
                internalData.aabb = pTransform->GetAABB();
                m_isAABBDirty = false;
            }
        }
        return internalData.aabb;
    }

    void IRenderComponent::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;
        m_material->UseSamplers(shader);
    }

    void IRenderComponent::MarkMaterialDirty() {
        m_dirtyMaterial = true;
    }

    void IRenderComponent::SetMaterial(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        SetMaterial(FileMaterial::Load(path));
    }

    void IRenderComponent::SetMaterial(const SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::BaseMaterial>& pMaterial) {
        SR_TRACY_ZONE;

        if (pMaterial == m_material) {
            return;
        }

        MarkMaterialDirty();
        m_hasErrors = false;

        auto&& internalData = GetInternalData();
        if (m_material) {
            m_material->Unregister(&internalData.materialRegisterId);
        }

        m_material = pMaterial;

        if (m_material) {
            internalData.materialRegisterId = m_material->Register(this);
        }

        if (IsRenderObjectRegistered()) {
            ReRegisterRenderObject();
        }
    }

    void IRenderComponent::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        m_material->Use(shader);
    }

    void IRenderComponent::MarkUniformsDirty() {
        SR_TRACY_ZONE;

        if (!IsActive()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        auto&& renderQueues = GetRenderQueues();
        auto pStart = renderQueues.data();
        auto pEnd = pStart + renderQueues.size();
        for (auto pElement = pStart; pElement != pEnd; ++pElement) {
            if (!pElement->pRenderQueue) SR_LIKELY_ATTRIBUTE {
                continue;
            }

            if (!pElement->isVisible) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            pElement->dirtyUniformsFrames.set();
            if (pElement->inUpdateQueue) {
                continue;
            }
            pElement->inUpdateQueue = true;
            pElement->pRenderQueue->OnObjectDirty(this, pElement);
        }
    }

    MeshRenderQueues& IRenderComponent::GetRenderQueues() noexcept {
        return GetInternalData().renderQueues;
    }

    IRenderComponentInternalData& IRenderComponent::GetInternalData() const {
        if (!m_internalData) {
            m_internalData = new IRenderComponentInternalData();
        }
        return *m_internalData;
    }

    void IRenderComponent::SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description) {
        GetInternalData().vertexLayoutDescription = description;
    }

    const SR_UTILS_NS::VertexLayoutDescription& IRenderComponent::GetVertexLayoutDescription() const noexcept {
        return GetInternalData().vertexLayoutDescription;
    }

    Pipeline* IRenderComponent::GetPipeline() const noexcept {
        return const_cast<Pipeline*>(GetRenderScene()->GetPipelineRef().Get());
    }

    Pipeline* IRenderComponent::TryGetPipeline() const noexcept {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return const_cast<Pipeline*>(pRenderScene->GetPipelineRef().Get());
        }
        return nullptr;
    }

    const SR_UTILS_NS::VertexLayoutDescription& IRenderComponent::GetShaderVertexLayoutDescription() const noexcept {
        static SR_UTILS_NS::VertexLayoutDescription emptyDescription;
        return emptyDescription;
    }

    const SR_UTILS_NS::UI::MaskInfo& IRenderComponent::GetMaskInfo() const {
        static SR_UTILS_NS::UI::MaskInfo emptyMaskInfo;
        return emptyMaskInfo;
    }

    /// ================================================================================================================

    UIRenderComponent::~UIRenderComponent() {
        SRAssert(m_virtualUBO == SR_ID_INVALID);
        SRAssert(m_virtualDescriptor == SR_ID_INVALID);
    }

    void UIRenderComponent::OnMaskDirty() {
        Super::OnMaskDirty();
        m_isMaskDirty = true;
    }

    const SR_UTILS_NS::UI::MaskInfo& UIRenderComponent::GetMaskInfo() const {
        if (m_isMaskDirty) {
            if (auto&& pTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
                m_maskInfo = pTransform->GetMaskInfo();
                m_isMaskDirty = false;
            }
        }
        return m_maskInfo;
    }

    bool UIRenderComponent::IsActive() const noexcept {
        return Super::IsActive() && !m_hasErrors;
    }

    void UIRenderComponent::FreeVideoMemory() {
        if (auto&& pPipeline = TryGetPipeline()) {
            pPipeline->GetUBOManager().TryFreeUBO(&m_virtualUBO);
            pPipeline->GetDescriptorManager().TryFreeDescriptorSet(&m_virtualDescriptor);
        }
        Super::FreeVideoMemory();
    }
}