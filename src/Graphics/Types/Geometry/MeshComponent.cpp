//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Types/Geometry/MeshComponent.h>
#include <Graphics/Utils/MeshUtils.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform2D.h>

namespace SR_GTYPES_NS {
    MeshComponent::MeshComponent(MeshType type)
        : IndexedMesh(type)
        , SR_GTYPES_NS::IRenderComponent()
    {
        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("MeshInv")
            .SetLabel("Invalid mesh!")
            .SetColor(SR_MATH_NS::FColor(1.f, 0.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !IsCalculatable(); })
            .SetDontSave();

        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("MeshNotCalc")
            .SetLabel("Mesh isn't calculated!")
            .SetColor(SR_MATH_NS::FColor(1.f, 1.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !IsCalculated(); })
            .SetDontSave();

        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("MeshNotReg")
            .SetLabel("Mesh isn't registered!")
            .SetColor(SR_MATH_NS::FColor(1.f, 1.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !IsGraphicsResourceRegistered(); })
            .SetDontSave();
    }

    void MeshComponent::OnLoaded() {
        Component::OnLoaded();
    }

    void MeshComponent::OnAttached() {
        Component::OnAttached();
    }

    void MeshComponent::OnDestroy() {
        Component::OnDestroy();
        DestroyMesh();
    }

    bool MeshComponent::ExecuteInEditMode() const {
        return true;
    }

    SR_MATH_NS::FVector3 MeshComponent::GetBarycenter() const {
        return m_barycenter;
    }

    void MeshComponent::OnMatrixDirty() {
        if (auto&& pTransform = GetTransform()) {
            m_modelMatrix = pTransform->GetMatrix();
            m_translation = pTransform->GetTranslation();
        }
        else {
            m_modelMatrix = SR_MATH_NS::Matrix4x4::Identity();
            m_translation = SR_MATH_NS::FVector3::Zero();
        }

        MarkUniformsDirty();

        Component::OnMatrixDirty();
    }

    int64_t MeshComponent::GetSortingPriority() const {
        if (!m_gameObject) {
            return -1;
        }

        if (auto&& pTransform = dynamic_cast<SR_UTILS_NS::Transform2D*>(m_gameObject->GetTransform())) {
            return pTransform->GetPriority();
        }

        return -1;
    }

    bool MeshComponent::InitializeEntity() noexcept {
        m_properties.AddEnumProperty("Mesh type", &m_meshType).SetReadOnly();
        m_properties.AddExternalProperty(&m_materialProperty);
        return Entity::InitializeEntity();
    }

    void MeshComponent::OnLayerChanged() {
        ReRegisterMesh();
        IRenderComponent::OnLayerChanged();
    }

    bool MeshComponent::HasSortingPriority() const {
        if (!m_gameObject) {
            return false;
        }

        return m_gameObject->GetTransform()->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
    }

    SR_UTILS_NS::StringAtom MeshComponent::GetMeshLayer() const {
        if (!m_gameObject) {
            return SR_UTILS_NS::StringAtom();
        }

        return m_gameObject->GetLayer();
    }

    void MeshComponent::OnEnable() {
        IRenderComponent::OnEnable();
        if (auto&& pRenderScene = GetRenderScene(); pRenderScene && !IsMeshRegistered()) {
            pRenderScene->Register(this);
        }
    }

    void MeshComponent::OnDisable() {
        IRenderComponent::OnDisable();
        UnRegisterMesh();
    }
}