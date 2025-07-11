//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/BaseMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    BaseMaterial::BaseMaterial()
        : SR_HTYPES_NS::SharedPtr<BaseMaterial>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    BaseMaterial::~BaseMaterial() {
        SRAssert2(m_meshes.IsEmpty(), "Material is not unregistered from all meshes!");
    }

    void BaseMaterial::SetVec4(const SR_UTILS_NS::StringAtom id, const SR_MATH_NS::FVector4& v) noexcept {
        if (auto&& pData = GetMaterialData()) {
            InitContext();
            pData->SetData(id, v, ShaderVarType::Vec4);
        }
    }

    void BaseMaterial::SetColor(const SR_UTILS_NS::StringAtom id, const SR_MATH_NS::FColor& v) noexcept {
        if (auto&& pData = GetMaterialData()) {
            InitContext();
            pData->SetData(id, SR_MATH_NS::FVector4(v.r, v.g, v.b, v.a), ShaderVarType::Vec4);
        }
    }

    void BaseMaterial::SetBool(const SR_UTILS_NS::StringAtom id, bool v) noexcept {
        if (auto&& pData = GetMaterialData()) {
            InitContext();
            pData->SetData(id, v, ShaderVarType::Bool);
        }
    }

    void BaseMaterial::SetTexture(const SR_UTILS_NS::StringAtom id, const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Texture>& pTexture) noexcept {
        if (auto&& pData = GetMaterialData()) {
            InitContext();
            pData->SetData(id, const_cast<SR_GTYPES_NS::Texture*>(pTexture.Get()), ShaderVarType::Sampler2D);
        }
    }

    SR_UTILS_NS::StringAtom BaseMaterial::GetRenderStageId() const noexcept {
        InitContext();
        return m_context->GetPipeline()->GetRenderStageId();
    }

    void BaseMaterial::Use() {
        SR_TRACY_ZONE;

        InitContext();

        if (auto&& pData = GetMaterialData()) {
            pData->UseUniforms(m_context->GetPipeline().Get());
        }
    }

    bool BaseMaterial::IsTransparent() const {
        if (auto&& pShader = GetShader()) {
            return pShader->IsBlendEnabled();
        }
        SRHalt("BaseMaterial::IsTransparent() : shader is nullptr!");
        return false;
    }

    BaseMaterial::ShaderPtr BaseMaterial::GetShader() const {
        InitContext();
        if (auto&& pData = GetMaterialData()) {
            return pData->GetShader(m_context->GetPipeline().Get());
        }
        return nullptr;
    }

    uint32_t BaseMaterial::RegisterMesh(MeshPtr pMesh) {
        SRAssert(pMesh);
        return m_meshes.Add(pMesh);
    }

    void BaseMaterial::UnregisterMesh(uint32_t* pId) {
        m_meshes.RemoveByIndex(*pId);
        *pId = SR_ID_INVALID;
    }

    void BaseMaterial::OnPropertyChanged(bool onlyUniforms) const {
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

    void BaseMaterial::OnShaderChanged() const {
        SR_TRACY_ZONE;

        m_meshes.ForEach([](uint32_t, auto&& pMesh) {
            pMesh->ReRegisterMesh();
        });
    }

    void BaseMaterial::SetShader(ShaderPtr pShader, SR_UTILS_NS::StringAtom stage) {
        SR_TRACY_ZONE;

        if (auto&& pData = GetMaterialData()) {
            pData->SetShader(pShader, stage);
        }
    }

    void BaseMaterial::SetShader(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom stage) {
        SR_TRACY_ZONE;

        auto&& pShader = SR_GTYPES_NS::Shader::Load(path);
        if (!pShader) {
            SR_ERROR("BaseMaterial::SetShader() : shader is nullptr!");
            return;
        }
        SetShader(pShader, stage);
    }

    void BaseMaterial::UseSamplers() {
        SR_TRACY_ZONE;

        InitContext();

        if (auto&& pData = GetMaterialData()) {
            pData->UseSamplers(m_context->GetPipeline().Get());
        }
    }

    void BaseMaterial::InitContext() const {
        if (!m_context) SR_UNLIKELY_ATTRIBUTE {
            if (!((m_context = SR_THIS_THREAD->GetContext()->GetValue<RenderContextPtr>()))) {
                SRHalt("Is not in render context!");
                return;
            }
        }
    }

    void BaseMaterial::InitMaterialDataSubscriptions() const {
        m_shaderChangedSubscription = GetMaterialData()->Subscribe(MaterialData::SHADER_CHANGED_EVENT,
            [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                OnShaderChanged();
            }
        );

        m_propertyChangedSubscription = GetMaterialData()->Subscribe(MaterialData::PROPERTY_CHANGED_EVENT,
            [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                OnPropertyChanged(msg.GetBool(MaterialData::ONLY_UNIFORMS_BOOL_ID));
            }
        );
    }

    void BaseMaterial::DeInitMaterialDataSubscriptions() const {
        m_shaderChangedSubscription.Reset();
        m_propertyChangedSubscription.Reset();
    }
}
