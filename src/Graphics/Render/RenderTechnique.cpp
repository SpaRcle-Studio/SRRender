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

        auto&& pFileRenderTechnique = FileRenderTechnique::MakeShared<FileRenderTechnique>();
        pFileRenderTechnique->SetResource(pResource);
        pFileRenderTechnique->SetDirty();
        return pFileRenderTechnique;
    }

    void FileRenderTechnique::SetResource(const SR_HTYPES_NS::SharedPtr<FileRenderTechniqueResource>& pResource) {
        if (m_resource) {
            RemoveDependency(m_resource.StaticCast<SR_UTILS_NS::ResourceContainer>());
            m_resource = nullptr;
        }

        if (pResource) {
            m_resource = pResource;
            AddDependency(pResource.StaticCast<SR_UTILS_NS::ResourceContainer>());
        }
    }

    bool FileRenderTechnique::Build() {
        SR_TRACY_ZONE;

        /// Метод выполняется в графическом контексте

        if (m_hasErrors) {
            return false;
        }

        if (!m_dirty) {
            SRHalt("RenderTechnique::Build() : render technique isn't dirty!");
            return false;
        }

        if (!m_resource) {
            SR_ERROR("RenderTechnique::Build() : resource is nullptr!");
            return false;
        }

        /// Очишаем старые данные, если они были
        DeInitPasses();
        m_queues.clear();
        SetName(SR_UTILS_NS::StringAtom());

        /// Загружаем новые данные
        auto&& document = m_resource->LoadDocument();
        if (!document.Valid()) {
            SR_ERROR("RenderTechnique::Build() : failed to load xml document!");
            return false;
        }

        if (auto&& settings = document.Root().GetNode("Technique")) {
            SetName(settings.GetAttribute("Name").ToString());

            for (auto&& passNode : settings.GetNodes()) {
                ProcessNode(passNode);
            }

            if (!m_passes.empty() && m_queues.empty()) {
                SR_ERROR("RenderTechnique::Build() : passes was loaded, but queue is empty!");
                return false;
            }
        }
        else {
            SR_ERROR("RenderTechnique::Build() : \"Technique\" node not found!");
            return false;
        }

        SR_GRAPH_LOG("RenderTechnique::Build() : building \"" + std::string(GetName()) + "\" render technique...");

        /// Инициализируем все успешно загруженнеы проходы
        IRenderTechnique::Init();

        m_dirty = false;

        return true;
    }

    void FileRenderTechnique::LoadPass(const SR_XML_NS::Node& node) {
        if (auto&& pPass = SR_ALLOCATE_RENDER_PASS(node, this)) {
            m_passes.emplace_back(pPass);
        }
        else {
            SR_ERROR("FileRenderTechnique::LoadPass() : failed to load \"" + node.Name() + "\" pass!");
        }
    }

    void FileRenderTechnique::ProcessNode(const SR_XML_NS::Node& passNode) {
        if (passNode.NameView() == "Include") {
            auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(passNode.GetAttribute("Path").ToString());
            auto&& includeXml = SR_XML_NS::Document::Load(path);
            if (includeXml) {
                for (auto&& includePassNode : includeXml.Root().GetNode("Include").GetNodes()) {
                    ProcessNode(includePassNode);
                }
            }
            return;
        }

        if (passNode.NameView() == "FrameBufferController") {
            auto&& name = passNode.GetAttribute("Name").ToString();
            auto&& pFrameBufferController = FrameBufferController::MakeShared();
            if (pFrameBufferController->LoadFramebufferSettings(passNode)) {
                m_frameBufferControllers[name] = pFrameBufferController;
            }
            else {
                SR_ERROR("FileRenderTechnique::ProcessNode() : failed to load \"" + name + "\" framebuffer controller!");
                pFrameBufferController.AutoFree();
            }
            return;
        }

        if (passNode.NameView() == "Queues") {
            for (auto&& queueNode : passNode.GetNodes()) {
                auto&& queue = m_queues.emplace_back();
                for (auto&& queuePassNode : queueNode.GetNodes()) {
                    auto&& name = queuePassNode.GetAttribute("Name").ToString();
                    if (auto&& pPass = FindPass(name)) {
                        queue.emplace_back(pPass);
                    }
                    else {
                        SR_ERROR("FileRenderTechnique::ProcessNode() : pass \"" + name + "\" for queue not found!");
                    }
                }
            }
            return;
        }

        LoadPass(passNode);
    }

    /// ================================================================================================================

    FileRenderTechniqueResource::Ptr FileRenderTechniqueResource::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;
        return SR_UTILS_NS::ResourceManager::Instance().GetOrLoadResource<FileRenderTechniqueResource>(rawPath);
    }

    bool FileRenderTechniqueResource::Load() {
        for (auto&& pRenderTechnique : m_renderTechniques) {
            pRenderTechnique->SetDirty();
        }

        m_loadState = LoadState::Loading;

        return true;
    }

    bool FileRenderTechniqueResource::Unload() {
        m_loadState = LoadState::Unloading;

        return Settings::Unload();
    }
}
