//
// Created by Monika on 10.10.2023.
//

#ifndef SR_ENGINE_GRAPHICS_I_RENDER_TECHNIQUE_H
#define SR_ENGINE_GRAPHICS_I_RENDER_TECHNIQUE_H

#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Pass/GroupPass.h>

#include <Utils/Settings.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Types/SafePointer.h>

namespace SR_GTYPES_NS {
    class Camera;
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class RenderContext;
    class BasePass;

    struct RenderTechniqueData : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        void SetRenderStagesSettingsPath(const SR_UTILS_NS::Path& path);

        /// @property
        SR_UTILS_NS::StringAtom name;
        /// @property @setter(SetRenderStagesSettingsPath)
        /// @customArgs(pick: enabled, filter name: Render stages settings, relative: resources)
        /// @customArg(filter value: sra)
        SR_UTILS_NS::Path renderStagesSettings;
        /// @property @notNull
        BasePass::Ptr pass;
        /// @property
        std::vector<FrameBufferController::Ptr> frameBuffers;

    };

    class IRenderTechnique : public Memory::IGraphicsResource, public SR_HTYPES_NS::SharedPtr<IRenderTechnique> {
    public:
        using CameraPtr = Types::Camera*;
        using Super = SR_HTYPES_NS::SharedPtr<IRenderTechnique>;
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<IRenderTechnique>;

    public:
        IRenderTechnique();
        ~IRenderTechnique() override;

    public:
        void PrepareFrame();
        void PrepareRender();
        bool Overlay();
        bool Render();
        void Update();
        void PostUpdate();
        void SetDirty();

        void KillTechnique();

        void FreeVMemory() override;

        void SetCamera(CameraPtr pCamera);
        void SetRenderScene(const RenderScenePtr& pRScene);
        void SetRenderTechniqueData(RenderTechniqueData&& data);

        SR_NODISCARD const RenderTechniqueData& GetRenderTechniqueData() const noexcept { return m_data; }
        SR_NODISCARD RenderTechniqueData& GetRenderTechniqueData() noexcept { return m_data; }
        SR_NODISCARD const CameraPtr& GetCamera() const noexcept { return m_camera; }
        SR_NODISCARD const RenderScenePtr& GetRenderScene() const noexcept { return m_renderScene; }
        SR_NODISCARD bool IsEmpty() const;
        SR_NODISCARD bool IsTechniqueDead() const;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetName() const noexcept { return m_data.name; }

        void OnResize(const SR_MATH_NS::UVector2& size);
        void OnCameraParamsChanged();
        void OnMultisampleChanged();
        void OnHierarchyChanged();

        void ForEachPass(const std::function<void(BasePass&)>& func);

        SR_NODISCARD BasePass* FindPass(SR_UTILS_NS::StringAtom name) const;

        template<typename T> SR_NODISCARD T* FindPassAs() const {
            SR_TRACY_ZONE;
            if (auto&& pPass = FindPass(T::GetClassStaticName())) {
                return static_cast<T*>(pPass);
            }
            return nullptr;
        }

        SR_NODISCARD const FrameBufferController::Ptr& GetFrameBufferController(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD const std::vector<FrameBufferController::Ptr>& GetFrameBufferControllers() const;

    private:
        bool Init();
        bool BuildTechnique();
        void DeInitPasses();
        void ReleaseFrameBuffers();
        void UpdateFrustumCulling();

        virtual void UpdateDataIfNeeded() { }

    protected:
        std::optional<SR_MATH_NS::UVector2> m_surfaceSize;
        RenderTechniqueData m_data;
        RenderScenePtr m_renderScene;
        CameraPtr m_camera = nullptr;
        bool m_modulesApplied = false;
        std::atomic<bool> m_dirty = true;
        std::atomic<bool> m_hasErrors = false;
        std::atomic<bool> m_isDead = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_I_RENDER_TECHNIQUE_H
