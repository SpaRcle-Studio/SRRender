//
// Created by Monika on 13.07.2022.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_CONTEXT_H
#define SR_ENGINE_GRAPHICS_RENDER_CONTEXT_H

#include <Graphics/Pipeline/PipelineType.h>
#include <Graphics/Settings/RenderSettings.h>
#include <Graphics/Material/FileMaterial.h>

#include <Utils/World/Scene.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Types/SafePointer.h>
#include <Utils/Common/PassKey.h>

namespace SR_GTYPES_NS {
    class Framebuffer;
    class Shader;
    class Camera;
    class Texture;
    class Skybox;
}

namespace SR_GRAPH_NS {
    namespace Memory {
        class IGraphicsResource;
    }

    class BaseMaterial;
    class Window;
    class RenderScene;
    class IRenderTechnique;
    class Pipeline;

    SR_ENUM_NS_CLASS_T(RCUpdateQueueState, uint8_t,
       Begin = 0,
       Framebuffers,
       Shaders,
       Textures,
       Techniques,
       Skyboxes,
       End
    );

    /**
     * Здесь хранятся все контекстные ресурсы.
     * Исключение - меши, потому что они могут быть в нескольких экземплярах.
     */
    class RenderContext : public SR_HTYPES_NS::SafePtr<RenderContext> {
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::Pipeline>;
        using Super = SR_HTYPES_NS::SafePtr<RenderContext>;
        using MaterialPtr = SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::BaseMaterial>;
        using TexturePtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Texture>;
        using SkyboxPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Skybox>;
        using FramebufferPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>;
        using CameraPtr = SR_GTYPES_NS::Camera*;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
        using RenderTechniquePtr = SR_HTYPES_NS::SharedPtr<IRenderTechnique>;
        using WindowPtr = SR_HTYPES_NS::SharedPtr<Window>;
        using RenderScenes = std::list<std::pair<SR_WORLD_NS::Scene::Ptr, RenderScenePtr>>;
        using Definitions = std::map<SR_UTILS_NS::StringAtom, std::string>;
    public:
        using Ptr = SR_HTYPES_NS::SafePtr<RenderContext>;

    public:
        RenderContext();
        virtual ~RenderContext();

    public:
        void SwitchWindow(WindowPtr pWindow);

        void PrepareFrame();

        bool Update() noexcept;

        bool PreInit();
        bool Init();
        void Close();

        void SetDirty();

        void OnResize(const SR_MATH_NS::UVector2& size);
        void OnMultiSampleChanged();

    public:
        RenderScenePtr CreateScene(const SR_WORLD_NS::Scene::Ptr& scene);

        void Register(Memory::IGraphicsResource* pResource, SR_UTILS_NS::PassKey<Memory::IGraphicsResource>);

        SR_NODISCARD bool IsOptimizedRenderUpdateEnabled() const noexcept { return m_isOptimizedUpdateEnabled; }
        SR_NODISCARD bool IsFrustumCullingEnabled() const noexcept { return m_isFrustumCullingEnabled; }
        SR_NODISCARD bool IsEmpty() const;
        SR_NODISCARD bool IsDirty() const;
        SR_NODISCARD const RenderContext::PipelinePtr& GetPipeline() const;
        SR_NODISCARD RenderContext::PipelinePtr& GetPipeline();
        SR_NODISCARD WindowPtr GetWindow() const;
        SR_NODISCARD PipelineType GetPipelineType() const;
        SR_NODISCARD MaterialPtr GetDefaultMaterial() const;
        SR_NODISCARD MaterialPtr GetDefaultUIMaterial() const;
        SR_NODISCARD TexturePtr GetDefaultTexture() const;
        SR_NODISCARD TexturePtr GetNoneTexture() const;
        SR_NODISCARD ShaderPtr GetCurrentShader() const noexcept;
        SR_NODISCARD FramebufferPtr FindFramebuffer(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD FramebufferPtr FindFramebuffer(SR_UTILS_NS::StringAtom name, CameraPtr pCamera) const;
        SR_NODISCARD SR_MATH_NS::UVector2 GetWindowSize() const;
        SR_NODISCARD const std::vector<ShaderPtr>& GetShaders() const noexcept;
        SR_NODISCARD const std::vector<SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>>& GetFramebuffers() const noexcept;
        SR_NODISCARD const std::vector<TexturePtr>& GetTextures() const noexcept;
        SR_NODISCARD const std::vector<RenderTechniquePtr>& GetRenderTechniques() const noexcept;
        SR_NODISCARD const std::vector<SkyboxPtr>& GetSkyboxes() const noexcept;
        SR_NODISCARD const RenderScenes& GetScenes() const noexcept { return m_scenes; }
        SR_NODISCARD const RenderSettingsPreset& GetSettingsPreset() const noexcept;
        SR_NODISCARD Definitions GetShaderMacros() const;
        SR_NODISCARD const RenderSettings& GetSettings() const noexcept;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetActivePreset() const noexcept { return m_activePreset; }

        void SetActivePreset(SR_UTILS_NS::StringAtom name);

        void SetOptimizedRenderUpdateEnabled(bool enabled) noexcept { m_isOptimizedUpdateEnabled = enabled; }
        bool SetCurrentShader(ShaderPtr pShader);
        void GarbageCollect() { m_isNeedGarbageCollection = true; }

        SR_NODISCARD bool IsMacroDefined(SR_UTILS_NS::StringAtom define) const { return m_definitions.find(define) != m_definitions.end(); }

        void SetMacro(SR_UTILS_NS::StringAtom define, std::optional<std::string> value = std::nullopt);
        void RemoveMacro(SR_UTILS_NS::StringAtom define);
        void SwitchMacro(SR_UTILS_NS::StringAtom define, bool enable) { if (enable) { SetMacro(define); } else { RemoveMacro(define); } }

        void ReloadShaders();

    private:
        bool LoadDefaultResources();
        bool InitPipeline();

    private:
        RCUpdateQueueState m_updateState = RCUpdateQueueState::Begin;
        SR_UTILS_NS::StringAtom m_activePreset;

        std::vector<SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>> m_framebuffers;
        std::vector<SR_HTYPES_NS::SharedPtr<IRenderTechnique>> m_techniques;
        std::vector<ShaderPtr> m_shaders;
        std::vector<TexturePtr> m_textures;
        std::vector<SkyboxPtr> m_skyboxes;
        Definitions m_definitions;

        RenderScenes m_scenes;

        WindowPtr m_window;

        RenderSettings::Ptr m_settings;
        SR_UTILS_NS::Subscription m_onSettingsReloaded;

        MaterialPtr m_defaultUIMaterial;
        MaterialPtr m_defaultMaterial;
        TexturePtr m_defaultTexture;
        TexturePtr m_noneTexture;

        PipelinePtr m_pipeline;

        bool m_isClosed = false;
        bool m_hasChangedFrameBuffers = false;

        bool m_isFrustumCullingEnabled = true;
        bool m_isNeedGarbageCollection = false;
        bool m_isOptimizedUpdateEnabled = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_CONTEXT_H
