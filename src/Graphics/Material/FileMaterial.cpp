//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Types/Shader.h>

namespace SR_GRAPH_NS {
    FileMaterial::FileMaterial()
        : BaseMaterial()
        , IResource(SR_COMPILE_TIME_CRC32_TYPE_NAME(FileMaterial))
    { }

    FileMaterial::~FileMaterial() = default;

    uint32_t FileMaterial::RegisterMesh(BaseMaterial::MeshPtr pMesh) {
        AddUsePoint();
        return BaseMaterial::RegisterMesh(pMesh);
    }

    void FileMaterial::UnregisterMesh(uint32_t* pId) {
        RemoveUsePoint();
        BaseMaterial::UnregisterMesh(pId);
    }

    SR_UTILS_NS::IResource::Ptr FileMaterial::CopyResource(SR_UTILS_NS::IResource::Ptr pDestination) const {
        SRHalt("Material is not are copyable!");
        return nullptr;
    }

    FileMaterial::Ptr FileMaterial::Load(SR_UTILS_NS::Path rawPath) {
        SR_TRACY_ZONE;

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();

        FileMaterial* pMaterial = nullptr;

        resourceManager.Execute([&](){
            auto&& path = rawPath.SelfRemoveSubPath(resourceManager.GetResPathRef());
            if (!resourceManager.GetResPathRef().Concat(path).Exists()) {
                SR_WARN("Material::Load() : path to the material doesn't exist! Loading is aborted.\n\tPath: " + path.ToStringRef());
                return;
            }

            if ((pMaterial = resourceManager.Find<FileMaterial>(path))) {
                return;
            }

            pMaterial = new FileMaterial();

            pMaterial->SetId(path.ToStringRef(), false);

            if (!pMaterial->Reload()) {
                delete pMaterial;
                pMaterial = nullptr;
                return;
            }

            resourceManager.RegisterResource(pMaterial);
        });

        return pMaterial;
    }

    void FileMaterial::OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) {
        bool hasChangedTexture = false;

        for (auto&& pTexture : GetTexturesFromMatProperties(m_properties)) {
            if (pTexture == pContainer) {
                hasChangedTexture = true;
                break;
            }
        }

        if (hasChangedTexture || m_dirtyShader) {
            m_meshes.ForEach([](uint32_t, auto&& pMesh) {
                pMesh->MarkMaterialDirty();
            });
        }

        IResource::OnResourceUpdated(pContainer, depth);
    }

    bool FileMaterial::Load() {
        SR_TRACY_ZONE;

        const auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(GetResourcePath());

        auto&& document = SR_XML_NS::Document::Load(path);
        if (!document.Valid()) {
            SR_ERROR("Material::Load() : file is not found! \n\tPath: " + path.ToString());
            return false;
        }

        auto&& matXml = document.Root().GetNode("Material");
        if (!matXml) {
            SR_ERROR("Material::Load() : \"Material\" node is not found! \n\tPath: " + path.ToString());
            return false;
        }

        if (auto&& shader = matXml.TryGetNode("Shader")) {
            SetShader(SR_GTYPES_NS::Shader::Load(shader.GetAttribute("Path").ToString()));
        }
        else {
            SR_ERROR("Material::Load() : the material have not shader!");
            return false;
        }

        LoadProperties(matXml.TryGetNode("Properties"));

        return IResource::Load();
    }

    bool FileMaterial::LoadProperties(const SR_XML_NS::Node& propertiesNode) {
        InitMaterialProperties();
        /// Применяем сохраненные в материале значения
        LoadMaterialProperties(GetResourcePath().ToStringRef(), propertiesNode, &m_properties);
        return true;
    }

    bool FileMaterial::Unload() {
        SR_TRACY_ZONE;
        SetShader(nullptr);

        m_properties.ClearContainer();

        return IResource::Unload();
    }

    bool FileMaterial::Reload() {
        SR_TRACY_ZONE;

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::Medium) {
            SR_LOG("Material::Reload() : reloading \"" + std::string(GetResourceId()) + "\" material...");
        }

        m_loadState = LoadState::Reloading;

        /// ========= Stash Properties =========
        auto&& stashTextures = GetTexturesFromMatProperties(m_properties);
        for (auto&& pTexture : stashTextures) {
            pTexture->AddUsePoint();
        }

        SR_GTYPES_NS::Shader* stashShader = m_shader;
        if (stashShader) {
            stashShader->AddUsePoint();
        }
        /// ========= Stash Properties =========

        Unload();

        if (!Load()) {
            m_loadState = LoadState::Error;
            return false;
        }

        /// ========= UnStash Properties =========
        for (auto&& pTexture : stashTextures) {
            pTexture->RemoveUsePoint();
        }
        if (stashShader) {
            stashShader->RemoveUsePoint();
        }
        /// ========= UnStash Properties =========

        UpdateResources();

        m_context.Do([](RenderContext* ptr) {
            ptr->SetDirty();
        });

        return true;
    }

    SR_UTILS_NS::Path FileMaterial::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    void FileMaterial::AddMaterialDependency(SR_UTILS_NS::IResource::Ptr pResource) {
        AddDependency(pResource);
    }

    void FileMaterial::RemoveMaterialDependency(SR_UTILS_NS::IResource::Ptr pResource) {
        RemoveDependency(pResource);
    }

    void FileMaterial::InitContext() {
        if (!m_context) {
            BaseMaterial::InitContext();
            if (m_context) {
                m_context->Register(this);
            }
        }
    }

    void FileMaterial::DeleteResource() {
        FinalizeMaterial();
        IResource::DeleteResource();
    }
}