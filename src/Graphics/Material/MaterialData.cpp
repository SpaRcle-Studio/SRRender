//
// Created by Monika on 08.02.2025.
//

#include <Graphics/Material/MaterialData.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Common/SubscriptionMessage.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#include <Enum/ShaderVarType.hpp>

#include <Codegen/MaterialData.generated.hpp>

namespace SR_GRAPH_NS {
    void MaterialShaderProperty::Save(SR_UTILS_NS::ISerializer& serializer) const {
        Super::Save(serializer);

        if (!data) {
            return;
        }

        if (IsSamplerType(type)) {
            if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(*data)) {
                SR_UTILS_NS::Serialization::Save(serializer, pTexture->GetResourcePath(), SR_UTILS_NS::SerializationId::Create("value"));
            }
            return;
        }

        switch (type) {
            case ShaderVarType::Int:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<int32_t>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Bool:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<int32_t>(*data) != 0, SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Float:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<float_t>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec2:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::FVector2>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec3:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::FVector3>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::IVec3:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::IVector3>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec4:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::FVector4>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            default:
                SRHalt("MaterialShaderProperty::Save() : unknown property type! Property id: {}, Type: {}", id, type);
                break;
        }
    }

    bool MaterialShaderProperty::Load(SR_UTILS_NS::IDeserializer& deserializer) {
        if (!Super::Load(deserializer)) {
            return false;
        }

        if (IsSamplerType(type)) {
            SR_UTILS_NS::Path path;
            SR_UTILS_NS::Serialization::Load(deserializer, path, SR_UTILS_NS::SerializationId::Create("value"));

            SR_GTYPES_NS::Texture::Ptr pTexture = path.empty() ? nullptr : CoreResLoader::Load<SR_GTYPES_NS::Texture>(path);

            if (data) {
                if (auto&& pOldTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&(*data))) {
                    if (*pOldTextureRef) {
                        (*pOldTextureRef)->RemoveUsePoint();
                    }
                }
            }

            data = pTexture;

            if (pTexture) {
                pTexture->AddUsePoint();
            }

            return true;
        }

        data = GetVariantFromShaderVarType(type);

        switch (type) {
            case ShaderVarType::Int:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<int32_t>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Bool: {
                bool boolean = false;
                SR_UTILS_NS::Serialization::Load(deserializer, boolean, SR_UTILS_NS::SerializationId::Create("value"));
                std::get<int32_t>(*data) = boolean ? 1 : 0;
                break;
            }
            case ShaderVarType::Float:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<float_t>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec2:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::FVector2>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec3:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::FVector3>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::IVec3:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::IVector3>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec4:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::FVector4>(*data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            default:
                SRHalt("MaterialShaderProperty::Load() : unknown property type! Property id: {}, Type: {}", id, type);
                break;
        }

        return true;
    }

    bool MaterialShaderProperty::operator==(const MaterialShaderProperty &other) const {
        return id == other.id && type == other.type && pushConstant == other.pushConstant && data == other.data;
    }

    MaterialShaderData::~MaterialShaderData() {
        SR_SAFE_DELETE_PTR(m_shaderSubscription);

        for (auto&& subscription : m_textureSubscriptions | std::views::values) {
            SR_SAFE_DELETE_PTR(subscription.first);
        }
        m_textureSubscriptions.clear();

        if (pShader) {
            pShader->RemoveUsePoint();
            pShader = nullptr;
        }

        for (MaterialShaderProperty& sampler : samplers) {
            if (sampler.data) {
                if (auto&& pTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&(*sampler.data))) {
                    if (*pTextureRef) {
                        (*pTextureRef)->RemoveUsePoint();
                    }
                }
            }
        }
        samplers.clear();
    }

    void MaterialShaderData::OnPreLoad() {
        Super::OnPreLoad();
    }

    void MaterialShaderData::OnPostLoad() {
        SR_TRACY_ZONE;

        if (!pOwnedMaterialData) {
            SRHalt("MaterialShaderData::OnPostLoad() : pOwnedMaterialData is null!");
            return;
        }

        for (MaterialShaderProperty& sampler : samplers) {
            if (sampler.data) {
                if (auto&& pTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&(*sampler.data))) {
                    if (*pTextureRef) {
                        OnSamplerChanged(nullptr, *pTextureRef);
                    }
                }
            }
        }

        Init();

        Super::OnPostLoad();
    }

    void MaterialShaderData::ForEachProperty(const SR_HTYPES_NS::Function<void(MaterialShaderProperty&)>& func) {
        for (MaterialShaderProperty& uniform : uniforms) {
            func(uniform);
        }
        for (MaterialShaderProperty& sampler : samplers) {
            func(sampler);
        }
    }

    void MaterialShaderData::ForEachProperty(const SR_HTYPES_NS::Function<void(const MaterialShaderProperty&)>& func) const {
        for (const MaterialShaderProperty& uniform : uniforms) {
            func(uniform);
        }
        for (const MaterialShaderProperty& sampler : samplers) {
            func(sampler);
        }
    }

    void MaterialShaderData::SetShader(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        if (path.empty()) {
            SetShader(SR_GTYPES_NS::Shader::Ptr());
            return;
        }

        SR_SRSL_NS::ShaderMacrosParams macros;
        for (auto&& [key, value] : pOwnedMaterialData->GetShaderDefines()) {
            macros.SetParam(key, value);
        }

        if (auto&& pNewShader = CoreResLoader::Load<SR_GTYPES_NS::Shader>(path, &macros)) {
            SetShader(pNewShader);
        }
        else {
            SR_ERROR("MaterialShaderData::SetShader() : failed to load shader! \n\tPath: " + path.ToString());
        }
    }

    void MaterialShaderData::SetShader(SR_GTYPES_NS::Shader::Ptr pNewShader) {
        SR_TRACY_ZONE;

        shaderPath = pNewShader ? pNewShader->GetResourcePath() : SR_UTILS_NS::Path();

        if (pShader) {
            SR_SAFE_DELETE_PTR(m_shaderSubscription);
            pShader->RemoveUsePoint();
        }

        pShader = std::move(pNewShader);
        if (pShader) {
            pShader->AddUsePoint();

            m_shaderSubscription = pShader->SubscribeDynamic(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                pOwnedMaterialData->OnPropertyChanged(false);
                UpdateProperties();
            });
        }

        UpdateProperties();

        if (pOwnedMaterialData) {
            pOwnedMaterialData->OnShaderChanged();
        }
    }

    void MaterialShaderData::OnSamplerChanged(SR_GTYPES_NS::Texture::Ptr pOldTexture, SR_GTYPES_NS::Texture::Ptr pNewTexture) noexcept {
        if (pOldTexture == pNewTexture) {
            return;
        }

        if (pOldTexture) {
            if (auto&& pIt = m_textureSubscriptions.find(pOldTexture); pIt != m_textureSubscriptions.end()) {
                SRAssert(pIt->second.second > 0);
                --pIt->second.second;
                if (pIt->second.second == 0) {
                    SR_SAFE_DELETE_PTR(pIt->second.first);
                    m_textureSubscriptions.erase(pIt);
                }
            }
            else {
                SRHalt("MaterialShaderData::OnSamplerChanged() : texture subscription not found!");
            }
        }

        if (pNewTexture) {
            std::pair<SR_UTILS_NS::Subscription*, uint32_t>& subscription = m_textureSubscriptions[pNewTexture];
            ++subscription.second;

            if (subscription.second == 1) {
                subscription.first = pNewTexture->SubscribeDynamic(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                    pOwnedMaterialData->OnPropertyChanged(false);
                });
            }
        }

        if (pOwnedMaterialData) {
            pOwnedMaterialData->OnPropertyChanged(false);
        }
    }

    MaterialPropertyChangeResult MaterialShaderData::SetData(SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, ShaderVarType type) noexcept {
        SR_TRACY_ZONE;

        if (IsSamplerType(type)) {
            for (MaterialShaderProperty& sampler : samplers) {
                if (sampler.id == id) {
                    if (sampler.type != type) {
                        SR_ERROR("MaterialShaderData::SetData() : invalid property!\n\tProperty: {}\n\tLoaded type: {}\n\tExpected type: {}", id, type, sampler.type);
                        return MaterialPropertyChangeResult::Error;
                    }

                    if (sampler.data) {
                        if (std::get<SR_GTYPES_NS::Texture::Ptr>(*sampler.data) == std::get<SR_GTYPES_NS::Texture::Ptr>(v)) {
                            return MaterialPropertyChangeResult::None;
                        }

                        if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(*sampler.data)) {
                            OnSamplerChanged(pTexture, nullptr);
                            pTexture->RemoveUsePoint();
                        }
                    }

                    sampler.data = v;

                    if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(*sampler.data)) {
                        pTexture->AddUsePoint();
                        OnSamplerChanged(nullptr, pTexture);
                    }
                    return MaterialPropertyChangeResult::ReDraw;
                }
            }
        }
        else {
            for (MaterialShaderProperty& uniform : uniforms) {
                if (uniform.id == id) {
                    if (uniform.type != type) {
                        SR_ERROR("MaterialShaderData::SetData() : invalid property!\n\tProperty: {}\n\tLoaded type: {}\n\tExpected type: {}", id, type, uniform.type);
                        return MaterialPropertyChangeResult::Error;
                    }

                    if (!uniform.data) {
                        SRHalt("MaterialShaderData::SetData() : property data is null! Property id: {}, Type: {}", id, type);
                        return MaterialPropertyChangeResult::Error;
                    }

                    switch (type) {
                        case ShaderVarType::Int:
                        case ShaderVarType::Bool:
                            if (std::get<int32_t>(*uniform.data) == std::get<int32_t>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Float:
                            if (std::get<float_t>(*uniform.data) == std::get<float_t>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Vec2:
                            if (std::get<SR_MATH_NS::FVector2>(*uniform.data) == std::get<SR_MATH_NS::FVector2>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Vec3:
                            if (std::get<SR_MATH_NS::FVector3>(*uniform.data) == std::get<SR_MATH_NS::FVector3>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::IVec3:
                            if (std::get<SR_MATH_NS::IVector3>(*uniform.data) == std::get<SR_MATH_NS::IVector3>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Vec4:
                            if (std::get<SR_MATH_NS::FVector4>(*uniform.data) == std::get<SR_MATH_NS::FVector4>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        default:
                            SRHalt("MaterialShaderData::SetData() : unknown property type! Property id: {}, Type: {}", id, type);
                            return MaterialPropertyChangeResult::Error;
                    }

                    uniform.data = v;
                    return uniform.pushConstant ? MaterialPropertyChangeResult::ReDraw : MaterialPropertyChangeResult::Update;
                }
            }
        }

        return MaterialPropertyChangeResult::None;
    }

    void MaterialShaderData::Init() {
        SR_TRACY_ZONE;
        SetShader(shaderPath);
    }

    void MaterialShaderData::UpdateProperties() {
        SR_TRACY_ZONE;

        static ShaderProperties empty;
        const ShaderProperties& properties = pShader ? pShader->GetProperties() : empty;

        samplers.reserve(16);
        uniforms.reserve(16);

        std::set<SR_UTILS_NS::StringAtom> uniformsIds;
        std::set<SR_UTILS_NS::StringAtom> samplersIds;

        uint32_t order = 0;

        for (const ShaderProperty& property : properties) {
            bool found = false;

            if (IsSamplerType(property.type)) {
                samplersIds.insert(property.id);

                for (MaterialShaderProperty& sampler : samplers) {
                    if (sampler.id == property.id) {
                        found = true;
                        sampler.editorOrder = order;
                        sampler.displayName = SR_UTILS_NS::Reflection::MakeDisplayName(property.id);
                        if (sampler.type != property.type || !sampler.data) {
                            sampler.type = property.type;
                            sampler.data = property.GetData();
                        }
                        break;
                    }
                }

                if (!found) {
                    MaterialShaderProperty& sampler = samplers.emplace_back();
                    sampler.editorOrder = order;
                    sampler.id = property.id;
                    sampler.type = property.type;
                    sampler.pushConstant = property.pushConstant;
                    sampler.data = property.GetData();
                    sampler.displayName = SR_UTILS_NS::Reflection::MakeDisplayName(property.id);
                }
            }
            else {
                uniformsIds.insert(property.id);

                for (MaterialShaderProperty& uniform : uniforms) {
                    if (uniform.id == property.id) {
                        found = true;
                        uniform.editorOrder = order;
                        uniform.displayName = SR_UTILS_NS::Reflection::MakeDisplayName(property.id);
                        if (uniform.type != property.type || !uniform.data) {
                            uniform.type = property.type;
                            uniform.data = property.GetData();
                        }
                        break;
                    }
                }

                if (!found) {
                    MaterialShaderProperty& uniform = uniforms.emplace_back();
                    uniform.editorOrder = order;
                    uniform.id = property.id;
                    uniform.type = property.type;
                    uniform.pushConstant = property.pushConstant;
                    uniform.data = property.GetData();
                    uniform.displayName = SR_UTILS_NS::Reflection::MakeDisplayName(property.id);
                }
            }

            ++order;
        }

        for (auto it = uniforms.begin(); it != uniforms.end();) {
            if (uniformsIds.find(it->id) == uniformsIds.end()) {
                it = uniforms.erase(it);
            }
            else {
                ++it;
            }
        }

        for (auto it = samplers.begin(); it != samplers.end();) {
            if (samplersIds.find(it->id) == samplersIds.end()) {
                if (it->data) {
                    if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(*it->data)) {
                        pTexture->RemoveUsePoint();
                        OnSamplerChanged(pTexture, nullptr);
                    }
                }
                it = samplers.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void MaterialShaderData::CloneTo(SR_UTILS_NS::SRClass& clone) const {
        SR_TRACY_ZONE;
        Super::CloneTo(clone);

        static_cast<MaterialShaderData&>(clone).Init();
        ForEachProperty([&clone](const MaterialShaderProperty& property) {
            SRAssert(property.data);
            static_cast<MaterialShaderData&>(clone).SetData(property.id, *property.data, property.type);
        });
    }

    bool MaterialShaderData::operator==(const MaterialShaderData& other) const {
        return shaderPath == other.shaderPath &&
            useType == other.useType &&
            uniforms == other.uniforms &&
            samplers == other.samplers;

    }

    SR_GTYPES_NS::Texture::Ptr MaterialShaderData::GetSamplerTexture(SR_UTILS_NS::StringAtom id) const noexcept {
        SR_TRACY_ZONE;

        for (const MaterialShaderProperty& sampler : samplers) {
            if (sampler.id == id) {
                if (sampler.data) {
                    return std::get<SR_GTYPES_NS::Texture::Ptr>(*sampler.data);
                }
                return nullptr;
            }
        }
        return nullptr;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    MaterialData::MaterialData()
        : SR_HTYPES_NS::SharedPtr<MaterialData>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    {
        m_defaultShader.pOwnedMaterialData = this;
    }

    MaterialData::~MaterialData() = default;

    void MaterialData::UseUniforms(SR_GTYPES_NS::Shader& shader) {
        MaterialShaderData& shaderData = GetDefaultShaderData();

        if (shaderData.useType != MaterialStageUseType::Uniforms && shaderData.useType != MaterialStageUseType::Full) {
            return;
        }

        for (const MaterialShaderProperty& uniform : shaderData.uniforms) {
            if (!uniform.data) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("MaterialData::UseUniforms() : property data is null! Property id: {}, Type: {}", uniform.id, uniform.type);
                continue;
            }

            switch (uniform.type) {
                case ShaderVarType::Int:
                case ShaderVarType::Bool:
                    shader.SetInt(uniform.id, std::get<int32_t>(*uniform.data));
                    break;
                case ShaderVarType::Float:
                    shader.SetFloat(uniform.id, std::get<float_t>(*uniform.data));
                    break;
                case ShaderVarType::Vec2:
                    shader.SetVec2(uniform.id, std::get<SR_MATH_NS::FVector2>(*uniform.data).template Cast<float_t>());
                    break;
                case ShaderVarType::Vec3:
                    shader.SetVec3(uniform.id, std::get<SR_MATH_NS::FVector3>(*uniform.data).template Cast<float_t>());
                    break;
                case ShaderVarType::IVec3:
                    shader.SetIVec3(uniform.id, std::get<SR_MATH_NS::IVector3>(*uniform.data).template Cast<int32_t>());
                    break;
                case ShaderVarType::Vec4:
                    shader.SetVec4(uniform.id, std::get<SR_MATH_NS::FVector4>(*uniform.data).template Cast<float_t>());
                    break;
                default:
                    SR_ERROR("MaterialData::UseUniforms() : unknown property type! Property id: {}, Type: {}", uniform.id, uniform.type);
                    break;
            }
        }
    }

    void MaterialData::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;

        MaterialShaderData& shaderData = GetDefaultShaderData();

        if (shaderData.useType != MaterialStageUseType::Samplers && shaderData.useType != MaterialStageUseType::Full) {
            return;
        }

        for (const MaterialShaderProperty& sampler : shaderData.samplers) {
            if (!sampler.data) SR_UNLIKELY_ATTRIBUTE {
                SRHalt("MaterialData::UseSamplers() : property data is null! Property id: {}, Type: {}", sampler.id, sampler.type);
                continue;
            }

            if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(*sampler.data)) {
                shader.SetSampler2D(sampler.id, pTexture);
            }
            else {
                shader.SetSampler2D(sampler.id, nullptr);
            }
        }
    }

    MaterialShaderData* MaterialData::GetShaderData(SR_UTILS_NS::StringAtom id) noexcept {
        if (id.empty()) {
            return &m_defaultShader;
        }

        return nullptr;
    }

    const MaterialShaderData* MaterialData::GetShaderData(SR_UTILS_NS::StringAtom id) const noexcept {
        return const_cast<MaterialData*>(this)->GetShaderData(id);
    }

    void MaterialData::SetSampler(SR_UTILS_NS::StringAtom id, const SR_UTILS_NS::Path& path) noexcept {
        SR_TRACY_ZONE;

        if (auto&& pTexture = CoreResLoader::Load<SR_GTYPES_NS::Texture>(path)) {
            SetData(id, pTexture, ShaderVarType::Sampler2D);
        }
        else {
            SR_ERROR("MaterialData::SetSampler() : failed to load texture! \n\tPath: " + path.ToString());
        }
    }

    void MaterialData::SetData(const SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, const ShaderVarType type) noexcept {
        SR_TRACY_ZONE;

        uint8_t changeResult = std::max(static_cast<uint8_t>(0), static_cast<uint8_t>(m_defaultShader.SetData(id, v, type)));

        if (changeResult == static_cast<uint8_t>(MaterialPropertyChangeResult::ReDraw)) {
            OnPropertyChanged(false);
        }
        else if (changeResult == static_cast<uint8_t>(MaterialPropertyChangeResult::Update)) {
            OnPropertyChanged(true);
        }
    }

    void MaterialData::OnShaderDefinesChanged() {
        SR_TRACY_ZONE;
        if (!GetDefaultShaderData().pShader) {
            return;
        }

        GetDefaultShaderData().SetShader(GetDefaultShaderData().shaderPath);
    }

    void MaterialData::OnShaderChanged() {
        SR_TRACY_ZONE;
        Broadcast(SHADER_CHANGED_EVENT);
    }

    void MaterialData::OnPropertyChanged(const bool onlyUniforms) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::SubscriptionMessage msg;
        msg.SetBool(ONLY_UNIFORMS_BOOL_ID, onlyUniforms);
        Broadcast(PROPERTY_CHANGED_EVENT, msg);
    }

    void MaterialData::SwitchShaderDefine(SR_UTILS_NS::StringAtom define, bool enabled) {
        if (enabled) {
            AddShaderDefine(define);
        }
        else {
            RemoveShaderDefine(define);
        }
    }

    void MaterialData::AddShaderDefine(SR_UTILS_NS::StringAtom define, const std::string& value) {
        SR_TRACY_ZONE;
        if (m_shaderDefines.find(define) != m_shaderDefines.end()) {
            return;
        }
        m_shaderDefines[define] = value;
        OnShaderDefinesChanged();
    }

    void MaterialData::RemoveShaderDefine(SR_UTILS_NS::StringAtom define) {
        SR_TRACY_ZONE;
        if (m_shaderDefines.find(define) == m_shaderDefines.end()) {
            return;
        }
        m_shaderDefines.erase(define);
        OnShaderDefinesChanged();
    }
}
