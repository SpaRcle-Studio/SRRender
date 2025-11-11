//
// Created by Monika on 25.12.2022.
//

#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderContext.h>

#include <Utils/Types/DataStorage.h>

namespace SR_GRAPH_NS::Memory {
    IGraphicsResource::~IGraphicsResource() {
        if (IsGraphicsResourceRegistered()) {
            SRHalt("IGraphicsResource is not deinitialized before destruction!");
        }
    }

    void IGraphicsResource::RegisterGraphicsResource() {
        if (m_renderContext) {
            return;
        }

        auto&& pContext = SR_THIS_THREAD->GetContext()->GetValue<SR_HTYPES_NS::SafePtr<RenderContext>>();
        if (!pContext) {
            SRHalt("Render context is nullptr!");
            return;
        }

        m_renderContext = pContext.Get();
        m_pipeline = m_renderContext->GetPipeline();

        if (!m_pipeline) {
            SRHalt("Pipeline is nullptr!");
        }

        m_renderContext->Register(this, SR_UTILS_NS::PassKey<IGraphicsResource>(this));
    }

    void IGraphicsResource::DeInitGraphicsResource(SR_UTILS_NS::PassKey<RenderContext>) {
        FreeVMemory();
        m_pipeline = nullptr;
        m_renderContext = nullptr;
    }
}
