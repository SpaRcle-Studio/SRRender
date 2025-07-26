//
// Created by Monika on 18.07.2022.
//

#ifndef SR_ENGINE_IGRAHPICSRESOURCE_H
#define SR_ENGINE_IGRAHPICSRESOURCE_H

#include <Graphics/macros.h>

#include <Utils/Debug.h>
#include <Utils/Common/PassKey.h>

namespace SR_GRAPH_NS {
    class RenderContext;
    class Pipeline;
}

namespace SR_GRAPH_NS::Memory {
    /// Не наследуемся от NonCopyable, чтобы не возникло конфликтов с IResource
    class IGraphicsResource {
    public:
        using RenderContextPtr = RenderContext*;
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<Pipeline>;
    protected:
        IGraphicsResource() = default;
        virtual ~IGraphicsResource();

    public:
        IGraphicsResource(const IGraphicsResource&) = delete;
        IGraphicsResource& operator=(const IGraphicsResource&) = delete;

    public:
        void RegisterGraphicsResource();
        void DeInitGraphicsResource(SR_UTILS_NS::PassKey<RenderContext>);

        SR_NODISCARD const PipelinePtr& GetPipeline() const noexcept { return m_pipeline; }
        SR_NODISCARD const RenderContextPtr& GetRenderContext() const noexcept { return m_renderContext; }
        SR_NODISCARD bool IsGraphicsResourceRegistered() const noexcept { return m_pipeline; }

    protected:
        virtual void FreeVMemory() { }

    private:
        PipelinePtr m_pipeline;
        RenderContextPtr m_renderContext = nullptr;

    };
}

#endif //SR_ENGINE_IGRAHPICSRESOURCE_H
