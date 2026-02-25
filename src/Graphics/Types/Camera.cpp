//
// Created by Nikita on 18.11.2020.
//

#include <Graphics/Types/Camera.h>
#include <Graphics/Memory/CameraManager.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Types/DataStorage.h>
#include <Utils/Types/SafePtrLockGuard.h>
#include <Utils/Platform/Platform.h>
#include <Utils/Events/Broadcaster.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/Camera.generated.hpp>

namespace SR_GTYPES_NS {
    Camera::Camera()
        : Super()
    { }

    Camera::~Camera() {
        m_onRenderSettingsChanged.Reset();
        SetRenderTechnique(SR_UTILS_NS::Path());
    }

    void Camera::OnAttached() {
        Super::OnAttached();

        m_onRenderSettingsChanged = SR_UTILS_NS::Broadcaster::Instance().Subscribe(SR_UTILS_NS::Events::EVENT_ON_RENDER_SETTINGS_CHANGED_ID, [this](auto&&) {
            m_hasErrors = false;
            RemoveTechnique();
        });

        if (auto&& pRenderScene = GetRenderScene()) {
            pRenderScene->Register(GetThis().DynamicCast<Camera>());
            m_isRegistered = true;
        }
        else {
            SRHalt("Render scene is invalid!");
        }
    }

    void Camera::OnDestroy() {
        RenderScene::Ptr pRenderScene = TryGetRenderScene();

        if (m_isRegistered && pRenderScene) {
            pRenderScene->Remove(GetThis().DynamicCast<Camera>());
        }

        Super::OnDestroy();
    }

    const SR_MATH_NS::Matrix4x4& Camera::GetInverseProjection() const {
        if (m_isInverseDirty) {
            SR_TRACY_ZONE;
            m_isInverseDirty = false;
            m_inverseProjection = m_projection.Inverse();
            m_inverseViewTranslate = m_viewTranslateMat.Inverse();
        }
        return m_inverseProjection;
    }

    const SR_MATH_NS::Matrix4x4& Camera::GetInverseViewTranslate() const {
        if (m_isInverseDirty) {
            SR_TRACY_ZONE;
            m_isInverseDirty = false;
            m_inverseProjection = m_projection.Inverse();
            m_inverseViewTranslate = m_viewTranslateMat.Inverse();
        }
        return m_inverseViewTranslate;
    }

    bool Camera::IsEditorCamera() const {
        return m_type == CameraType::Editor || m_type == CameraType::EditorPrefab;
    }

    IRenderTechnique* Camera::GetRenderTechnique() {
        if (m_renderTechnique.pTechnique || m_hasErrors) {
            return m_renderTechnique.pTechnique.Get();
        }

        SR_TRACY_ZONE;

        SR_UTILS_NS::Path path = GetRenderTechniquePath();

        auto&& pScene = TryGetRenderScene();
        if (!pScene) {
            return nullptr;
        }

        auto&& pContext = pScene->GetContext();

        if (path.IsEmpty()) {
            switch (m_type) {
                case CameraType::Main:
                    path = pContext->GetSettingsPreset().mainCameraRenderTechnique;
                    break;
                case CameraType::Editor:
                case CameraType::EditorPrefab:
                    path = pContext->GetSettingsPreset().editorCameraRenderTechnique;
                    break;
                default:
                    break;
            }
        }

        RemoveTechnique();

        if (path.IsEmpty()) {
            m_hasErrors = true;
            return nullptr;
        }

        RenderTechniqueLoadParams params;
        params.instancing = pContext->GetPipeline()->IsShaderViewportIndexLayerSupported();
        params.editor = IsEditorCamera();
        params.pRenderSettings = &pContext->GetSettings();
        params.sceneViewName = pContext->GetSettings().editorSceneImageName;
        params.activeGraphicsSettings = pContext->GetActiveGraphicsSettings();
        m_renderTechnique.pTechnique = FileRenderTechnique::Load(path, params).StaticCast<IRenderTechnique>();

        if (m_renderTechnique.pTechnique) {
            m_renderTechnique.pTechnique->SetCamera(this);
        }
        else {
            SR_ERROR("Camera::GetRenderTechnique() : failed to load render technique from path: \"{}\"!", path);
            m_hasErrors = true;
        }

        return m_renderTechnique.pTechnique.Get();
    }

    const SR_UTILS_NS::Path& Camera::GetRenderTechniquePath() {
        return m_renderTechnique.path;
    }

    Camera::RenderScenePtr Camera::TryGetRenderScene() const {
        auto&& scene = TryGetScene();
        if (!scene) {
            return RenderScenePtr();
        }

        if (scene->Valid()) {
            return scene->GetDataStorage().GetValue<RenderScenePtr>();
        }

        return RenderScenePtr();
    }

    Camera::RenderScenePtr Camera::GetRenderScene() const {
        auto&& renderScene = TryGetRenderScene();

        if (!renderScene) {
            SRHalt("Render scene is nullptr!");
        }

        return renderScene;
    }

    void Camera::UpdateView() noexcept {
        SR_TRACY_ZONE;

        m_isInverseDirty = true;
        m_viewMat = m_rotation.RotateX(SR_DEG(SR_PI)).Inverse().ToMat4x4();

        /*auto&& euler = m_rotation.RotateX(SR_DEG(SR_PI)).Inverse().EulerAngle();

        float yr = SR_RAD(euler.y);
        float sy = sin(yr);
        float cy = cos(yr);

        float pr = SR_RAD(euler.x);
        float sx = sin(pr);
        float cx = cos(pr);

        float rr = SR_RAD(euler.z);
        float sz = sin(rr);
        float cz = cos(rr);

        glm::mat3x3 rotation = glm::mat3x3(cy*cz, -cy*sz, sy, sx*sy*cz+cx*sz, -sx*sy*sz+cx*cz, -sx*cy, -cx*sy*cz+sx*sz, cx*sy*sz+sx*cz, cx*cy);
        m_viewMat = SR_MATH_NS::Matrix4x4(rotation);*/


        m_viewTranslateMat = m_viewMat.Translate(m_position.Inverse());
        m_viewDirection = m_rotation * SR_MATH_NS::FVector3(0, 0, 1);
        m_frustum = ExtractFrustum(m_projection * m_viewTranslateMat);
    }

    void Camera::UpdateProjection(bool nonResized) {
        m_isInverseDirty = true;

        if (m_viewportSize.HasZero()) {
            m_aspect = 0.f;
        }
        else {
            m_aspect = static_cast<float_t>(m_viewportSize.x) / static_cast<float_t>(m_viewportSize.y);
        }

        m_projection = SR_MATH_NS::Matrix4x4::Perspective(SR_RAD(m_FOV), m_aspect, m_near, m_far);
        m_projectionNoFOV = SR_MATH_NS::Matrix4x4::Perspective(SR_RAD(45.f), m_aspect, m_near, m_far);

        //////////////////////////////////////////////////////////////////////////////////////////////

        m_orthogonal = SR_MATH_NS::Matrix4x4::Identity();

        //m_orthogonal[0][0] = 1.f / m_aspect;

        m_orthogonal[0][0] = 1.f;
        m_orthogonal[1][1] = -1.f;
        m_orthogonal[2][2] = 1.f / (m_far - m_near);
        m_orthogonal[3][2] = m_near / (m_far - m_near);

        //m_orthogonal[0][0] =  2.0f / m_viewportSize.x;
        //m_orthogonal[1][1] = -2.0f / m_viewportSize.y; // В Vulkan Y растёт вниз
        //m_orthogonal[2][2] = 1.0f / (m_far - m_near);
        //m_orthogonal[3][0] = -1.0f;
        //m_orthogonal[3][1] = 1.0f;
        //m_orthogonal[3][2] = m_near / (m_near - m_far);

        if (!m_viewportSize.HasZero()) {
            m_pixelOrthogonal = SR_MATH_NS::Matrix4x4::Identity();
            m_pixelOrthogonal[0][0] =  2.0f / m_viewportSize.x;  // масштабируем X в [-1,1]
            m_pixelOrthogonal[1][1] = -2.0f / m_viewportSize.y;  // масштабируем Y в [-1,1] и переворачиваем, чтобы (0,0) был в верхнем левом
            m_pixelOrthogonal[2][2] = 1.f / (m_far - m_near);
            m_pixelOrthogonal[3][2] = m_near / (m_far - m_near);
            m_pixelOrthogonal[3][0] = -1.0f;       // смещение X
            m_pixelOrthogonal[3][1] = 1.0f;        // смещение Y
        }

        //////////////////////////////////////////////////////////////////////////////////////////////

        if (m_renderTechnique.pTechnique) {
            if (nonResized) {
                m_renderTechnique.pTechnique->OnCameraParamsChanged();
            }
            else {
                m_renderTechnique.pTechnique->OnResize(m_viewportSize);
            }
        }
    }

    void Camera::UpdateProjection(uint32_t w, uint32_t h) {
        if (m_viewportSize.x == w && m_viewportSize.y == h) {
            return;
        }

        m_viewportSize = SR_MATH_NS::UVector2(w, h);

        UpdateProjection(false);
    }

    void Camera::SetFar(float_t value) {
        m_far = value;

        if (!m_viewportSize.HasZero()) {
            UpdateProjection(true);
        }
    }

    void Camera::SetNear(float_t value) {
        m_near = value;

        if (!m_viewportSize.HasZero()) {
            UpdateProjection(true);
        }
    }

    void Camera::SetFOV(float_t value) {
        m_FOV = value;

        if (!m_viewportSize.HasZero()) {
            UpdateProjection(true);
        }
    }

    void Camera::OnEnable() {
        if (auto&& renderScene = TryGetRenderScene()) {
            renderScene->SetDirtyCameras();
        }

        Super::OnEnable();
    }

    void Camera::OnDisable() {
        if (auto&& renderScene = GetRenderScene()) {
            renderScene->SetDirtyCameras();
        }

        Super::OnDisable();
    }

    void Camera::RemoveTechnique() {
        if (m_renderTechnique.pTechnique) {
            m_renderTechnique.pTechnique->KillTechnique();
            m_renderTechnique.pTechnique = nullptr;
        }
    }

    void Camera::OnMatrixDirty() {
        auto&& pTransform = GetTransform();
        if (!pTransform) {
            return;
        }

        pTransform->GetMatrix().Decompose(m_position, m_rotation);

        UpdateView();

        Super::OnMatrixDirty();
    }

    void Camera::SetRenderTechnique(const SR_UTILS_NS::Path& path) {
        RemoveTechnique();
        m_hasErrors = false;

        if (path.IsEmpty()) {
            m_renderTechnique.path = SR_UTILS_NS::Path();
        }
        else {
            m_renderTechnique.path = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
        }
    }

    void Camera::SetCameraType(CameraType type) {
        if (m_type == type) {
            return;
        }
        m_type = type;
        m_hasErrors = false;
        if (m_renderTechnique.path.empty()) {
            RemoveTechnique();
        }
    }

    void Camera::SetPriority(int32_t priority) {
        m_priority = priority;

        if (!m_parent) {
            return;
        }

        if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->SetDirtyCameras();
        }
    }

    const SR_MATH_NS::FVector3& Camera::GetViewDirection() const {
        return m_viewDirection;
    }

    SR_MATH_NS::FVector3 Camera::GetViewDirection(const SR_MATH_NS::FVector3& pos) const noexcept {
        auto&& dir = m_position.Direction(pos);
        return m_rotation * SR_MATH_NS::FVector3(dir);
    }

    SR_MATH_NS::FVector3 Camera::GetViewPosition() const {
        return m_position;
    }

    void Camera::Update(float_t dt) {
        Super::Update(dt);
    }

    void Camera::Start() {
        Super::Start();
    }

    SR_MATH_NS::FPoint Camera::GetMousePos() const {
        return SR_PLATFORM_NS::GetMousePos();
    }

    SR_MATH_NS::Ray Camera::GetScreenRay(float_t x, float_t y, bool orthogonal) const {
        return GetScreenRay(SR_MATH_NS::FPoint(x, y), orthogonal);
    }

    SR_MATH_NS::Ray Camera::GetScreenRay(const SR_MATH_NS::FPoint& screenPos, bool orthogonal) const {
        SR_MATH_NS::Matrix4x4 viewProjInverse;

        if (orthogonal) {
            viewProjInverse = GetOrthogonal().Inverse();
        }
        else {
            viewProjInverse = (GetProjection() * GetViewTranslate()).Inverse();
        }

        SR_MATH_NS::Ray ray;

        const float_t x = 2.0f * screenPos.x - 1.0f;
        const float_t y = 2.0f * screenPos.y - 1.0f;

        const float_t zNear = 0.f;
        const float_t zFar = 1.f - SR_FLT_EPSILON;

        auto&& origin = viewProjInverse.TransformVector(SR_MATH_NS::FVector4(x, y, zNear, 1.0f));
        ray.origin = origin.XYZ() / origin.w;

        auto&& rayEnd = viewProjInverse.TransformVector(SR_MATH_NS::FVector4(x, y, zFar, 1.0f));
        rayEnd /= rayEnd.w;

        ray.direction = (rayEnd.XYZ() - ray.origin).Normalize();

        return ray;
    }

    SR_MATH_NS::FVector3 Camera::ScreenToWorldPoint(const SR_MATH_NS::FVector3& screenPos) const {
        auto&& ray = GetScreenRay(screenPos.x, screenPos.y);
        auto&& viewSpaceDir = GetViewTranslate() * SR_MATH_NS::FVector4(-ray.direction, 1.f);
        const float_t rayDistance = (SR_MAX(screenPos.z - GetNear(), 0.0f) / viewSpaceDir.z);
        return (ray.origin + ray.direction * rayDistance);
    }

    SR_MATH_NS::FVector3 Camera::ScreenToWorldPoint(const SR_MATH_NS::FPoint& screenPos) const {
        return ScreenToWorldPoint(screenPos,GetNear());
    }

    SR_MATH_NS::FVector3 Camera::ScreenToWorldPoint(const SR_MATH_NS::FPoint& screenPos, float_t depth) const {
        return ScreenToWorldPoint(SR_MATH_NS::FVector3(screenPos.x, screenPos.y, depth));
    }

    float_t Camera::CalculateScreenFactor(const SR_MATH_NS::Matrix4x4& modelMatrix, float_t sizeClipSpace, bool orthogonal) const {
        return CalculateScreenFactor(modelMatrix, GetViewTranslate(), sizeClipSpace, orthogonal);
    }

    float_t Camera::CalculateScreenFactor(const SR_MATH_NS::Matrix4x4& modelMatrix, const SR_MATH_NS::Matrix4x4& viewMatrix, float_t sizeClipSpace, bool orthogonal) const {
        SR_MATH_NS::Matrix4x4 mvp;

        if (orthogonal) {
            mvp = GetOrthogonal() * modelMatrix;
        }
        else {
            mvp = GetProjection() * viewMatrix * modelMatrix;
        }

        auto&& modelInverse = modelMatrix.Inverse();

        auto&& rightViewInverse = orthogonal ? SR_MATH_NS::Matrix4x4::Identity().Inverse().v.right : viewMatrix.Inverse().v.right;
        rightViewInverse = modelInverse.TransformVector(rightViewInverse.XYZ());

        const float_t aspectRatio = GetAspect();
        const float_t rightLength = mvp.GetSegmentLengthClipSpace(SR_MATH_NS::FVector3(), rightViewInverse.XYZ(), aspectRatio);

        return sizeClipSpace / rightLength;
    }

    SR_MATH_NS::FVector3 Camera::GetCameraEye() const {
        return GetViewTranslate().Inverse().v.position.XYZ();
    }

    SR_MATH_NS::FVector3 Camera::GetCameraDir() const {
        return GetViewTranslate().Inverse().v.dir.XYZ();
    }

    SR_MATH_NS::FRect Camera::GetViewportRect() const {
        if (m_viewportRect) {
            return *m_viewportRect;
        }
        return SR_MATH_NS::FRect(0.f, 0.f, m_viewportSize.CastToFloat());
    }

    const RenderTechniqueData &Camera::GetRenderTechniqueData() const {
        if (!m_renderTechnique.pTechnique) {
            static const RenderTechniqueData emptyData;
            return emptyData;
        }
        return m_renderTechnique.pTechnique->GetRenderTechniqueData();
    }
}