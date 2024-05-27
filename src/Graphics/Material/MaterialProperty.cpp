//
// Created by Monika on 27.05.2024.
//

#include <Graphics/Material/MaterialProperty.h>

namespace SR_GRAPH_NS {
    void LoadMaterialProperties(const std::string& materialDebugIdentifier, const SR_XML_NS::Node& propertiesNode, MaterialProperties* pProperties) {
        SR_TRACY_ZONE;

        for (auto&& propertyXml : propertiesNode.TryGetNodes()) {
            auto&& type = SR_UTILS_NS::EnumReflector::FromString<ShaderVarType>(propertyXml.GetAttribute("Type").ToString());
            auto&& id = propertyXml.GetAttribute("Id").ToString();

            auto&& pMaterialProperty = pProperties->Find<MaterialProperty>(id);
            if (!pMaterialProperty) {
                continue;
            }

            if (pMaterialProperty->GetShaderVarType() != type) {
                SR_ERROR("LoadMaterialProperties() : invalid property!\n\tMaterial: " + materialDebugIdentifier +
                     "\n\tProperty: " + id + "\n\tLoaded type: " + SR_UTILS_NS::EnumReflector::ToStringAtom(type).ToStringRef() +
                     "\n\tExpected type: " + SR_UTILS_NS::EnumReflector::ToStringAtom(pMaterialProperty->GetShaderVarType()).ToStringRef()
                );
                continue;
            }

            switch (type) {
                case ShaderVarType::Int:
                    pMaterialProperty->SetData(propertyXml.GetAttribute<int32_t>());
                    break;
                case ShaderVarType::Bool:
                    pMaterialProperty->SetData(propertyXml.GetAttribute<bool>());
                    break;
                case ShaderVarType::Float:
                    pMaterialProperty->SetData(propertyXml.GetAttribute<float_t>());
                    break;
                case ShaderVarType::Vec2:
                    pMaterialProperty->SetData(propertyXml.GetAttribute<SR_MATH_NS::FVector2>());
                    break;
                case ShaderVarType::Vec3:
                    pMaterialProperty->SetData(propertyXml.GetAttribute<SR_MATH_NS::FVector3>());
                    break;
                case ShaderVarType::Vec4:
                    pMaterialProperty->SetData(propertyXml.GetAttribute<SR_MATH_NS::FVector4>());
                    break;
                case ShaderVarType::Sampler2D: {
                    auto&& pTexture = SR_GTYPES_NS::Texture::Load(propertyXml.GetAttribute<std::string>());
                    pMaterialProperty->GetMaterial()->SetTexture(pMaterialProperty, pTexture);
                    break;
                }
                default:
                    SRHalt("Unknown property!");
                    break;
            }
        }
    }

    std::list<SR_GTYPES_NS::Texture*> GetTexturesFromMatProperties(const MaterialProperties& properties) {
        std::list<SR_GTYPES_NS::Texture*> textures;

        properties.ForEachProperty<MaterialProperty>([&textures](auto&& pProperty){
            std::visit([&textures](ShaderPropertyVariant&& arg) {
                if (std::holds_alternative<SR_GTYPES_NS::Texture*>(arg)) {
                    if (auto&& value = std::get<SR_GTYPES_NS::Texture*>(arg)) {
                        textures.emplace_back(value);
                    }
                }
            }, pProperty->GetData());
        });

        return textures;
    }

    void MaterialProperty::Use(SR_GTYPES_NS::Shader* pShader) const noexcept {
        SR_TRACY_ZONE;

        auto&& hashId = GetName().GetHash();

        switch (GetShaderVarType()) {
            case ShaderVarType::Int:
            case ShaderVarType::Bool:
                pShader->SetInt(hashId, std::get<int32_t>(GetData()));
                break;
            case ShaderVarType::Float:
                pShader->SetFloat(hashId, std::get<float_t>(GetData()));
                break;
            case ShaderVarType::Vec2:
                pShader->SetVec2(hashId, std::get<SR_MATH_NS::FVector2>(GetData()).template Cast<float_t>());
                break;
            case ShaderVarType::Vec3:
                pShader->SetVec3(hashId, std::get<SR_MATH_NS::FVector3>(GetData()).template Cast<float_t>());
                break;
            case ShaderVarType::Vec4:
                pShader->SetVec4(hashId, std::get<SR_MATH_NS::FVector4>(GetData()).template Cast<float_t>());
                break;
            case ShaderVarType::Sampler2D:
                pShader->SetSampler2D(GetName(), std::get<SR_GTYPES_NS::Texture*>(GetData()));
                break;
            default:
                SRAssertOnce(false);
                break;
        }
    }

    MaterialProperty::~MaterialProperty() {
        if (GetShaderVarType() == ShaderVarType::Sampler2D) {
            if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture*>(GetData())) {
                GetMaterial()->RemoveMaterialDependency(pTexture);
            }
        }
    }

    bool MaterialProperty::IsSampler() const noexcept {
        return m_type >= ShaderVarType::Sampler2D && m_type <= ShaderVarType::Sampler2DShadow;
    }

    void MaterialProperty::OnPropertyChanged() {
        if (m_material) {
            m_material->OnPropertyChanged();
        }
    }

    void MaterialProperty::SaveProperty(MarshalRef marshal) const noexcept {
        if (auto&& pBlock = AllocatePropertyBlock()) {
            pBlock->Write<uint8_t>(static_cast<uint8_t>(GetShaderVarType()));

            switch (GetShaderVarType()) {
                case ShaderVarType::Int:
                case ShaderVarType::Bool:
                    pBlock->Write<int32_t>(std::get<int32_t>(GetData()));
                    break;
                case ShaderVarType::Float:
                    pBlock->Write<float_t>(std::get<float_t>(GetData()));
                    break;
                case ShaderVarType::Vec2:
                    pBlock->Write<SR_MATH_NS::FVector2>(std::get<SR_MATH_NS::FVector2>(GetData()));
                    break;
                case ShaderVarType::Vec3:
                    pBlock->Write<SR_MATH_NS::FVector3>(std::get<SR_MATH_NS::FVector3>(GetData()));
                    break;
                case ShaderVarType::Vec4:
                    pBlock->Write<SR_MATH_NS::FVector4>(std::get<SR_MATH_NS::FVector4>(GetData()));
                    break;
                case ShaderVarType::Sampler2D: {
                    auto&& pTexture = std::get<SR_GTYPES_NS::Texture*>(GetData());
                    std::string path = pTexture ? pTexture->GetResourcePath().ToString() : "";
                    pBlock->Write<std::string>(path);
                    break;
                }
                default:
                    return;
            }

            SavePropertyBase(marshal, std::move(pBlock));
        }
    }

    void MaterialProperty::LoadProperty(MarshalRef marshal) noexcept {
        if (auto&& pBlock = LoadPropertyBase(marshal)) {
            const auto type = static_cast<ShaderVarType>(pBlock->Read<uint8_t>());
            SetShaderVarType(type);

            switch (type) {
                case ShaderVarType::Int:
                case ShaderVarType::Bool:
                    SetData(pBlock->Read<int32_t>());
                    break;
                case ShaderVarType::Float:
                    SetData(pBlock->Read<float_t>());
                    break;
                case ShaderVarType::Vec2:
                    SetData(pBlock->Read<SR_MATH_NS::FVector2>());
                    break;
                case ShaderVarType::Vec3:
                    SetData(pBlock->Read<SR_MATH_NS::FVector3>());
                    break;
                case ShaderVarType::Vec4:
                    SetData(pBlock->Read<SR_MATH_NS::FVector4>());
                    break;
                case ShaderVarType::Sampler2D: {
                    auto&& path = pBlock->Read<std::string>();
                    if (!path.empty()) {
                        auto&& pTexture = SR_GTYPES_NS::Texture::Load(path);
                        GetMaterial()->SetTexture(this, pTexture);
                    }
                    break;
                }
                default:
                    SRHalt("Unknown property type!");
                    return;
            }
        }
    }
}
