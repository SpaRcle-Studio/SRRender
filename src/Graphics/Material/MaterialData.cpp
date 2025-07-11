//
// Created by Monika on 08.02.2025.
//

#include <Graphics/Material/MaterialData.h>

#include <Enum/ShaderVarType.hpp>

#include <Codegen/MaterialData.generated.hpp>

namespace SR_GRAPH_NS {
    void MaterialShaderProperty::Save(SR_UTILS_NS::ISerializer& serializer) const {
        Super::Save(serializer);

        if (IsSamplerType(type)) {
            if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(data)) {
                SR_UTILS_NS::Serialization::Save(serializer, pTexture->GetResourcePath(), SR_UTILS_NS::SerializationId::Create("value"));
            }
            return;
        }

        switch (type) {
            case ShaderVarType::Int:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<int32_t>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Bool:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<int32_t>(data) != 0, SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Float:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<float_t>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec2:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::FVector2>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec3:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::FVector3>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::IVec3:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::IVector3>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec4:
                SR_UTILS_NS::Serialization::Save(serializer, std::get<SR_MATH_NS::FVector4>(data), SR_UTILS_NS::SerializationId::Create("value"));
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

            auto&& pTexture = SR_GTYPES_NS::Texture::Load(path);

            if (auto&& pOldTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&data)) {
                if (*pOldTextureRef) {
                    (*pOldTextureRef)->RemoveUsePoint();
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
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<int32_t>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Bool: {
                bool boolean = false;
                SR_UTILS_NS::Serialization::Load(deserializer, boolean, SR_UTILS_NS::SerializationId::Create("value"));
                std::get<int32_t>(data) = boolean ? 1 : 0;
                break;
            }
            case ShaderVarType::Float:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<float_t>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec2:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::FVector2>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec3:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::FVector3>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::IVec3:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::IVector3>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            case ShaderVarType::Vec4:
                SR_UTILS_NS::Serialization::Load(deserializer, std::get<SR_MATH_NS::FVector4>(data), SR_UTILS_NS::SerializationId::Create("value"));
                break;
            default:
                SRHalt("MaterialShaderProperty::Load() : unknown property type! Property id: {}, Type: {}", id, type);
                break;
        }

        return true;
    }

    void MaterialShaderData::OnPreLoad() {
        for (MaterialShaderProperty& sampler : samplers) {
            pOwnedMaterialData->OnSamplerChanged(std::get<SR_GTYPES_NS::Texture::Ptr>(sampler.data), nullptr);
        }
        Serializable::OnPreLoad();
    }

    void MaterialShaderData::OnPostLoad() {
        for (MaterialShaderProperty& sampler : samplers) {
            pOwnedMaterialData->OnSamplerChanged(nullptr, std::get<SR_GTYPES_NS::Texture::Ptr>(sampler.data));
        }
        Serializable::OnPostLoad();
    }

    void MaterialShaderData::ForEachProperty(const SR_HTYPES_NS::Function<void(MaterialShaderProperty&)>& func) {
        for (MaterialShaderProperty& uniform : uniforms) {
            func(uniform);
        }
        for (MaterialShaderProperty& sampler : samplers) {
            func(sampler);
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

                    if (std::get<SR_GTYPES_NS::Texture::Ptr>(sampler.data) == std::get<SR_GTYPES_NS::Texture::Ptr>(v)) {
                        return MaterialPropertyChangeResult::None;
                    }

                    if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(sampler.data)) {
                        pTexture->RemoveUsePoint();
                        if (SRVerify(pOwnedMaterialData)) {
                            pOwnedMaterialData->OnSamplerChanged(pTexture, nullptr);
                        }
                    }

                    sampler.data = v;

                    if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(sampler.data)) {
                        pTexture->AddUsePoint();
                        if (SRVerify(pOwnedMaterialData)) {
                            pOwnedMaterialData->OnSamplerChanged(nullptr, pTexture);
                        }
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

                    switch (type) {
                        case ShaderVarType::Int:
                        case ShaderVarType::Bool:
                            if (std::get<int32_t>(uniform.data) == std::get<int32_t>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Float:
                            if (std::get<float_t>(uniform.data) == std::get<float_t>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Vec2:
                            if (std::get<SR_MATH_NS::FVector2>(uniform.data) == std::get<SR_MATH_NS::FVector2>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Vec3:
                            if (std::get<SR_MATH_NS::FVector3>(uniform.data) == std::get<SR_MATH_NS::FVector3>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::IVec3:
                            if (std::get<SR_MATH_NS::IVector3>(uniform.data) == std::get<SR_MATH_NS::IVector3>(v)) {
                                return MaterialPropertyChangeResult::None;
                            }
                            break;
                        case ShaderVarType::Vec4:
                            if (std::get<SR_MATH_NS::FVector4>(uniform.data) == std::get<SR_MATH_NS::FVector4>(v)) {
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

    void MaterialShaderData::UpdateProperties() {
        const ShaderProperties& properties = pShader->GetProperties();

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
                        if (sampler.type != property.type) {
                            sampler.type = property.type;
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
                        if (uniform.type != property.type) {
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
                if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(it->data)) {
                    pTexture->RemoveUsePoint();
                    if (SRVerify(pOwnedMaterialData)) {
                        pOwnedMaterialData->OnSamplerChanged(pTexture, nullptr);
                    }
                }
                it = samplers.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

    MaterialData::MaterialData()
        : SR_HTYPES_NS::SharedPtr<MaterialData>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    {
        m_defaultShader.pOwnedMaterialData = this;
    }

    MaterialData::~MaterialData() {
        Finalize();
    }

    void MaterialData::Save(SR_UTILS_NS::ISerializer& serializer) const {
        SR_TRACY_ZONE;

        /// default shader
        {
            serializer.BeginObject(SR_UTILS_NS::SerializationId::Create("default"));
            const SR_UTILS_NS::Path path = m_defaultShader.pShader ? m_defaultShader.pShader->GetResourcePath() : SR_UTILS_NS::Path();
            SR_UTILS_NS::Serialization::Save(serializer, path, SR_UTILS_NS::SerializationId::Create("shader"));
            SR_UTILS_NS::Serialization::Save(serializer, m_defaultShader, SR_UTILS_NS::SerializationId::Create("data"));
            serializer.EndObject();
        }

        serializer.BeginArray(m_shaders.size(), SR_UTILS_NS::SerializationId::Create("stages"));

        for (const auto& [stage, shaderData] : m_shaders) {
            SRAssert(shaderData.pShader);

            serializer.BeginItem(SR_UTILS_NS::SerializationId::Create("stage"));

            const SR_UTILS_NS::Path path = shaderData.pShader ? shaderData.pShader->GetResourcePath() : SR_UTILS_NS::Path();
            SR_UTILS_NS::Serialization::Save(serializer, stage, SR_UTILS_NS::SerializationId::Create("id"));
            SR_UTILS_NS::Serialization::Save(serializer, path, SR_UTILS_NS::SerializationId::Create("shader"));
            SR_UTILS_NS::Serialization::Save(serializer, shaderData, SR_UTILS_NS::SerializationId::Create("data"));

            serializer.EndItem();
        }

        serializer.EndArray();
    }

    bool MaterialData::Load(SR_UTILS_NS::IDeserializer& deserializer) {
        SR_TRACY_ZONE;

        if (!Super::Load(deserializer)) {
            return false;
        }

        Finalize();

        {
            deserializer.BeginObject(SR_UTILS_NS::SerializationId::Create("default"));
            SR_UTILS_NS::Path path;
            SR_UTILS_NS::Serialization::Load(deserializer, path, SR_UTILS_NS::SerializationId::Create("shader"));
            if (auto&& pShader = SR_GTYPES_NS::Shader::Load(path)) {
                SetShader(pShader);
                SR_UTILS_NS::Serialization::Load(deserializer, m_defaultShader, SR_UTILS_NS::SerializationId::Create("data"));
                m_defaultShader.UpdateProperties();
            }
            else {
                SR_ERROR("MaterialData::Load() : failed to load default shader! Path: {}", path.ToString());
            }
            deserializer.EndObject();
        }

        const uint64_t size = deserializer.BeginArray(SR_UTILS_NS::SerializationId::Create("stages"));
        if (size > 0) {
            uint64_t index = 0;

            while (deserializer.BeginItem(SR_UTILS_NS::SerializationId::Create("stage"), index)) {
                SR_UTILS_NS::StringAtom stage;
                SR_UTILS_NS::Path path;

                SR_UTILS_NS::Serialization::Load(deserializer, stage, SR_UTILS_NS::SerializationId::Create("id"));
                SR_UTILS_NS::Serialization::Load(deserializer, path, SR_UTILS_NS::SerializationId::Create("shader"));

                if (auto&& pShader = SR_GTYPES_NS::Shader::Load(path)) {
                    SetShader(pShader, stage);
                    SR_UTILS_NS::Serialization::Load(deserializer, m_shaders[stage], SR_UTILS_NS::SerializationId::Create("data"));
                    m_shaders[stage].UpdateProperties();
                }
                else {
                    SR_ERROR("MaterialData::Load() : failed to load shader! Path: {}", path.ToString());
                }

                deserializer.EndItem();
                index++;
            }

            deserializer.EndArray();
        }

        return true;
    }

    SR_GTYPES_NS::Shader::Ptr MaterialData::GetShader(const Pipeline* pPipeline) const noexcept {
        SR_TRACY_ZONE;

        if (auto&& pIt = m_shaders.find(pPipeline->GetRenderStageId()); pIt != m_shaders.end()) {
            return pIt->second.pShader;
        }
        return m_defaultShader.pShader;
    }

    void MaterialData::Finalize() {
        SR_TRACY_ZONE;

        for (auto&& [stage, data] : m_shaders) {
            if (data.pShader) {
                data.pShader->RemoveUsePoint();
                data.pShader = nullptr;
            }
            for (MaterialShaderProperty& sampler : data.samplers) {
                if (auto&& pTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&sampler.data)) {
                    if (*pTextureRef) {
                        (*pTextureRef)->RemoveUsePoint();
                    }
                }
            }
            data.samplers.clear();
        }
        m_shaders.clear();

        if (m_defaultShader.pShader) {
            m_defaultShader.pShader->RemoveUsePoint();
            m_defaultShader.pShader = nullptr;
        }
        for (MaterialShaderProperty& sampler : m_defaultShader.samplers) {
            if (auto&& pTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&sampler.data)) {
                if (*pTextureRef) {
                    (*pTextureRef)->RemoveUsePoint();
                }
            }
        }
        m_defaultShader.samplers.clear();

        m_shaderSubscriptions.clear();
        m_textureSubscriptions.clear();
    }

    void MaterialData::UseUniforms(const Pipeline* pPipeline) {
        SR_TRACY_ZONE;

        const SR_UTILS_NS::StringAtom renderStageId = pPipeline->GetRenderStageId();
        SR_GTYPES_NS::Shader* pShader = pPipeline->GetCurrentShader();

        MaterialShaderData& shaderData = m_defaultShader;
        if (auto&& pIt = m_shaders.find(renderStageId); pIt != m_shaders.end()) {
            shaderData = pIt->second;
        }

        for (MaterialShaderProperty& uniform : shaderData.uniforms) {
            switch (uniform.type) {
                case ShaderVarType::Int:
                case ShaderVarType::Bool:
                    pShader->SetInt(uniform.id, std::get<int32_t>(uniform.data));
                    break;
                case ShaderVarType::Float:
                    pShader->SetFloat(uniform.id, std::get<float_t>(uniform.data));
                    break;
                case ShaderVarType::Vec2:
                    pShader->SetVec2(uniform.id, std::get<SR_MATH_NS::FVector2>(uniform.data).template Cast<float_t>());
                    break;
                case ShaderVarType::Vec3:
                    pShader->SetVec3(uniform.id, std::get<SR_MATH_NS::FVector3>(uniform.data).template Cast<float_t>());
                    break;
                case ShaderVarType::IVec3:
                    pShader->SetIVec3(uniform.id, std::get<SR_MATH_NS::IVector3>(uniform.data).template Cast<int32_t>());
                    break;
                case ShaderVarType::Vec4:
                    pShader->SetVec4(uniform.id, std::get<SR_MATH_NS::FVector4>(uniform.data).template Cast<float_t>());
                    break;
                default:
                    SR_ERROR("MaterialData::UseUniforms() : unknown property type! Property id: {}, Type: {}", uniform.id, uniform.type);
                    break;
            }
        }
    }

    void MaterialData::UseSamplers(const Pipeline* pPipeline) {
        SR_TRACY_ZONE;

        const SR_UTILS_NS::StringAtom renderStageId = pPipeline->GetRenderStageId();
        SR_GTYPES_NS::Shader* pShader = pPipeline->GetCurrentShader();

        MaterialShaderData& shaderData = m_defaultShader;
        if (auto&& pIt = m_shaders.find(renderStageId); pIt != m_shaders.end()) {
            shaderData = pIt->second;
        }

        for (MaterialShaderProperty& sampler : shaderData.samplers) {
            if (auto&& pTexture = std::get<SR_GTYPES_NS::Texture::Ptr>(sampler.data)) {
                pShader->SetSampler2D(sampler.id, pTexture);
            }
            else {
                pShader->SetSampler2D(sampler.id, nullptr);
            }
        }
    }

    MaterialShaderData* MaterialData::GetShaderData(SR_UTILS_NS::StringAtom id) noexcept {
        if (auto&& pIt = m_shaders.find(id); pIt != m_shaders.end()) {
            return &pIt->second;
        }
        return nullptr;
    }

    const MaterialShaderData* MaterialData::GetShaderData(SR_UTILS_NS::StringAtom id) const noexcept {
        if (auto&& pIt = m_shaders.find(id); pIt != m_shaders.end()) {
            return &pIt->second;
        }
        return nullptr;
    }

    void MaterialData::SetSampler(SR_UTILS_NS::StringAtom id, const SR_UTILS_NS::Path& path) noexcept {
        SR_TRACY_ZONE;

        if (auto&& pTexture = SR_GTYPES_NS::Texture::Load(path)) {
            SetData(id, pTexture, ShaderVarType::Sampler2D);
        }
        else {
            SR_ERROR("MaterialData::SetSampler() : failed to load texture! \n\tPath: " + path.ToString());
        }
    }

    void MaterialData::SetShader(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom stage) {
        SR_TRACY_ZONE;

        static const SR_UTILS_NS::StringAtom defaultStage = "Default";
        if (stage == defaultStage) {
            stage = SR_UTILS_NS::StringAtom();
        }

        if (auto&& pShader = SR_GTYPES_NS::Shader::Load(path)) {
            SetShader(pShader, stage);
        }
        else {
            SR_ERROR("MaterialData::SetShader() : failed to load shader! \n\tPath: " + path.ToString());
        }
    }

    void MaterialData::SetShader(SR_GTYPES_NS::Shader::Ptr pShader, const SR_UTILS_NS::StringAtom stage) {
        SR_TRACY_ZONE;

        if (!pShader) {
            SR_ERROR("MaterialData::SetShader() : shader is nullptr!");
            return;
        }

        MaterialShaderData* pShaderData = &m_defaultShader;
        if (!stage.Empty()) {
            if (auto&& pIt = m_shaders.find(stage); pIt != m_shaders.end()) {
                pShaderData = &pIt->second;
            }
            else {
                pShaderData = &m_shaders[stage];
                pShaderData->pOwnedMaterialData = this;
            }
        }

        if (pShaderData->pShader == pShader) {
            return;
        }

        if (pShaderData->pShader) {
            pShaderData->pShader->RemoveUsePoint();

            if (auto&& pIt = m_shaderSubscriptions.find(pShaderData->pShader); pIt != m_shaderSubscriptions.end()) {
                SRAssert(pIt->second.second > 0);
                --pIt->second.second;
                if (pIt->second.second == 0) {
                    m_shaderSubscriptions.erase(pIt);
                }
            }
            else {
                SRHalt("MaterialData::SetShader() : shader subscription not found!");
            }
        }

        pShaderData->pShader = pShader;
        pShader->AddUsePoint();

        std::pair<SR_UTILS_NS::Subscription, uint32_t>& subscription = m_shaderSubscriptions[pShader];
        ++subscription.second;

        if (subscription.second == 1) {
            subscription.first = pShader->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT,
                [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                    OnPropertyChanged(false);
                }
            );
        }

        pShaderData->UpdateProperties();

        if (pShaderData->pOwnedMaterialData) {
            pShaderData->pOwnedMaterialData->OnShaderChanged();
        }

        SRAssert(stage.Empty() || (m_shaders[stage].pShader == pShader && pShader));
    }

    void MaterialData::SetData(const SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, const ShaderVarType type) noexcept {
        SR_TRACY_ZONE;

        uint8_t changeResult = std::max(static_cast<uint8_t>(0), static_cast<uint8_t>(m_defaultShader.SetData(id, v, type)));

        for (auto&& [stage, data] : m_shaders) {
            changeResult = std::max(changeResult, static_cast<uint8_t>(data.SetData(id, v, type)));
        }

        if (changeResult == static_cast<uint8_t>(MaterialPropertyChangeResult::ReDraw)) {
            OnPropertyChanged(false);
        }
        else if (changeResult == static_cast<uint8_t>(MaterialPropertyChangeResult::Update)) {
            OnPropertyChanged(true);
        }
    }

    void MaterialData::OnSamplerChanged(SR_GTYPES_NS::Texture::Ptr pOldTexture, SR_GTYPES_NS::Texture::Ptr pNewTexture) noexcept {
        if (pOldTexture == pNewTexture) {
            return;
        }

        if (pOldTexture) {
            if (auto&& pIt = m_textureSubscriptions.find(pOldTexture); pIt != m_textureSubscriptions.end()) {
                SRAssert(pIt->second.second > 0);
                --pIt->second.second;
                if (pIt->second.second == 0) {
                    m_textureSubscriptions.erase(pIt);
                }
            }
            else {
                SRHalt("MaterialData::OnSamplerChanged() : texture subscription not found!");
            }
        }

        if (pNewTexture) {
            std::pair<SR_UTILS_NS::Subscription, uint32_t>& subscription = m_textureSubscriptions[pNewTexture];
            ++subscription.second;

            if (subscription.second == 1) {
                subscription.first = pNewTexture->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT,
                    [this](const SR_UTILS_NS::SubscriptionMessage& msg) {
                        OnPropertyChanged(false);
                    }
                );
            }
        }
    }

    void MaterialData::RemoveStage(const SR_UTILS_NS::StringAtom stage) {
        auto&& pStageIt = m_shaders.find(stage);
        if (pStageIt == m_shaders.end()) {
            return;
        }

        if (auto&& pIt = m_shaderSubscriptions.find(pStageIt->second.pShader); pIt != m_shaderSubscriptions.end()) {
            SRAssert(pIt->second.second > 0);
            --pIt->second.second;
            if (pIt->second.second == 0) {
                m_shaderSubscriptions.erase(pIt);
            }
        }

        pStageIt->second.pShader->RemoveUsePoint();
        pStageIt->second.pShader = nullptr;

        for (MaterialShaderProperty& sampler : pStageIt->second.samplers) {
            if (auto&& pTextureRef = std::get_if<SR_GTYPES_NS::Texture::Ptr>(&sampler.data)) {
                if (*pTextureRef) {
                    (*pTextureRef)->RemoveUsePoint();

                    if (auto&& pIt = m_textureSubscriptions.find(*pTextureRef); pIt != m_textureSubscriptions.end()) {
                        SRAssert(pIt->second.second > 0);
                        --pIt->second.second;
                        if (pIt->second.second == 0) {
                            m_textureSubscriptions.erase(pIt);
                        }
                    }
                }
            }
        }

        m_shaders.erase(pStageIt);

        OnShaderChanged();
    }

    void MaterialData::OnShaderChanged() {
        Broadcast(SHADER_CHANGED_EVENT);
    }

    void MaterialData::OnPropertyChanged(const bool onlyUniforms) {
        SR_UTILS_NS::SubscriptionMessage msg;
        msg.SetBool(ONLY_UNIFORMS_BOOL_ID, onlyUniforms);
        Broadcast(PROPERTY_CHANGED_EVENT, msg);
    }
}
