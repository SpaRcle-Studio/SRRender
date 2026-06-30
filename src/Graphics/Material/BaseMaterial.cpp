//
// Created by Monika on 19.05.2024.
//

#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Mesh.h>

#include <Utils/Common/SubscriptionMessage.h>

#include <Codegen/BaseMaterial.generated.hpp>

namespace SR_GRAPH_NS {
    BaseMaterial::BaseMaterial()
        : SR_HTYPES_NS::SharedPtr<BaseMaterial>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    BaseMaterial::~BaseMaterial() {
        SRAssert2(m_registerObjects.IsEmpty(), "Material is not unregistered from all render objects! Application may will crash!");

        for (auto&& pVariant : m_variants | std::views::values) {
            pVariant->RemoveUsePoint();
        }
        m_variants.clear();
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

    void BaseMaterial::SetFloat(const SR_UTILS_NS::StringAtom id, float_t v) noexcept {
        if (auto&& pData = GetMaterialData()) {
            InitContext();
            pData->SetData(id, v, ShaderVarType::Float);
        }
    }

    void BaseMaterial::SetTexture(const SR_UTILS_NS::StringAtom id, const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Texture>& pTexture) noexcept {
        if (auto&& pData = GetMaterialData()) {
            InitContext();
            pData->SetData(id, const_cast<SR_GTYPES_NS::Texture*>(pTexture.Get()), ShaderVarType::Sampler2D);
        }
    }

    void BaseMaterial::Use(SR_GTYPES_NS::Shader& shader) {
        if (auto&& pData = GetMaterialData()) {
            pData->UseUniforms(shader);
        }
    }

    uint32_t BaseMaterial::Register(SR_GTYPES_NS::IRenderComponent* pObject) {
        SRAssert(pObject);
        return m_registerObjects.Add(pObject);
    }

    void BaseMaterial::Unregister(uint32_t* pId) {
        m_registerObjects.RemoveByIndex(*pId);
        *pId = SR_ID_INVALID;
    }

    void BaseMaterial::OnPropertyChanged(bool onlyUniforms) const {
        SR_TRACY_ZONE;

        if (onlyUniforms) {
            m_registerObjects.ForEach([](uint32_t, auto&& pObject) {
                pObject->MarkUniformsDirty();
            });
        }
        else {
            m_registerObjects.ForEach([](uint32_t, auto&& pObject) {
                pObject->MarkMaterialDirty();
            });
            GetContext().Do([](RenderContext* ptr) {
                ptr->SetDirty();
            });
        }
    }

    void BaseMaterial::OnShaderChanged() {
        SR_TRACY_ZONE;

        for (auto&& pVariant : m_variants | std::views::values) {
            pVariant->RemoveUsePoint();
        }
        m_variants.clear();

        m_hashRedirect.clear();

        m_registerObjects.ForEach([](uint32_t, auto&& pObject) {
            pObject->ReRegisterRenderObject();
        });
    }

    void BaseMaterial::SetShader(ShaderPtr pShader) {
        SR_TRACY_ZONE;

        if (auto&& pData = GetMaterialData()) {
            pData->GetDefaultShaderData().SetShader(std::move(pShader));
        }
    }

    void BaseMaterial::SetShader(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        if (auto&& pData = GetMaterialData()) {
            auto&& pShader = pData->GetDefaultShaderData().pShader;
            if (pShader && pShader->GetResourcePath() == path) {
                return;
            }
        }

        SR_SRSL_NS::ShaderParams params;
        params.GetVertexLayoutDescriptions().AddLayout().AddAttribute(
            SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3
        );

        auto&& pShader = CoreResLoader::Load<SR_GTYPES_NS::Shader>(path, &params);
        if (!pShader) {
            SR_ERROR("BaseMaterial::SetShader() : shader is nullptr!");
            return;
        }
        SetShader(pShader);
    }

    void BaseMaterial::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;

        if (auto&& pData = GetMaterialData()) {
            pData->UseSamplers(shader);
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

    void BaseMaterial::InitMaterialDataSubscriptions() {
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

    void BaseMaterial::DeInitMaterialDataSubscriptions() {
        m_shaderChangedSubscription.Reset();
        m_propertyChangedSubscription.Reset();
    }

    bool BaseMaterial::IsValid() const {
        auto&& pData = GetMaterialData();
        if (!pData) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }
        return pData->GetDefaultShaderData().pShader;
    }

    SR_GTYPES_NS::Shader* BaseMaterial::GetDefaultShader() const noexcept {
        auto&& pData = GetMaterialData();
        if (!pData) SR_UNLIKELY_ATTRIBUTE {
            return nullptr;
        }
        return pData->GetDefaultShaderData().pShader.Get();
    }

    SR_GTYPES_NS::Shader* BaseMaterial::GetShader(const SR_SRSL_NS::ShaderParams& params) const noexcept {
        SR_TRACY_ZONE;

        auto&& pData = GetMaterialData();
        if (!pData) SR_UNLIKELY_ATTRIBUTE {
            return nullptr;
        }

        auto&& pDefaultShader = pData->GetDefaultShaderData().pShader.Get();
        if (!pDefaultShader) {
            return nullptr;
        }

        const SR_SRSL_NS::ShaderParams* pMacros = &params;

        if (!pData->GetShaderDefines().empty()) {
            const auto baseHash = params.GetHash();
            auto&& pBaseIt = m_hashRedirect.find(baseHash);

            if (pBaseIt == m_hashRedirect.end()) {
                SR_SRSL_NS::ShaderParams clone = params;
                for (auto&& [key, value] : pData->GetShaderDefines()) {
                    clone.SetParam(key, value);
                }
                m_hashRedirect.insert({ baseHash, clone });
                pMacros = &m_hashRedirect.at(baseHash);
            }
            else {
                pMacros = &pBaseIt->second;
            }
        }

        const auto hash = pMacros->GetHash();
        if (hash == pDefaultShader->GetMacros().GetHash()) {
            return pDefaultShader;
        }

        auto&& pIt = m_variants.find(hash);
        if (pIt != m_variants.end()) {
            return pIt->second.Get();
        }

        auto&& pShader = CoreResLoader::Load<SR_GTYPES_NS::Shader>(pDefaultShader->GetResourcePath(), pMacros);
        if (pShader) {
            m_variants[hash] = pShader;
            pShader->AddUsePoint();
        }

        return pShader.Get();
    }

    const MaterialData::Ptr& BaseMaterial::GetMaterialData() const noexcept {
        static MaterialData::Ptr empty;
        return empty;
    }
}
