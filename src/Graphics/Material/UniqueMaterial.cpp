//
// Created by Monika on 26.05.2024.
//

#include <Graphics/Material/UniqueMaterial.h>
#include <Graphics/Material/FileMaterial.h>

#include <Codegen/UniqueMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    UniqueMaterial::UniqueMaterial() = default;

    UniqueMaterial::~UniqueMaterial() {
        SR_TRACY_ZONE;
        DeInitMaterialDataSubscriptions();
        m_data.Reset();
    }

    void UniqueMaterial::SetMaterialData(const MaterialData::Ptr& pData) noexcept {
        DeInitMaterialDataSubscriptions();
        m_data.Reset();
        m_data = pData;
        InitMaterialDataSubscriptions();
    }

    const MaterialData::Ptr& UniqueMaterial::GetMaterialData() const noexcept {
        if (!m_data) {
            m_data = SRNew<MaterialData>();
            const_cast<UniqueMaterial&>(*this).InitMaterialDataSubscriptions();
        }

        return m_data;
    }

    MaterialType UniqueMaterial::GetMaterialType() const noexcept {
        return MaterialType::Unique;
    }

    void UniqueMaterial::SaveAs(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        auto&& pMaterial = SR_UTILS_NS::Asset::LoadOrCreate<FileMaterialResource>(path);
        MaterialData::Ptr pCopyData = SRNew<MaterialData>();
        m_data->CloneTo(*pCopyData);
        pMaterial->SetData(pCopyData);
        if (!pMaterial->SaveAsset(path)) {
            SR_ERROR("UniqueMaterial::SaveAs() : failed to save material to file!\n\tPath: {}", path);
        }
    }
}
