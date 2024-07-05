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

    void BaseMaterial::SetVec4(SR_UTILS_NS::StringAtom id, const SR_MATH_NS::FVector4& v) noexcept {
        for (auto&& pProperty : m_properties.GetMaterialUniformsProperties()) {
            if (pProperty->GetName() == id && pProperty->GetShaderVarType() == ShaderVarType::Vec4) {
                pProperty->SetData(v);
                return;
            }
        }
    }


    void BaseMaterial::SetBool(SR_UTILS_NS::StringAtom id, bool v) noexcept {
        for (auto&& pProperty : m_properties.GetMaterialUniformsProperties()) {
            if (pProperty->GetName() == id && pProperty->GetShaderVarType() == ShaderVarType::Bool) {
                pProperty->SetData(v);
                return;
            }
        }
    }

    void BaseMaterial::SetTexture(SR_UTILS_NS::StringAtom id, SR_GTYPES_NS::Texture* pTexture) noexcept {
        for (auto&& pProperty : m_properties.GetMaterialSamplerProperties()) {
            if (pProperty->GetName() == id && pProperty->GetShaderVarType() == ShaderVarType::Sampler2D) {
                pProperty->SetData(pTexture);
                return;
            }
        }
    }

    void BaseMaterial::Use() {
        SR_TRACY_ZONE;
        InitContext();
        m_properties.UseMaterialUniforms(GetContext()->GetPipeline()->GetCurrentShader());
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

    void BaseMaterial::OnPropertyChanged(bool onlyUniforms) {
        SR_TRACY_ZONE;

        if (onlyUniforms) {
            m_meshes.ForEach([](uint32_t, auto&& pMesh) {
                pMesh->MarkUniformsDirty();
            });
        }
        else {
            m_meshes.ForEach([](uint32_t, auto&& pMesh) {
                pMesh->MarkMaterialDirty();
            });
            GetContext().Do([](RenderContext* ptr) {
                ptr->SetDirty();
            });
        }
    }

    void BaseMaterial::SetShader(ShaderPtr pShader) {
        if (m_shader == pShader) {
            return;
        }

        m_dirtyShader = true;

        if (m_shader) {
            m_shader->RemoveUsePoint();
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
            OnPropertyChanged(false);
        });

        m_shader->AddUsePoint();
        InitMaterialProperties();
    }

    void BaseMaterial::SetShader(const SR_UTILS_NS::Path& path) {
        auto&& pShader = SR_GTYPES_NS::Shader::Load(path);
        if (!pShader) {
            SR_ERROR("BaseMaterial::SetShader() : shader is nullptr!");
            return;
        }
        SetShader(pShader);
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
