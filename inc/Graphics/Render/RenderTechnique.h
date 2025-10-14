//
// Created by Monika on 17.07.2022.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_H
#define SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_H

#include <Graphics/Render/IRenderTechnique.h>

#include <Utils/Resources/Asset.h>

namespace SR_GRAPH_NS {
    class FileRenderTechniqueResource;

    class FileRenderTechnique : public IRenderTechnique {
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FileRenderTechnique>;

    public:
        FileRenderTechnique() = default;
        ~FileRenderTechnique() override;

    public:
        static FileRenderTechnique::Ptr Load(const SR_UTILS_NS::Path& path);

    private:
        void SetResource(const SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource>& pResource);
        void UpdateDataIfNeeded() override;

    private:
        SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource> m_resource;
        SR_UTILS_NS::Subscription m_onResourceReloaded;
        bool m_isResourceReloaded = false;

    };

    /// @extension(srtech)
    class FileRenderTechniqueResource : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource>;

    public:
        static FileRenderTechniqueResource::Ptr Load(const SR_UTILS_NS::Path& path);

    public:
        const RenderTechniqueData& GetData() const noexcept { return m_data; }

    private:
        /// @property @noHeader
        RenderTechniqueData m_data;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_H
