//
// Created by Monika on 17.07.2022.
//

#ifndef SR_ENGINE_RENDERTECHNIQUE_H
#define SR_ENGINE_RENDERTECHNIQUE_H

#include <Graphics/Render/IRenderTechnique.h>

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

    protected:
        bool Build() override;

    private:
        void LoadPass(const SR_XML_NS::Node& node);
        void ProcessNode(const SR_XML_NS::Node& passNode);

        void SetResource(const SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource>& pResource);

    private:
        SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource> m_resource;

    };

    class FileRenderTechniqueResource : public SR_UTILS_NS::Settings {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource>;

    public:
        FileRenderTechniqueResource() = default;

    public:
        static FileRenderTechniqueResource::Ptr Load(const SR_UTILS_NS::Path& path);

    public:
        void RegisterRenderTechnique(const FileRenderTechnique::Ptr& renderTechnique) {
            m_renderTechniques.insert(renderTechnique);
        }

        void UnregisterRenderTechnique(const FileRenderTechnique::Ptr& renderTechnique) {
            m_renderTechniques.erase(renderTechnique);
        }

    protected:
        bool Load() override;
        bool Unload() override;

    protected:
        std::set<FileRenderTechnique::Ptr> m_renderTechniques;

    };
}

#endif //SR_ENGINE_RENDERTECHNIQUE_H
