//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/FileMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    FileMaterialResource::FileMaterialResource()
        : IResource(SR_COMPILE_TIME_CRC32_TYPE_NAME(FileMaterialResource))
    { }

    SR_UTILS_NS::IResource::Ptr FileMaterialResource::CopyResource(SR_UTILS_NS::IResource::Ptr pDestination) const {
        SRHalt("Material is not are copyable!");
        return nullptr;
    }

    FileMaterialResource* FileMaterialResource::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();

        FileMaterialResource* pMaterial = nullptr;

        resourceManager.Execute([&](){
            auto&& path = rawPath.SelfRemoveSubPath(resourceManager.GetResPathRef());
            if (!resourceManager.GetResPathRef().Concat(path).Exists()) {
                SR_WARN("FileMaterialResource::Load() : path to the material doesn't exist! Loading is aborted.\n\tPath: " + path.ToStringRef());
                return;
            }

            if ((pMaterial = resourceManager.Find<FileMaterialResource>(path))) {
                return;
            }

            pMaterial = new FileMaterialResource();

            pMaterial->SetId(path.ToStringRef(), false);

            if (!pMaterial->Reload()) {
                pMaterial->DeleteResource();
                pMaterial = nullptr;
                return;
            }

            resourceManager.RegisterResource(pMaterial);
        });

        return pMaterial;
    }

    bool FileMaterialResource::Load() {
        SR_TRACY_ZONE;

        const auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(GetResourcePath());

        SR_UTILS_NS::SRADeserializer deserializer;
        if (!deserializer.LoadFromFile(path)) {
            SR_ERROR("FileMaterial::Load() : failed to deserialize material from file! \n\tPath: " + path.ToString());
            return false;
        }

        m_data = SRNew<MaterialData>();
        m_data->Load(deserializer);

        return Super::Load();
    }

    bool FileMaterialResource::Unload() {
        SR_TRACY_ZONE;
        m_data.AutoFree();
        return Super::Unload();
    }

    bool FileMaterialResource::CreateTemplateMaterial(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;

        SR_UTILS_NS::Path&& path = rawPath.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
        path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);

        if (!path.CreateIfNotExists()) {
            SR_ERROR("FileMaterialResource::CreateTemplateMaterial() : failed to create path for the material! \n\tPath: " + path.ToString());
            return false;
        }

        MaterialData::Ptr pData = SRNew<MaterialData>();

        pData->SetShader("Engine/Shaders/SSAO/ssao_geometry.srsl");
        pData->SetShader("Engine/Shaders/CascadedShadowMap/spatial.srsl", "Shadow");

        pData->SetSampler("diffuse", "Engine/Textures/default_improved.png");
        pData->SetData("color", SR_MATH_NS::FVector4(1.f, 1.f, 1.f, 1.f), ShaderVarType::Vec4);

        SR_UTILS_NS::SRASerializer serializer;
        serializer.SetUseTabs(true);

        pData->Save(serializer);

        if (!serializer.SaveToFile(path)) {
            SR_ERROR("FileMaterialResource::CreateTemplateMaterial() : failed to save material to file! \n\tPath: " + path.ToString());
            return false;
        }

        return true;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    BaseMaterial::Ptr FileMaterial::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;

        auto&& pResource = FileMaterialResource::Load(rawPath);
        if (!pResource) {
            SR_ERROR("FileMaterial::Load() : failed to load material resource! \n\tPath: " + rawPath.ToString());
            return nullptr;
        }

        FileMaterial::Ptr pFileMaterial = SRNew<FileMaterial>();
        pFileMaterial->m_pResource = pResource;

        pFileMaterial->Init();

        if (!pFileMaterial->m_data || !pFileMaterial->m_pResource) {
            SR_ERROR("FileMaterial::Load() : failed to load material from file! \n\tPath: " + rawPath.ToString());
            return nullptr;
        }

        return pFileMaterial.StaticCast<BaseMaterial>();
    }

    const MaterialData::Ptr& FileMaterial::GetMaterialData() const noexcept {
        static MaterialData::Ptr pEmptyData;
        return m_pResource ? m_pResource->GetData() : pEmptyData;
    }

    FileMaterial::~FileMaterial() {
        if (m_pResource) {
            m_pResource->RemoveUsePoint();
            m_pResource = nullptr;
        }
    }

    void FileMaterial::SetMaterialPath(const SR_UTILS_NS::Path& path) noexcept {
        FileMaterialResource* pMaterial = nullptr;

        if (!path.IsEmpty()) {
            pMaterial = FileMaterialResource::Load(path);
            if (!pMaterial) {
                SR_ERROR("FileMaterial::SetMaterialPath() : failed to load material resource! \n\tPath: " + path.ToString());
            }
        }

        if (m_pResource == pMaterial) {
            return;
        }

        if (m_pResource) {
            m_pResource->RemoveUsePoint();
            m_pResource = nullptr;
        }

        m_pResource = pMaterial;

        Init();

        OnShaderChanged();
    }

    void FileMaterial::Init() {
        if (m_pResource) {
            m_pResource->AddUsePoint();

            m_reloadBeginSubscription = m_pResource->Subscribe(SR_UTILS_NS::IResource::RELOAD_BEGIN_EVENT,
                [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                    DeInitMaterialDataSubscriptions();
                }
            );

            m_reloadDoneSubscription = m_pResource->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT,
                [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                    InitMaterialDataSubscriptions();
                    OnShaderChanged();
                    OnPropertyChanged(false);
                }
            );
        }
        else {
            DeInitMaterialDataSubscriptions();
            m_reloadBeginSubscription.Reset();
            m_reloadDoneSubscription.Reset();
        }
    }
}
