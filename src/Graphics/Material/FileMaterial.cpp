//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/FileMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    FileMaterialResource::FileMaterialResource() = default;

    bool FileMaterialResource::CreateTemplateMaterial(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;

        SR_UTILS_NS::Path&& path = rawPath.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

        if (!path.CreateIfNotExists()) {
            SR_ERROR("FileMaterialResource::CreateTemplateMaterial() : failed to create path for the material! \n\tPath: " + path.ToString());
            return false;
        }

        auto&& pResource = SR_UTILS_NS::Asset::CreateNew<FileMaterialResource>(path);
        if (!pResource) {
            SR_ERROR("FileMaterialResource::CreateTemplateMaterial() : failed to create material resource! \n\tPath: " + path.ToString());
            return false;
        }

        MaterialData::Ptr pData = SRNew<MaterialData>();
        {
            pData->GetDefaultShaderData().SetShader("Engine/Shaders/SSAO/ssao_geometry.srsl");
            pData->SetSampler("diffuse", "Engine/Textures/default_improved.png");
            pData->SetData("color", SR_MATH_NS::FVector4(1.f, 1.f, 1.f, 1.f), ShaderVarType::Vec4);
        }
        pResource->SetData(pData);

        if (!pResource->SaveAsset(path)) {
            SR_ERROR("FileMaterialResource::CreateTemplateMaterial() : failed to save material to file! \n\tPath: " + path.ToString());
            return false;
        }

        return true;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    BaseMaterial::Ptr FileMaterial::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;

        auto&& pResource = SR_UTILS_NS::Asset::Load<FileMaterialResource>(rawPath);
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

    BaseMaterial::Ptr FileMaterial::LoadAsUnique(const SR_UTILS_NS::Path& rawPath) {
        auto&& pFileMaterial = Load(rawPath);
        if (!pFileMaterial) {
            SR_ERROR("FileMaterial::LoadAsUnique() : failed to load material from file! \n\tPath: " + rawPath.ToString());
            return nullptr;
        }

        UniqueMaterial::Ptr pUniqueMaterial = SRNew<UniqueMaterial>();

        pFileMaterial->GetMaterialData()->GetDefaultShaderData().CloneTo(
            pUniqueMaterial->GetMaterialData()->GetDefaultShaderData()
        );

        return pUniqueMaterial.StaticCast<BaseMaterial>();
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
        FileMaterialResource::Ptr pMaterial = nullptr;

        if (!path.IsEmpty()) {
            pMaterial = SR_UTILS_NS::Asset::Load<FileMaterialResource>(path);
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

            InitMaterialDataSubscriptions();

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
