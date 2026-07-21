//
// Created by Monika on 17.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_TARGET_H
#define SR_ENGINE_GRAPHICS_RENDER_TARGET_H

#include <Graphics/Pipeline/TextureHelper.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/FileSystem/Path.h>
#include <Utils/Types/Optional.h>

namespace SR_GRAPH_NS {
    class BasePass;
    class SkyboxPass;
    class RenderScene;
    class IRenderTechnique;
}

namespace SR_GTYPES_NS {
    class Camera;
    class Framebuffer;

    struct RenderTargetLayer : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        ImageFormat format = ImageFormat::RGBA8_UNORM;
        /// @property
        SR_MATH_NS::FColor clearColor;
    };

    SR_ENUM_NS_CLASS_T(RenderTargetMode, uint8_t,
        CameraIntegration,
        CameraShare,
        Custom
    );

    /// @category(Render)
    class RenderTarget : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = Component;
    public:
        void Update(float dt) override;
        void OnDisable() override;
        void OnDetached() override;

        void Activate();
        void Deactivate();

        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        SR_NODISCARD SR_UTILS_NS::StringAtom GetName() const { return m_name; }
        SR_NODISCARD Framebuffer* GetFramebuffer() const;

    private:
        SR_NODISCARD RenderScene* GetRenderScene() const;
        SR_NODISCARD IRenderTechnique* GetRenderTechnique() const;

    private:
        mutable SR_HTYPES_NS::SharedPtr<RenderScene> m_renderScene;
        SR_HTYPES_NS::SharedPtr<IRenderTechnique> m_usedRenderTechnique;
        SR_UTILS_NS::Vector<SR_HTYPES_NS::SharedPtr<BasePass>> m_cachedPasses;
        SR_UTILS_NS::Vector<ImageFormat> m_cachedFormats;
        RenderTargetMode m_cachedMode = RenderTargetMode::CameraIntegration;

    private:
        /// @property @onChanged(Deactivate)
        SR_UTILS_NS::StringAtom m_name;
        /// @property @onChanged(Deactivate)
        RenderTargetMode m_mode = RenderTargetMode::CameraIntegration;

        /// @property @onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::Custom)
        SR_UTILS_NS::EntityRef<Camera> m_camera;

        /// @property @onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::CameraShare)
        SR_UTILS_NS::Vector<SR_HTYPES_NS::SharedPtr<BasePass>> m_passes;

        /// @property @onChanged(Deactivate) @condition(This.m_mode == RenderTargetMode::CameraShare)
        SR_UTILS_NS::StringAtom m_frameBufferName;

        /// @property @group(Framebuffer) @onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::CameraShare)
        bool m_dynamicResolution = false;
        /// @property @group(Framebuffer) @onChanged(Deactivate) @condition(!This.m_dynamicResolution && This.m_mode != RenderTargetMode::CameraShare)
        SR_MATH_NS::UVector2 m_resolution = { 1920, 1080 };
        /// @property @group(Framebuffer) @onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::CameraShare)
        SR_MATH_NS::FVector2 m_resolutionScale = { 1.f, 1.f };
        /// @property @group(Framebuffer) @onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::CameraShare)
        uint32_t m_sampleCount = 1;
        /// @property @group(Framebuffer) @onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::CameraShare)
        SR_UTILS_NS::Vector<RenderTargetLayer> m_layers;
        /// @property @group(Framebuffer)@onChanged(Deactivate) @condition(This.m_mode != RenderTargetMode::CameraShare)
        SR_UTILS_NS::Optional<float_t> m_clearDepth = 1.f;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_TARGET_H