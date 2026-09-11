//
// Created by Monika on 25.12.2022.
//

#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderContext.h>

#include <Utils/Common/StoreUtils.h>

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

        m_renderContext = (RenderContext*)SR_UTILS_NS::StoreUtils::Temp::GetPointer("RenderContext");
        if (!m_renderContext) {
            SRHalt("Render context is nullptr!");
            return;
        }

        m_pipeline = m_renderContext->GetPipeline();

        if (!m_pipeline) {
            SRHalt("Pipeline is nullptr!");
        }

        m_renderContext->Register(this, SR_UTILS_NS::PassKey<IGraphicsResource>(this));
    }

    void IGraphicsResource::DeInitGraphicsResource() {
        FreeVMemory();
        m_pipeline = nullptr;
        m_renderContext = nullptr;
    }
}
