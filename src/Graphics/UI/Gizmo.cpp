//
// Created by Monika on 23.11.2023.
//

#include <Graphics/Material/UniqueMaterial.h>
#include <Graphics/UI/Gizmo.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Render/RenderTechnique.h>

#include <Utils/ECS/ComponentManager.h>
#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform3D.h>
#include <Utils/Input/InputSystem.h>
#include <Utils/DebugDraw.h>

namespace SR_GRAPH_UI_NS {
    void Gizmo::OnAttached() {
        Super::OnAttached();
    }

    void Gizmo::OnDestroy() {
        ReleaseGizmo();

        Super::OnDestroy();

        GetThis().AutoFree([](auto&& pData) {
            delete pData;
        });
    }

    void Gizmo::LoadMesh(GizmoOperationFlag operation, SR_UTILS_NS::StringAtom path, SR_UTILS_NS::StringAtom name, GizmoMeshLoadMode mode) { /// NOLINT
        SR_TRACY_ZONE;

        if (!SR_MATH_NS::IsMaskIncludedSubMask(m_operation, operation)) {
            return;
        }

        if (mode == GizmoMeshLoadMode::All) {
            LoadMesh(operation, path, name, GizmoMeshLoadMode::Visual);
            LoadMesh(operation, path, name, GizmoMeshLoadMode::Selection);
            return;
        }

        if (!GetGameObject()) {
            return;
        }

        auto&& pMesh = SR_GTYPES_NS::Mesh::Load(path, MeshType::Static, name);
        if (!pMesh) {
            SR_ERROR("Gizmo::LoadMesh() : failed to load mesh!\n\tPath: {}\n\tName: {}", path.ToStringRef(), name.ToStringRef());
            return;
        }

        auto&& pMeshComponent = dynamic_cast<SR_GTYPES_NS::IndexedMeshComponent*>(pMesh);
        if (!pMeshComponent) {
            SRHalt("Failed to cast!");
            return;
        }

        pMeshComponent->AddSerializationFlags(SR_UTILS_NS::ObjectSerializationFlags::DontSave);

        auto&& pMaterial = new SR_GRAPH_NS::UniqueMaterial();
        pMaterial->SetShader("Engine/Shaders/Gizmo/gizmo.srsl");
        pMaterial->SetVec4("color", GetColorByOperation(operation));

        pMesh->SetMaterial(pMaterial);

        if (mode == GizmoMeshLoadMode::Visual) {
            if (SR_MATH_NS::IsMaskIncludedSubMask(operation, GizmoOperation::Rotate)) {
                auto&& gameObject = GetGameObjectByOperation(mode, operation);
                gameObject->AddComponent(pMeshComponent);
                gameObject->SetLayer("Gizmo");
            }
            else {
                GetGameObject()->AddComponent(pMeshComponent);
            }
            m_meshes[operation].pVisual = pMeshComponent;
        }
        else if (mode == GizmoMeshLoadMode::Selection) {
            if (SR_MATH_NS::IsMaskIncludedSubMask(operation, GizmoOperation::Rotate)) {
                auto&& gameObject = GetGameObjectByOperation(mode, operation);
                gameObject->AddComponent(pMeshComponent);
                gameObject->SetLayer("GizmoSelection");
            }
            else {
                GetGameObject()->GetOrCreateChild("Selection")->AddComponent(pMeshComponent);
            }
            m_meshes[operation].pSelection = pMeshComponent;
        }
        else {
            SRHalt("Unresolved situation!");
        }
    }

    void Gizmo::OnEnable() {
        m_isGizmoDirty = true;
        Super::OnEnable();
    }

    void Gizmo::LoadGizmo() {
        if (!m_isGizmoDirty) {
            return;
        }

        ReleaseGizmo();

        SRAssert(m_meshes.empty());

        GetGameObject()->SetLayer("Gizmo");
        GetGameObject()->GetOrCreateChild("Selection")->SetLayer("GizmoSelection");
        SR_MAYBE_UNUSED auto&& debugObject = GetGameObject()->GetOrCreateChild("Debug");

        static const SR_UTILS_NS::StringAtom gizmoFile = "Engine/Models/gizmo-translation.fbx";

        LoadMesh(GizmoOperation::TranslateCenter, gizmoFile, "CenterTranslation", GizmoMeshLoadMode::Visual);
        LoadMesh(GizmoOperation::TranslateCenter, gizmoFile, "CenterTranslationSelection", GizmoMeshLoadMode::Selection);

        LoadMesh(GizmoOperation::TranslateAltX, gizmoFile, "PlaneX", GizmoMeshLoadMode::All);
        LoadMesh(GizmoOperation::TranslateAltY, gizmoFile, "PlaneY", GizmoMeshLoadMode::All);
        LoadMesh(GizmoOperation::TranslateAltZ, gizmoFile, "PlaneZ", GizmoMeshLoadMode::All);

        LoadMesh(GizmoOperation::TranslateX, gizmoFile, "ArrowX", GizmoMeshLoadMode::Visual);
        LoadMesh(GizmoOperation::TranslateY, gizmoFile, "ArrowY", GizmoMeshLoadMode::Visual);
        LoadMesh(GizmoOperation::TranslateZ, gizmoFile, "ArrowZ", GizmoMeshLoadMode::Visual);

        LoadMesh(GizmoOperation::TranslateX, gizmoFile, "ArrowXSelection", GizmoMeshLoadMode::Selection);
        LoadMesh(GizmoOperation::TranslateY, gizmoFile, "ArrowYSelection", GizmoMeshLoadMode::Selection);
        LoadMesh(GizmoOperation::TranslateZ, gizmoFile, "ArrowZSelection", GizmoMeshLoadMode::Selection);

        if (!SR_MATH_NS::IsMaskIncludedSubMask(m_operation, GizmoOperation::Rotate2D)) {
            LoadMesh(GizmoOperation::RotateX, gizmoFile, "RotateX", GizmoMeshLoadMode::All);
            LoadMesh(GizmoOperation::RotateY, gizmoFile, "RotateY", GizmoMeshLoadMode::All);
            LoadMesh(GizmoOperation::RotateZ, gizmoFile, "RotateZ", GizmoMeshLoadMode::All);

            LoadMesh(GizmoOperation::RotateCenter, gizmoFile, "RotateCenter", GizmoMeshLoadMode::All);
        }

        LoadMesh(GizmoOperation::Rotate2D, gizmoFile, "Rotate2D", GizmoMeshLoadMode::All);

        LoadMesh(GizmoOperation::ScaleCenter, gizmoFile, "CenterScale", GizmoMeshLoadMode::Visual);
        LoadMesh(GizmoOperation::ScaleCenter, gizmoFile, "CenterScaleSelection", GizmoMeshLoadMode::Selection);

        LoadMesh(GizmoOperation::ScaleX, gizmoFile, "ScaleX", GizmoMeshLoadMode::All);
        LoadMesh(GizmoOperation::ScaleY, gizmoFile, "ScaleY", GizmoMeshLoadMode::All);
        LoadMesh(GizmoOperation::ScaleZ, gizmoFile, "ScaleZ", GizmoMeshLoadMode::All);

        m_isGizmoDirty = false;
    }

    void Gizmo::ReleaseGizmo() {
        for (auto&& [operation, info] : m_meshes) {
            if (info.pSelection) {
                info.pSelection->Detach();
            }
            if (info.pVisual) {
                info.pVisual->Detach();
            }
        }

        m_meshes.clear();

        if (auto&& pGameObject = GetGameObject()) {
            for (auto&& pChild : pGameObject->GetChildren()) {
                if (pChild->GetName() == "Debug") {
                    continue;
                }
                pChild->Destroy();
            }
        }

        m_isGizmoDirty = true;
    }

    void Gizmo::OnDisable() {
        ReleaseGizmo();
        Super::OnDisable();
    }

    void Gizmo::FixedUpdate() {
        SR_TRACY_ZONE;

        if (!IsGizmoAvailable()) {
            ReleaseGizmo();
            return;
        }

        PrepareGizmo();
        LoadGizmo();

        auto&& pCamera = GetCamera();
        if (!pCamera) {
            return;
        }

        auto&& pTechnique = pCamera->GetRenderTechnique();
        if (!pTechnique) {
            return;
        }

        UpdateGizmoTransform();

        if (m_activeOperation != GizmoOperation::None && !SR_UTILS_NS::Input::Instance().GetMouse(SR_UTILS_NS::MouseCode::MouseLeft)) {
            EndGizmo();
            m_activeOperation = GizmoOperation::None;
        }

        auto&& mousePos = GetCamera()->GetMousePos();
        if (!m_lastMousePos.IsFinite()) {
            m_lastMousePos = mousePos;
        }

        if (IsLocal()) {
            m_modelMatrix = m_modelMatrix.OrthogonalNormalize(); /// normalized for local scape
        }

        m_hoveredOperation = GizmoOperation::None;

        for (auto&& [flag, info]: m_meshes) {
            if (info.pVisual) {
                info.pVisual->GetMaterial()->SetBool("useOrthogonal", IsGizmo2DSpace());
            }

            if (info.pSelection) {
                info.pSelection->GetMaterial()->SetBool("useOrthogonal", IsGizmo2DSpace());
            }
        }

        if (m_activeOperation == GizmoOperation::None) {
            auto&& pMesh = pTechnique->PickMeshAt(mousePos);
            for (auto&& [flag, info] : m_meshes) {
                if (!info.pVisual) {
                    continue;
                }

                if (pMesh == info.pSelection.Get() && info.pSelection.Get()) {
                    info.pVisual->GetMaterial()->SetVec4("color", SR_MATH_NS::FColor(1.f, 1.f, 0.f, 1.f));
                    m_hoveredOperation = flag;
                }
                else {
                    info.pVisual->GetMaterial()->SetVec4("color", GetColorByOperation(flag));
                }
            }
        }

        if (m_hoveredOperation != GizmoOperation::None && SR_UTILS_NS::Input::Instance().GetMouseDown(SR_UTILS_NS::MouseCode::MouseLeft)) {
            m_activeOperation = m_hoveredOperation;

            auto&& translationNormal = SR_MATH_NS::CalcTranslationPlanNormal(
                    m_modelMatrix,
                    GetCamera()->GetCameraEye(),
                    GetCamera()->GetCameraDir(),
                    GetAxis()
            );
            m_translationPlan = SR_MATH_NS::BuildPlan(m_modelMatrix.v.position, translationNormal);

            if (SR_MATH_NS::IsMaskIncludedSubMask(m_activeOperation, GizmoOperation::Rotate)) {
                auto&& rotationNormal = IsLocal() ? SR_MATH_NS::CalcRotationPlanNormal(
                        m_modelMatrix,
                        GetCamera()->GetCameraDir(),
                        GetAxis()
                ) : SR_MATH_NS::CalcRotationPlanNormal(GetCamera()->GetCameraDir(), GetAxis());

                m_rotationPlan = SR_MATH_NS::BuildPlan(m_modelMatrix.v.position, rotationNormal);
            }

            auto&& screenRay = GetCamera()->GetScreenRay(mousePos, IsGizmo2DSpace());

            const float_t screenFactor = GetCamera()->CalculateScreenFactor(m_modelMatrix, m_moveFactor, IsGizmo2DSpace());

            m_translationPlanOrigin = screenRay.IntersectPlane(m_translationPlan);
            m_relativeOrigin = (m_translationPlanOrigin - m_modelMatrix.v.position.XYZ()) * (1.f / screenFactor);

            m_rotationVectorSource = screenRay.RotationVector(m_rotationPlan, m_modelMatrix.v.position.XYZ());
            m_rotationAngleOrigin = screenRay.ComputeAngleOnPlan(m_rotationPlan, m_modelMatrix.v.position.XYZ(), m_rotationVectorSource);

            BeginGizmo();
        }

        ProcessGizmo(mousePos);

        m_lastMousePos = mousePos;

        Super::FixedUpdate();
    }

    bool Gizmo::InitializeEntity() noexcept {
        GetComponentProperties()
            .AddStandardProperty("Zoom factor", &m_zoomFactor)
            .SetResetValue(0.1f)
            .SetDrag(0.05f)
            .SetWidth(60.f);

        GetComponentProperties()
            .AddStandardProperty("Move factor", &m_moveFactor)
            .SetResetValue(0.1f)
            .SetDrag(0.05f)
            .SetWidth(60.f);

        return Super::InitializeEntity();
    }

    void Gizmo::ProcessGizmo(const SR_MATH_NS::FPoint& mousePos) {
        if (m_activeOperation == GizmoOperation::None) {
            return;
        }

        auto&& screenRay = GetCamera()->GetScreenRay(mousePos, IsGizmo2DSpace());
        const float_t screenFactor = GetCamera()->CalculateScreenFactor(m_modelMatrix, m_moveFactor, IsGizmo2DSpace());

        auto&& newPos = screenRay.IntersectPlane(m_translationPlan);
        auto&& newOrigin = newPos - m_relativeOrigin * screenFactor;
        auto&& delta = newOrigin - m_modelMatrix.v.position.XYZ();
        SR_MATH_NS::FVector3 axisValue;

        if (!(m_activeOperation & GizmoOperation::Alternative) && !(m_activeOperation & GizmoOperation::Center)) {
            axisValue = m_modelMatrix.GetAxis(GetAxis()).XYZ();
            const float_t lengthOnAxis = axisValue.Dot(delta);
            delta = axisValue * lengthOnAxis;
        }

        if (m_activeOperation & GizmoOperation::Translate) {
            OnGizmoTranslated(delta);
            UpdateGizmoTransform();
        }

        if (m_activeOperation & GizmoOperation::Rotate) {
            const SR_MATH_NS::Unit rotationAngle = screenRay.ComputeAngleOnPlan(m_rotationPlan, m_modelMatrix.v.position.XYZ(), m_rotationVectorSource);
            const SR_MATH_NS::Unit deltaAngle = rotationAngle - m_rotationAngleOrigin;

            auto&& rotationAxisLocalSpace = m_modelMatrix.Inverse().TransformVector(m_rotationPlan.XYZ()).Normalize();

            m_rotationVectorSource = screenRay.RotationVector(m_rotationPlan, m_modelMatrix.v.position.XYZ());
            m_rotationAngleOrigin = screenRay.ComputeAngleOnPlan(m_rotationPlan, m_modelMatrix.v.position.XYZ(), m_rotationVectorSource);

            OnGizmoRotated(SR_MATH_NS::Quaternion(rotationAxisLocalSpace.XYZ(), deltaAngle));

            UpdateGizmoTransform();
        }

        if (m_activeOperation & GizmoOperation::Scale) {
            uint8_t axisIndex = SR_UINT8_MAX;

            if (m_activeOperation & GizmoOperation::X) {
                axisIndex = 0;
            }
            else if (m_activeOperation & GizmoOperation::Y) {
                axisIndex = 1;
            }
            else if (m_activeOperation & GizmoOperation::Z) {
                axisIndex = 2;
            }

            SR_MATH_NS::FVector3 scale = SR_MATH_NS::FVector3::One();

            if (axisIndex != SR_UINT8_MAX) {
                axisValue = IsLocal() ? axisValue : SR_MATH_NS::FVector3::AxisByIndex(axisIndex);
                auto&& baseVector = m_translationPlanOrigin - m_modelMatrix.v.position.XYZ();
                const float_t ratio = axisValue.Dot(baseVector + delta) / axisValue.Dot(baseVector);
                scale[axisIndex] = SR_MAX(ratio, SR_BIG_EPSILON);
            }
            else {
                const float_t scaleDelta = (mousePos.x - m_lastMousePos.x) + (mousePos.y - m_lastMousePos.y);
                scale = SR_MATH_NS::FVector3(SR_MAX(1.f + scaleDelta, SR_BIG_EPSILON));
            }

            OnGizmoScaled(scale);

            UpdateGizmoTransform();
        }

        m_translationPlanOrigin = screenRay.IntersectPlane(m_translationPlan);
        m_relativeOrigin = (m_translationPlanOrigin - m_modelMatrix.v.position.XYZ()) * (1.f / screenFactor);
    }

    SR_MATH_NS::AxisFlag Gizmo::GetAxis() const {
        SR_MATH_NS::AxisFlag axis = SR_MATH_NS::Axis::None;
        axis |= (m_activeOperation & GizmoOperation::X) ? SR_MATH_NS::Axis::X : SR_MATH_NS::Axis::None;
        axis |= (m_activeOperation & GizmoOperation::Y) ? SR_MATH_NS::Axis::Y : SR_MATH_NS::Axis::None;
        axis |= (m_activeOperation & GizmoOperation::Z) ? SR_MATH_NS::Axis::Z : SR_MATH_NS::Axis::None;
        axis |= (m_activeOperation & GizmoOperation::Center) ? SR_MATH_NS::Axis::XYZ : SR_MATH_NS::Axis::None;
        return axis;
    }

    SR_MATH_NS::FColor Gizmo::GetColorByOperation(GizmoOperationFlag operation) const {
        if (operation & GizmoOperation::Center) {
            return SR_MATH_NS::FVector4(1.f, 1.f, 1.f, 0.75f);
        }

        if ((operation & GizmoOperation::X && !(operation & GizmoOperation::Alternative)) || (operation == GizmoOperation::TranslateAltX)) {
            return SR_MATH_NS::FVector4(1.f, 0.f, 0.f, 0.75f);
        }

        if ((operation & GizmoOperation::Y && !(operation & GizmoOperation::Alternative)) || (operation == GizmoOperation::TranslateAltY)) {
            return SR_MATH_NS::FVector4(0.f, 1.f, 0.f, 0.75f);
        }

        if ((operation & GizmoOperation::Z && !(operation & GizmoOperation::Alternative)) || (operation == GizmoOperation::TranslateAltZ)) {
            return SR_MATH_NS::FVector4(0.f, 0.f, 1.f, 0.75f);
        }

        SRHalt("Unknown operation!");
        return SR_MATH_NS::FVector4();
    }

    SR_MATH_NS::Matrix4x4 Gizmo::GetGizmoMatrix() const {
        return SR_MATH_NS::Matrix4x4(
            GetTransform()->GetTranslation(),
            GetTransform()->GetQuaternion()
            /// ignore the scale, because gizmo automatically changes its own scale
        );
    }

    void Gizmo::OnGizmoTranslated(const SR_MATH_NS::FVector3& delta) {
        GetTransform()->Translate(GetTransform()->GetMatrix().GetQuat().Inverse() * delta);
    }

    void Gizmo::OnGizmoRotated(const SR_MATH_NS::Quaternion& delta) {
        GetTransform()->Rotate(delta);
    }

    void Gizmo::UpdateGizmoTransform() {
        SR_TRACY_ZONE;

        m_modelMatrix = GetGizmoMatrix();

        const bool isScaleOperation = m_activeOperation & GizmoOperation::Scale;

        m_modelMatrix = SR_MATH_NS::Matrix4x4(
                m_modelMatrix.GetTranslate(),
                IsLocal() ? m_modelMatrix.GetQuat() : SR_MATH_NS::Quaternion::Identity(),
                isScaleOperation ? m_modelMatrix.GetScale() : SR_MATH_NS::FVector3(1.f)
        );

        if (IsHandledAnotherObject()) {
            GetTransform()->SetTranslation(m_modelMatrix.GetTranslate());
            GetTransform()->SetRotation(m_modelMatrix.GetQuat());
        }

        if (m_zoomFactor > 0.f) {
            auto&& modelMatrix = SR_MATH_NS::Matrix4x4::FromTranslate(GetTransform()->GetTranslation());
            const float_t screenFactor = GetCamera()->CalculateScreenFactor(modelMatrix, m_zoomFactor, IsGizmo2DSpace());
            GetTransform()->SetScale(screenFactor);
        }

        for (auto&& [flag, info] : m_meshes) {
            if (!info.pVisual || !info.pSelection) {
                continue;
            }

            if (!SR_MATH_NS::IsMaskIncludedSubMask(flag, GizmoOperation::Rotate)) {
                continue;
            }

            if (SR_MATH_NS::IsMaskIncludedSubMask(flag, GizmoOperation::Rotate2D)) {
                continue;
            }

            auto&& direction = GetCamera()->GetPosition() - GetTransform()->GetTranslation();

            auto&& right = IsLocal() ? GetTransform()->Right() : SR_MATH_NS::FVector3::Right();
            auto&& up = IsLocal() ? GetTransform()->Up() : SR_MATH_NS::FVector3::Up();
            auto&& forward = IsLocal() ? GetTransform()->Forward() : SR_MATH_NS::FVector3::Forward();

            auto&& directionRight = direction.ProjectOnPlane(right);
            auto&& directionUp = direction.ProjectOnPlane(up);
            auto&& directionForward = direction.ProjectOnPlane(forward);

            SR_MATH_NS::Quaternion quaternion;

            if (flag & GizmoOperation::X) {
                auto&& axis = directionRight.Cross(up).Normalize();
                quaternion = SR_MATH_NS::Quaternion::LookAt(-directionRight, axis).RotateZ(90);
            }
            else if (flag & GizmoOperation::Y) {
                quaternion = SR_MATH_NS::Quaternion::LookAt(-directionUp, up);
            }
            else if (flag & GizmoOperation::Z) {
                auto&& axis = directionForward.Cross(up).Normalize();
                quaternion = SR_MATH_NS::Quaternion::LookAt(directionForward, axis).RotateY(90).RotateX(-90);
            }
            else if (flag & GizmoOperation::Center) {
                quaternion = SR_MATH_NS::Quaternion::LookAt(-direction, SR_MATH_NS::FVector3::One());
            }

            info.pVisual->GetTransform()->SetGlobalRotation(quaternion);
            info.pSelection->GetTransform()->SetGlobalRotation(quaternion);
        }
    }

    Utils::Component::GameObjectPtr Gizmo::GetGameObjectByOperation(GizmoMeshLoadMode mode, GizmoOperationFlag operation) const {
        auto&& root = mode == GizmoMeshLoadMode::Visual ? GetGameObject() : GetGameObject()->GetOrCreateChild("Selection");

        if (operation & GizmoOperation::Center) {
            return root->GetOrCreateChild("RotateCenter");
        }
        else if (operation & GizmoOperation::X) {
            return root->GetOrCreateChild("RotateX");
        }
        else if (operation & GizmoOperation::Y) {
            return root->GetOrCreateChild("RotateY");
        }
        else if (operation & GizmoOperation::Z) {
            return root->GetOrCreateChild("RotateZ");
        }

        SR_ERROR("Gizmo::GetGameObjectByOperation() : unknown operation!");

        return nullptr;
    }

    void Gizmo::SetOperation(GizmoOperationFlag operation) {
        if (m_operation == operation) {
            return;
        }

        m_operation = operation;
        m_isGizmoDirty = true;
    }

    void Gizmo::OnGizmoScaled(const SR_MATH_NS::FVector3& delta) {
        GetTransform()->Scale(delta);
    }
}