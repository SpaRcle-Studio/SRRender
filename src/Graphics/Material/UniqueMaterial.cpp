//
// Created by Monika on 26.05.2024.
//

#include <Graphics/Material/UniqueMaterial.h>

#include <Codegen/UniqueMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    UniqueMaterial::UniqueMaterial() = default;

    UniqueMaterial::~UniqueMaterial() {
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
}
