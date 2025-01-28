//
// Created by Monika on 05.01.2025.
//

#ifndef SR_GRAPHICS_RENDER_I_RENDERER_H
#define SR_GRAPHICS_RENDER_I_RENDERER_H

#include <Utils/Types/SharedPtr.h>
#include <Utils/TypeTraits/SRClass.h>

namespace SR_GRAPH_NS {
    class RenderScene;

    class IRenderer : public SR_UTILS_NS::SRClass, public SR_HTYPES_NS::SharedPtr<IRenderer> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<IRenderer>;

        explicit IRenderer()
            : SR_UTILS_NS::SRClass()
            , SR_HTYPES_NS::SharedPtr<IRenderer>(this, SR_UTILS_NS::SharedPtrPolicy::Manually)
        { }

    public:
        SR_NODISCARD RenderScene* GetRenderScene() const noexcept { return m_renderScene; }

        void SetRenderScene(RenderScene* pRenderScene) noexcept { m_renderScene = pRenderScene; }

        virtual void Clear() { }
        virtual void Init() { }
        virtual void DeInit() { }
        virtual void Prepare() { }
        virtual void PostUpdate() { }
        virtual bool IsEmpty() const noexcept { return true; }

    protected:
        mutable std::recursive_mutex m_mutex;

    private:
        RenderScene* m_renderScene = nullptr;

    };
}

#endif //SR_GRAPHICS_RENDER_I_RENDERER_H
