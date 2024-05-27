//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Render/RenderContext.h>

namespace SR_GRAPH_NS {
    BaseMaterial::BaseMaterial() = default;

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
        SRAssert(pMesh);
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
            m_shaderReloadDoneSubscription.Reset();
        }

        m_meshes.ForEach([](uint32_t, auto&& pMesh) {
            pMesh->ReRegisterMesh();
        });

        if (!((m_shader = pShader))) {
            return;
        }

        m_shaderReloadDoneSubscription = m_shader->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this]() {
            m_dirtyShader = true;
            m_meshes.ForEach([](uint32_t, auto&& pMesh) {
                pMesh->MarkMaterialDirty();
            });
        });

        AddMaterialDependency(m_shader);
        InitMaterialProperties();
    }

    void BaseMaterial::SetTexture(MaterialProperty* pProperty, SR_GTYPES_NS::Texture::Ptr pTexture) {
        if (!SRVerifyFalse(!pProperty)) {
            return;
        }

        if (auto&& oldTexture = std::get<SR_GTYPES_NS::Texture*>(pProperty->GetData())) {
            if (oldTexture == pTexture) {
                return;
            }

            RemoveMaterialDependency(oldTexture);
        }

        if (pTexture) {
            SRAssert(!(pTexture->GetCountUses() == 0 && pTexture->IsCalculated()));
            AddMaterialDependency(pTexture);
        }

        pProperty->SetData(pTexture);

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

        m_properties.ClearContainer();
    }

    void BaseMaterial::InitMaterialProperties() {
        if (!m_shader) {
            SR_ERROR("BaseMaterial::LoadProperties() : shader is nullptr!");
            return;
        }

        m_properties.ClearContainer();

        /// Загружаем базовые значения
        for (auto&& property : m_shader->GetProperties()) {
            m_properties.AddCustomProperty<MaterialProperty>(property.id, property.type)
                .SetData(property.GetData())
                .SetMaterial(this)
                .SetDisplayName(property.id); // TODO: make a pretty name
        }
    }
}
