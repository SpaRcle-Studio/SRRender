//
// Created by Monika on 23.11.2023.
//

#ifndef SR_ENGINE_GRAPHICS_MANIPULATION_TOOL_H
#define SR_ENGINE_GRAPHICS_MANIPULATION_TOOL_H

#include <Graphics/Types/Geometry/MeshComponent.h>
#include <Graphics/Types/IRenderComponent.h>

namespace SR_GRAPH_UI_NS {
    SR_ENUM_NS_CLASS_T(GizmoMode, uint8_t, Local, Global);

    SR_ENUM_NS_STRUCT_T(GizmoOperation, uint64_t,
        None = 0,
        Center = 1 << 0,
        Alternative = 1 << 1,

        X = 1 << 2,
        Y = 1 << 3,
        Z = 1 << 4,

        Translate = 1 << 5,
        Rotate = 1 << 6,
        Scale = 1 << 7,
        Bounds = 1 << 8,

        TranslateX = X | Translate,
        TranslateY = Y | Translate,
        TranslateZ = Z | Translate,
        TranslateAltX = Y | Z | Translate | Alternative,
        TranslateAltY = X | Z | Translate | Alternative,
        TranslateAltZ = X | Y | Translate | Alternative,
        TranslateCenter = Translate | Center,
        TranslateAll = X | Y | Z | Translate | Center | Alternative,

        RotateX = X | Rotate,
        RotateY = Y | Rotate,
        RotateZ = Z | Rotate,
        RotateCenter = Rotate | Center,
        RotateAll = X | Y | Z | Rotate | Center,

        ScaleX = X | Scale,
        ScaleY = Y | Scale,
        ScaleZ = Z | Scale,
        ScaleCenter = Scale | Center,
        ScaleAll = X | Y | Z | Scale | Center,

        BoundsX = X | Bounds,
        BoundsY = Y | Bounds,
        BoundsZ = Z | Bounds,

        Universal = TranslateAll | RotateAll | ScaleAll
    );

    class Gizmo : public SR_GTYPES_NS::IRenderComponent {
        SR_REGISTER_NEW_COMPONENT(Gizmo, 1001);
        using Super = SR_GTYPES_NS::IRenderComponent;
        enum class GizmoMeshLoadMode {
            Visual, Selection, All
        };
    public:
        bool InitializeEntity() noexcept override;

        void OnEnable() override;
        void OnDisable() override;
        void OnAttached() override;
        void OnDestroy() override;
        void FixedUpdate() override;

        SR_NODISCARD bool IsGizmoActive() const { return m_activeOperation != GizmoOperation::None; }
        SR_NODISCARD bool IsGizmoHovered() const { return m_hoveredOperation != GizmoOperation::None; }
        SR_NODISCARD bool IsGizmo2DSpace() const { return !SR_MATH_NS::IsMaskIncludedSubMask(m_operation, GizmoOperation::Z); }

        void SetMode(GizmoMode mode) { m_mode = mode; }
        void SetOperation(GizmoOperationFlag operation);

        SR_NODISCARD virtual GizmoMode GetMode() const { return m_mode; }
        SR_NODISCARD virtual GizmoOperationFlag GetOperation() const { return m_operation; }

    protected:
        void ProcessGizmo(const SR_MATH_NS::FPoint& mousePos);
        void LoadGizmo();
        void ReleaseGizmo();
        void LoadMesh(GizmoOperationFlag operation, SR_UTILS_NS::StringAtom path, SR_UTILS_NS::StringAtom name, GizmoMeshLoadMode mode);
        void UpdateGizmoTransform();

        virtual void PrepareGizmo() { }

        virtual void OnGizmoTranslated(const SR_MATH_NS::FVector3& delta);
        virtual void OnGizmoScaled(const SR_MATH_NS::FVector3& delta);
        virtual void OnGizmoRotated(const SR_MATH_NS::Quaternion& delta);

        virtual void BeginGizmo() { }
        virtual void EndGizmo() { }

        SR_NODISCARD virtual bool IsGizmoAvailable() const { return true; }
        SR_NODISCARD virtual bool IsHandledAnotherObject() const { return false; }
        SR_NODISCARD virtual SR_MATH_NS::Matrix4x4 GetGizmoMatrix() const;
        SR_NODISCARD bool IsLocal() const { return GetMode() == GizmoMode::Local; }

        SR_NODISCARD GameObjectPtr GetGameObjectByOperation(GizmoMeshLoadMode mode, GizmoOperationFlag operation) const;

        SR_NODISCARD SR_MATH_NS::AxisFlag GetAxis() const;

        SR_NODISCARD SR_FORCE_INLINE bool ExecuteInEditMode() const override { return true; }

    private:
        struct MeshInfo {
            SR_GTYPES_NS::MeshComponent::Ptr pVisual;
            SR_GTYPES_NS::MeshComponent::Ptr pSelection;
            bool isHovered = false;
        };
        std::map<GizmoOperationFlag, MeshInfo> m_meshes;

        bool m_isGizmoDirty = false;

        float_t m_zoomFactor = 0.0665f;
        float_t m_moveFactor = 0.1f;

        SR_MATH_NS::FPoint m_lastMousePos = SR_MATH_NS::InfinityFV2;

        GizmoMode m_mode = GizmoMode::Local;
        GizmoOperationFlag m_operation = GizmoOperation::TranslateAll;

        GizmoOperationFlag m_activeOperation = GizmoOperation::None;
        GizmoOperationFlag m_hoveredOperation = GizmoOperation::None;

        SR_MATH_NS::FVector3 m_translationPlanOrigin;
        SR_MATH_NS::FVector3 m_relativeOrigin;
        SR_MATH_NS::FVector3 m_rotationVectorSource;
        SR_MATH_NS::FVector4 m_translationPlan;
        SR_MATH_NS::FVector4 m_rotationPlan;
        SR_MATH_NS::Matrix4x4 m_modelMatrix;

        SR_MATH_NS::Unit m_rotationAngleOrigin = 0.f;

    };
}

#endif //SR_ENGINE_GRAPHICS_MANIPULATION_TOOL_H
