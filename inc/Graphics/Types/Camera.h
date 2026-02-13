//
// Created by Nikita on 18.11.2020.
//

#ifndef SR_ENGINE_CAMERA_H
#define SR_ENGINE_CAMERA_H

#include <Graphics/Utils/Frustum.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/ComponentManager.h>
#include <Utils/Math/Vector3.h>
#include <Utils/Math/Rect.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Common/SubscriptionHolder.h>

namespace SR_GRAPH_NS {
    class Window;
    class IRenderTechnique;
    class RenderScene;
    struct RenderTechniqueData;
}

namespace SR_GTYPES_NS {
    SR_ENUM_NS_CLASS_T(CameraType, uint8_t,
        Main,
        Offscreen,
        Editor,
        EditorPrefab
    );

    /// @category(Render)
    class Camera : public SR_UTILS_NS::Component {
        SR_CLASS()
        struct RenderTechniqueInfo {
            SR_UTILS_NS::Path path;
            SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::IRenderTechnique> pTechnique;
        };
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using Super = SR_UTILS_NS::Component;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Camera>;

    public:
        Camera();
        ~Camera() override;

    public:
        void Start() override;
        void OnMatrixDirty() override;
        void OnAttached() override;
        void UpdateProjection(uint32_t w, uint32_t h);
        void Update(float_t dt) override;

    public:
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Matrix4x4& GetView() const noexcept { return m_viewMat; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Matrix4x4& GetOrthogonal() const noexcept { return m_orthogonal; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Matrix4x4& GetPixelOrthogonal() const noexcept { return m_pixelOrthogonal; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Matrix4x4& GetViewTranslate() const noexcept { return m_viewTranslateMat; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Matrix4x4& GetProjection() const noexcept { return m_projection; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Matrix4x4& GetProjectionNoFOV() const noexcept { return m_projectionNoFOV; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::Quaternion& GetRotation() const noexcept { return m_rotation; }
        SR_NODISCARD SR_FORCE_INLINE SR_MATH_NS::UVector2 GetSize() const { return m_viewportSize; }
        SR_NODISCARD SR_MATH_NS::FVector3 GetViewPosition() const;
        SR_NODISCARD SR_MATH_NS::FVector3 GetCameraEye() const;
        SR_NODISCARD SR_MATH_NS::FVector3 GetCameraDir() const;
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::FVector3& GetPosition() const { return m_position; }
        SR_NODISCARD SR_FORCE_INLINE glm::vec3 GetGLPosition() const { return m_position.ToGLM(); }
        SR_NODISCARD SR_FORCE_INLINE float_t GetFar() const { return m_far; }
        SR_NODISCARD SR_FORCE_INLINE float_t GetNear() const { return m_near; }
        SR_NODISCARD SR_FORCE_INLINE float_t GetFOV() const { return m_FOV; }
        SR_NODISCARD SR_FORCE_INLINE float_t GetAspect() const { return m_aspect; }
        SR_NODISCARD SR_FORCE_INLINE int32_t GetPriority() const { return m_priority; }
        SR_NODISCARD SR_FORCE_INLINE const SR_MATH_NS::UVector2& GetViewportSize() const { return m_viewportSize; }
        SR_NODISCARD SR_MATH_NS::FRect GetViewportRect() const;

        SR_NODISCARD const SR_MATH_NS::Matrix4x4& GetInverseProjection() const;
        SR_NODISCARD const SR_MATH_NS::Matrix4x4& GetInverseViewTranslate() const;

        SR_NODISCARD bool IsEditorCamera() const;
        SR_NODISCARD CameraType GetCameraType() const { return m_type; }
        SR_NODISCARD const SR_MATH_NS::FVector3& GetViewDirection() const;
        SR_NODISCARD SR_MATH_NS::FVector3 GetViewDirection(const SR_MATH_NS::FVector3& pos) const noexcept;

        SR_NODISCARD IRenderTechnique* GetRenderTechnique();
        SR_NODISCARD RenderScenePtr GetRenderScene() const;
        SR_NODISCARD RenderScenePtr TryGetRenderScene() const;
        SR_NODISCARD const SR_UTILS_NS::Path& GetRenderTechniquePath();

        SR_NODISCARD virtual SR_MATH_NS::FPoint GetMousePos() const;

        SR_NODISCARD float_t CalculateScreenFactor(const SR_MATH_NS::Matrix4x4& modelMatrix, float_t sizeClipSpace, bool orthogonal) const;
        SR_NODISCARD float_t CalculateScreenFactor(const SR_MATH_NS::Matrix4x4& modelMatrix, const SR_MATH_NS::Matrix4x4& viewMatrix, float_t sizeClipSpace, bool orthogonal) const;

        SR_NODISCARD SR_MATH_NS::Ray GetScreenRay(const SR_MATH_NS::FPoint& screenPos, bool orthogonal) const;
        SR_NODISCARD SR_MATH_NS::Ray GetScreenRay(float_t x, float_t y, bool orthogonal) const;
        SR_NODISCARD SR_MATH_NS::FVector3 ScreenToWorldPoint(const SR_MATH_NS::FVector3& screenPos) const;
        SR_NODISCARD SR_MATH_NS::FVector3 ScreenToWorldPoint(const SR_MATH_NS::FVector2& screenPos) const;
        SR_NODISCARD SR_MATH_NS::FVector3 ScreenToWorldPoint(const SR_MATH_NS::FVector2& screenPos, float_t depth) const;
        SR_NODISCARD const Frustum& GetFrustum() const { return m_frustum; }

        void SetFar(float_t value);
        void SetNear(float_t value);
        void SetFOV(float_t value);
        void SetPriority(int32_t priority);
        void SetCameraType(CameraType type);

        void SetRenderTechnique(const SR_UTILS_NS::Path& path);
        void SetViewportRect(const std::optional<SR_MATH_NS::FRect>& rect) { m_viewportRect = rect; }

        SR_NODISCARD const RenderTechniqueData& GetRenderTechniqueData() const;

    protected:
        void UpdateProjection(bool nonResized);

        void UpdateView() noexcept;

        void OnDestroy() override;
        void OnEnable() override;
        void OnDisable() override;

    private:
        void RemoveTechnique();

    private:
        /** >= 0 - одна главная камера, < 0 - закадровые камеры, которые рендерятся в RenderTexture.
         * Выбирается та камера, что ближе к нулю */
        /// @property @setter(SetPriority)
        int32_t m_priority = 0;

        /// @property @setter(SetFar)
        float_t m_far = 750.f;
        /// @property @setter(SetNear)
        float_t m_near = 0.01f;
        /// @property @setter(SetFOV)
        float_t m_FOV = 60.f;

        /// @virtualProperty(renderTechnique) @getter(GetRenderTechniquePath) @setter(SetRenderTechnique)
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech,srptech)
        SR_VIRTUAL_PROPERTY

        /// @virtualProperty(renderTechniqueInfo) @readOnly @dontSave
        /// @getter(GetRenderTechniqueData)
        SR_VIRTUAL_PROPERTY

        /// @property @readOnly @dontSave
        float_t m_aspect = 1.f;
        /// @property @readOnly @dontSave
        bool m_hasErrors = false;
        /// @property @readOnly @dontSave
        bool m_isRegistered = false;
        /// @property
        CameraType m_type = CameraType::Main;

        SR_UTILS_NS::Subscription m_onRenderSettingsChanged;

        SR_MATH_NS::Matrix4x4 m_projection;
        SR_MATH_NS::Matrix4x4 m_projectionNoFOV;
        SR_MATH_NS::Matrix4x4 m_viewTranslateMat;
        SR_MATH_NS::Matrix4x4 m_viewMat;
        SR_MATH_NS::Matrix4x4 m_orthogonal;
        SR_MATH_NS::Matrix4x4 m_pixelOrthogonal;

        mutable bool m_isInverseDirty = true;
        mutable SR_MATH_NS::Matrix4x4 m_inverseProjection;
        mutable SR_MATH_NS::Matrix4x4 m_inverseViewTranslate;

        Frustum m_frustum;

        std::optional<SR_MATH_NS::FRect> m_viewportRect;

        SR_MATH_NS::Quaternion m_rotation;

        SR_MATH_NS::FVector3 m_viewDirection;
        SR_MATH_NS::FVector3 m_position;
        SR_MATH_NS::UVector2 m_viewportSize;

        RenderTechniqueInfo m_renderTechnique = { };

    };
}

#endif //SR_ENGINE_CAMERA_H
