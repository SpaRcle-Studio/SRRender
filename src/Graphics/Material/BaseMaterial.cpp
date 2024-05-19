//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Render/RenderContext.h>

namespace SR_GRAPH_NS {
    BaseMaterial::~BaseMaterial() {
        SRAssert2(m_isFinalized, "Material is not finalized!");
        SRAssert2(m_meshes.IsEmpty(), "Material is not unregistered from all meshes!");
    }

    void BaseMaterial::Use() {
        SR_TRACY_ZONE;
        InitContext();
        m_properties.UseMaterialUniforms(m_shader);
    }

    bool BaseMaterial::IsTransparent() const {
        if (!m_shader) {
            SRHalt("Shader is nullptr!");
            return false;
        }

        return m_shader->IsBlendEnabled();
    }

    bool BaseMaterial::ContainsTexture(SR_GTYPES_NS::Texture* pTexture) const {
        if (!pTexture) {
            return false;
        }

        return !m_properties.ForEachPropertyRet<MaterialProperty>([pTexture](auto&& pProperty) -> bool {
            if (std::visit([pTexture](ShaderPropertyVariant&& arg) -> bool {
                if (std::holds_alternative<SR_GTYPES_NS::Texture*>(arg)) {
                    return std::get<SR_GTYPES_NS::Texture*>(arg) == pTexture;
                }
                return false;
            }, pProperty->GetData())) {
                return false;
            }
            return true;
        });
    }

    uint32_t BaseMaterial::RegisterMesh(MeshPtr pMesh) {
        return m_meshes.Add(pMesh);
    }

    void BaseMaterial::UnregisterMesh(uint32_t* pId) {
        m_meshes.RemoveByIndex(*pId);
        *pId = SR_ID_INVALID;
    }

    void BaseMaterial::OnPropertyChanged() {
        SR_TRACY_ZONE;

        m_meshes.ForEach([](uint32_t, auto&& pMesh) {
            pMesh->MarkUniformsDirty();
        });
    }

    void BaseMaterial::SetShader(ShaderPtr pShader) {
        if (m_shader == pShader) {
            return;
        }

        m_dirtyShader = true;

        if (m_shader) {
            RemoveMaterialDependency(m_shader);
            m_shader = nullptr;
        }

        m_meshes.ForEach([](uint32_t, auto&& pMesh) {
            pMesh->MarkMaterialDirty();
        });

        if (!(m_shader = pShader)) {
            return;
        }

        AddMaterialDependency(m_shader);
    }

    void BaseMaterial::SetTexture(MaterialProperty* property, SR_GTYPES_NS::Texture::Ptr pTexture) {
        if (!SRVerifyFalse(!property)) {
            return;
        }

        if (auto&& oldTexture = std::get<SR_GTYPES_NS::Texture*>(property->GetData())) {
            if (oldTexture == pTexture) {
                return;
            }

            RemoveMaterialDependency(oldTexture);
        }

        if (pTexture) {
            SRAssert(!(pTexture->GetCountUses() == 0 && pTexture->IsCalculated()));
            AddMaterialDependency(pTexture);
        }

        property->SetData(pTexture);

        m_meshes.ForEach([](uint32_t, auto&& pMesh) {
            pMesh->MarkMaterialDirty();
        });

        /// сработает только если хоть раз была отрендерина текстура материала
        m_context.Do([](RenderContext* ptr) {
            ptr->SetDirty();
        });
    }

    void BaseMaterial::UseSamplers() {
        InitContext();

        if (m_shader && m_context->GetCurrentShader() != m_shader) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        m_properties.UseMaterialSamplers(m_shader);
    }

    MaterialProperty* BaseMaterial::GetProperty(uint64_t hashId) {
        return m_properties.Find<MaterialProperty>(hashId);
    }

    MaterialProperty* BaseMaterial::GetProperty(const SR_UTILS_NS::StringAtom& id) {
        return GetProperty(id.GetHash());
    }

    bool BaseMaterial::LoadProperties(const SR_XML_NS::Node& propertiesNode) {
        if (!m_shader) {
            SR_ERROR("Material::LoadProperties() : shader is nullptr!");
            return false;
        }

        m_properties.ClearContainer();

        /// Загружаем базовые значения
        for (auto&& [id, propertyType] : m_shader->GetProperties()) {
            m_properties.AddCustomProperty<MaterialProperty>(id.c_str(), propertyType)
                .SetData(GetVariantFromShaderVarType(propertyType))
                .SetMaterial(this)
                .SetDisplayName(id); // TODO: make a pretty name
        }

        /// Применяем сохраненные в материале значения
        if (auto&& pFileMaterial = dynamic_cast<FileMaterial*>(this)) {
            LoadMaterialProperties(pFileMaterial->GetResourcePath().ToStringRef(), propertiesNode, &m_properties);
        }
        else {
            const static std::string identifier = "From memory";
            LoadMaterialProperties(identifier, propertiesNode, &m_properties);
        }

        /// Добавляем все текстуры в зависимости
        for (auto&& pTexture : GetTexturesFromMatProperties(m_properties)) {
            SRAssert(!(pTexture->GetCountUses() == 0 && pTexture->IsCalculated()));
            AddMaterialDependency(pTexture);
        }

        return true;
    }

    void BaseMaterial::InitContext() {
        if (!m_context) SR_UNLIKELY_ATTRIBUTE {
            if (!(m_context = SR_THIS_THREAD->GetContext()->GetValue<RenderContextPtr>())) {
                SRHalt("Is not render context!");
                return;
            }
        }
    }

    void BaseMaterial::AddMaterialDependency(SR_UTILS_NS::IResource* pResource) {
        pResource->AddUsePoint();
    }

    void BaseMaterial::RemoveMaterialDependency(SR_UTILS_NS::IResource* pResource) {
        pResource->RemoveUsePoint();
    }

    void BaseMaterial::FinalizeMaterial() {
        SRAssert2(!m_isFinalized, "Material is already finalized!");
        m_isFinalized = true;

        SetShader(nullptr);

        for (auto&& pTexture : GetTexturesFromMatProperties(m_properties)) {
            RemoveMaterialDependency(pTexture);
        }

        m_properties.ClearContainer();
    }
}