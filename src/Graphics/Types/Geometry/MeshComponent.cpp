//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Types/Geometry/MeshComponent.h>
#include <Graphics/Utils/MeshUtils.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform2D.h>

namespace SR_GTYPES_NS {
    IMeshComponent::IMeshComponent(Mesh* pMesh)
        : Super()
        , m_pInternal(pMesh)
    {
        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("MeshInv")
            .SetLabel("Invalid mesh!")
            .SetColor(SR_MATH_NS::FColor(1.f, 0.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !m_pInternal->IsCalculatable(); })
            .SetDontSave();

        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("MeshNotCalc")
            .SetLabel("Mesh isn't calculated!")
            .SetColor(SR_MATH_NS::FColor(1.f, 1.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !m_pInternal->IsCalculated(); })
            .SetDontSave();

        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("MeshNotReg")
            .SetLabel("Mesh isn't registered!")
            .SetColor(SR_MATH_NS::FColor(1.f, 1.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !m_pInternal->IsGraphicsResourceRegistered(); })
            .SetDontSave();
    }

    void IMeshComponent::OnDestroy() {
        Super::OnDestroy();
        m_pInternal->DestroyMesh();
    }

    bool IMeshComponent::ExecuteInEditMode() const {
        return true;
    }

    void IMeshComponent::OnMatrixDirty() {
        if (auto&& pTransform = GetTransform()) {
            m_pInternal->SetMatrix(pTransform->GetMatrix());
        }
        else {
            m_pInternal->SetMatrix(SR_MATH_NS::Matrix4x4::Identity());
        }

        m_pInternal->MarkUniformsDirty();

        Super::OnMatrixDirty();
    }

    bool IMeshComponent::InitializeEntity() noexcept {
        m_properties.AddEnumProperty<MeshType>("Mesh type")
            .SetGetter([this] {
                return SR_UTILS_NS::EnumReflector::ToStringAtom(m_pInternal->GetMeshType());
            })
            .SetReadOnly();

        m_properties.AddExternalProperty(&m_pInternal->GetMaterialProperty());

        return Super::InitializeEntity();
    }

    void IMeshComponent::OnLayerChanged() {
        m_pInternal->ReRegisterMesh();
        Super::OnLayerChanged();
    }

    void IMeshComponent::OnPriorityChanged() {
        m_pInternal->ReRegisterMesh();
        Super::OnPriorityChanged();
    }

    void IMeshComponent::OnEnable() {
        Super::OnEnable();
        if (!m_pInternal->IsMeshRegistered()) {
            if (auto&& pRenderScene = GetRenderScene()) {
                pRenderScene->Register(m_pInternal);
            }
        }
    }

    void IMeshComponent::OnDisable() {
        Super::OnDisable();
        m_pInternal->UnRegisterMesh();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    MeshComponent::MeshComponent(MeshType type)
        : IMeshComponent(this)
        , Mesh(type)
    { }

    int64_t MeshComponent::GetSortingPriority() const {
        if (m_gameObject) {
            if (GetTransform()->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                return static_cast<SR_UTILS_NS::Transform2D*>(m_gameObject->GetTransform())->GetPriority();
            }
        }

        return -1;
    }

    bool MeshComponent::HasSortingPriority() const {
        if (!m_gameObject) {
            return false;
        }

        return GetTransform()->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
    }

    SR_UTILS_NS::StringAtom MeshComponent::GetMeshLayer() const {
        if (!m_gameObject) {
            return SR_UTILS_NS::StringAtom();
        }

        return m_gameObject->GetLayer();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    IndexedMeshComponent::IndexedMeshComponent(MeshType type)
        : IMeshComponent(this)
        , IndexedMesh(type)
    { }

    int64_t IndexedMeshComponent::GetSortingPriority() const {
        if (m_gameObject) {
            if (GetTransform()->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                return static_cast<SR_UTILS_NS::Transform2D*>(m_gameObject->GetTransform())->GetPriority();
            }
        }

        return -1;
    }

    bool IndexedMeshComponent::HasSortingPriority() const {
        if (!m_gameObject) {
            return false;
        }

        return GetTransform()->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
    }

    SR_UTILS_NS::StringAtom IndexedMeshComponent::GetMeshLayer() const {
        if (!m_gameObject) {
            return SR_UTILS_NS::StringAtom();
        }

        return m_gameObject->GetLayer();
    }
}