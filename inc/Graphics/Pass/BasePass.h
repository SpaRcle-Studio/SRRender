//
// Created by Monika on 14.07.2022.
//

#ifndef SR_ENGINE_BASE_PASS_H
#define SR_ENGINE_BASE_PASS_H

#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Pass/Data/SamplersPassData.h>

#include <Utils/Common/NonCopyable.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Types/Function.h>
#include <Utils/Types/SafePointer.h>
#include <Utils/Types/Time.h>
#include <Utils/Resources/Xml.h>
#include <Utils/Resources/ResourceContainer.h>

namespace SR_GTYPES_NS {
    class Camera;
    class Mesh;
    class Shader;
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class RenderContext;
    class IRenderTechnique;
    class Pipeline;
    class BasePass;
    class FrameBufferPass;

    /// @abstract
    class BasePass : public SR_HTYPES_NS::SharedPtr<BasePass>, public SR_UTILS_NS::Serializable {
        SR_CLASS()
        using Super = SR_HTYPES_NS::SharedPtr<BasePass>;
    public:
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using CameraPtr = SR_GTYPES_NS::Camera*;
        using RenderContextPtr = RenderContext*;
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<Pipeline>;
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using FrameBuffers = std::vector<SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<BasePass>;

    public:
        BasePass();
        ~BasePass() override = default;

    public:
        virtual bool PreInit();
        virtual bool Init();
        virtual void DeInit();

        virtual bool HasPreRender() const noexcept { return true; }
        virtual bool HasRender() const noexcept { return true; }
        virtual bool HasPostRender() const noexcept { return true; }
        virtual bool HasUpdate() const noexcept { return true; }

        /// Вызывается всегда и в самом начале
        virtual bool Overlay() { return false; }

        /// Вызывается перед PreRender, Render, PostRender, Update
        virtual void Bind() { }

        /// Вызывается всегда но полсе оверлея
        virtual bool Prepare();

        /// Вызывается только во время построения
        virtual bool PreRender() { return false; }
        /// Вызывается только во время построения
        virtual bool Render() { return false; }
        /// Вызывается только во время построения
        virtual bool PostRender() { return false; }

        /// Вызывается постоянно после построения
        virtual void Update() { }
        virtual void PostUpdate() { }

        virtual void OnResize(const SR_MATH_NS::UVector2& size);
        virtual void OnMultisampleChanged();
        virtual void OnCameraParamsChanged();

        virtual void UseUniformsFromAnotherPass(SR_GTYPES_NS::Shader& shader) { }
        virtual void UseSSBOFromAnotherPass(SR_GTYPES_NS::Shader& shader) { }
        virtual void UseSamplers(SR_GTYPES_NS::Shader& shader);
        virtual void SetRenderTechnique(IRenderTechnique* pRenderTechnique);

        void SetCustomName(const SR_UTILS_NS::StringAtom& name) { m_customName = name; }

        SR_NODISCARD SR_UTILS_NS::StringAtom GetPassName() const;
        SR_NODISCARD bool IsActive() const;
        SR_NODISCARD const RenderScenePtr& GetRenderScene() const;
        SR_NODISCARD const CameraPtr& GetCamera() const;
        SR_NODISCARD const RenderContextPtr& GetRenderContext() const;
        SR_NODISCARD const PipelinePtr& GetPipeline() const;
        SR_NODISCARD IRenderTechnique* GetTechnique() const { return m_technique; }
        SR_NODISCARD bool IsInit() const { return m_isInit; }
        SR_NODISCARD virtual BasePass* FindPass(SR_UTILS_NS::StringAtom name);

        template<typename T> T* FindPassAs(SR_UTILS_NS::StringAtom name) {
            SR_TRACY_ZONE;
            if (auto&& pPass = FindPass(name)) {
                return dynamic_cast<T*>(pPass);
            }
            return nullptr;
        }

        SR_NODISCARD FrameBufferPass* GetFrameBufferPass() const;
        SR_NODISCARD uint32_t GetColorLayersCount() const;

        SR_NODISCARD BasePass* GetParent() const { return m_parent; }
        virtual void SetParent(BasePass* pParent) { m_parent = pParent; }

        virtual void ForEachPass(const std::function<void(BasePass&)>& func);
        virtual bool UpdateFrustum() { return false; }

    protected:
        Memory::UBOManager& m_uboManager;
        DescriptorManager& m_descriptorManager;

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_customName;

    private:
        BasePass* m_parent = nullptr;

        IRenderTechnique* m_technique = nullptr;
        bool m_isInit = false;

    };
}

#endif //SR_ENGINE_BASE_PASS_H
