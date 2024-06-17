//
// Created by Monika on 26.05.2024.
//

#include <Graphics/Material/MeshMaterialProperty.h>
#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Material/UniqueMaterial.h>

namespace SR_GRAPH_NS {
    MeshMaterialProperty::MeshMaterialProperty() {
        static const SR_UTILS_NS::StringAtom name = "Material";
        SetName(name);
    }

    MeshMaterialProperty::~MeshMaterialProperty() {
        SRAssert(m_materialRegisterId == SR_ID_INVALID);
    }

    void MeshMaterialProperty::SaveProperty(MarshalRef marshal) const noexcept {
        if (auto&& pBlock = AllocatePropertyBlock()) {
            const MaterialType type = m_pMaterial ? m_pMaterial->GetMaterialType() : MaterialType::None;
            pBlock->Write<uint8_t>(static_cast<uint8_t>(type));

            switch (type) {
                case MaterialType::None:
                    break; /// nothing to save
                case MaterialType::File: {
                    auto&& pFileMaterial = SR_UTILS_NS::PolymorphicCast<FileMaterial>(m_pMaterial);
                    pBlock->Write<std::string>(pFileMaterial->GetResourcePath().ToStringRef());
                    break;
                }
                case MaterialType::Unique: {
                    auto&& pShader = m_pMaterial->GetShader();
                    pBlock->Write<std::string>(pShader ? pShader->GetResourcePath().ToStringRef() : "");
                    m_pMaterial->GetProperties().SaveProperty(*pBlock);
                    break;
                }
                default:
                    SRHalt("MeshMaterialProperty::SaveProperty() : unknown material type!");
                    return;
            }

            SavePropertyBase(marshal, std::move(pBlock));
        }
    }

    void MeshMaterialProperty::LoadProperty(MarshalRef marshal) noexcept {
        if (auto&& pBlock = LoadPropertyBase(marshal)) {
            const auto type = static_cast<MaterialType>(pBlock->Read<uint8_t>()); /// NOLINT

            switch (type) {
                case MaterialType::None:
                    break; /// nothing to load
                case MaterialType::File: {
                    const auto path = pBlock->Read<std::string>();
                    SetMaterial(FileMaterial::Load(path));
                    break;
                }
                case MaterialType::Unique: {
                    auto&& pMaterial = new UniqueMaterial();
                    auto&& shaderPath = pBlock->Read<std::string>();
                    if (!shaderPath.empty()) {
                        pMaterial->SetShader(SR_GTYPES_NS::Shader::Load(shaderPath));
                    }
                    pMaterial->GetProperties().LoadProperty(*pBlock);
                    SetMaterial(pMaterial);
                    break;
                }
                default:
                    SRHalt("MeshMaterialProperty::LoadProperty() : unknown material type!");
                    return;
            }
        }
    }

    MaterialType MeshMaterialProperty::GetMaterialType() const noexcept {
        return m_pMaterial ? m_pMaterial->GetMaterialType() : MaterialType::None;
    }

    void MeshMaterialProperty::SetMaterial(BaseMaterial* pMaterial) noexcept {
        if (pMaterial == m_pMaterial) {
            return;
        }

        m_pMesh->MarkMaterialDirty();
        m_pMesh->SetErrorsClean();

        SRAssert(m_materialRegisterId == SR_ID_INVALID || m_pMaterial);

        if (m_pMaterial) {
            m_pMaterial->UnregisterMesh(&m_materialRegisterId);
        }

        if ((m_pMaterial = pMaterial)) {
            m_materialRegisterId = m_pMaterial->RegisterMesh(m_pMesh);
        }

        m_pMesh->ReRegisterMesh();
    }

    void MeshMaterialProperty::SetMaterial(const SR_UTILS_NS::Path& path) noexcept {
        if (m_pMaterial && m_pMaterial->GetMaterialType() == MaterialType::File) {
            auto&& pFileMaterial = SR_UTILS_NS::PolymorphicCast<FileMaterial>(m_pMaterial);
            if (pFileMaterial->GetResourcePath() == path) {
                return;
            }
        }

        SetMaterial(path.empty() ? nullptr : FileMaterial::Load(path));
    }
}
