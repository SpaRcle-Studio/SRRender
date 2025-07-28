//
// Created by Monika on 17.07.2022.
//

#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Pass/GroupPass.h>

#include <Codegen/RenderTechnique.generated.hpp>

namespace SR_GRAPH_NS {
    FileRenderTechnique::~FileRenderTechnique() {
        SetResource(nullptr);
    }

    FileRenderTechnique::Ptr FileRenderTechnique::Load(const SR_UTILS_NS::Path& path) {
        auto&& pResource = FileRenderTechniqueResource::Load(path);
        if (!pResource) {
            SR_ERROR("FileRenderTechnique::Load() : failed to load render technique resource from path: {}", path);
            return nullptr;
        }

        if (pResource->GetData().name.Empty()) {
            SR_ERROR("FileRenderTechnique::Load() : render technique resource name is not set!\n\tPath: {}", path);
            return nullptr;
        }

        auto&& pFileRenderTechnique = FileRenderTechnique::MakeShared<FileRenderTechnique>();
        pFileRenderTechnique->SetResource(pResource);

        RenderTechniqueData clone;
        pResource->GetData().CloneTo(clone);

        pFileRenderTechnique->SetRenderTechniqueData(std::move(clone));
        return pFileRenderTechnique;
    }

    void FileRenderTechnique::SetResource(const SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource>& pResource) {
        if (m_resource) {
            m_resource->RemoveUsePoint();
            m_resource = nullptr;
            m_onResourceReloaded.Reset();
        }

        if (pResource) {
            m_resource = pResource;
            m_resource->AddUsePoint();

            m_onResourceReloaded = pResource->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](auto&& msg){
                m_isResourceReloaded = true;
            });
        }
    }

    void FileRenderTechnique::UpdateDataIfNeeded() {
        if (!m_resource || !m_isResourceReloaded) {
            return;
        }

        SR_LOG("FileRenderTechnique::UpdateDataIfNeeded() : reloading render technique data from resource: {}", m_resource->GetResourcePath());

        RenderTechniqueData clone;
        m_resource->GetData().CloneTo(clone);
        SetRenderTechniqueData(std::move(clone));

        m_isResourceReloaded = false;
    }

    /// ================================================================================================================

    FileRenderTechniqueResource::Ptr FileRenderTechniqueResource::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;
        return SR_UTILS_NS::Asset::Load<FileRenderTechniqueResource>(rawPath);
    }
}
