//
// Created by Monika on 08.02.2025.
//

#ifndef SR_ENGINE_GRAPHICS_MATERIAL_DATA_H
#define SR_ENGINE_GRAPHICS_MATERIAL_DATA_H

#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Texture.h>

#include <Utils/Serialization/Serializable.h>

namespace SR_GRAPH_NS {
    /*SR_ENUM_NS_CLASS_T(MaterialShader, uint16_t,
        Default,
        Simple,
        Shadows, SSAO, HDAO, HBAO, VXAO, Bloom,
        SSAOShadows, HDAOShadows, HBAOShadows, VXAOShadows,
        SSAOShadowsBloom
    );*/

    class MaterialData;

    enum class MaterialPropertyChangeResult : uint8_t {
        None, Error, Update, ReDraw
    };

    struct MaterialShaderProperty : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
    private:
        using Super = SR_UTILS_NS::Serializable;

    public:
        void Save(SR_UTILS_NS::ISerializer& serializer) const override;
        bool Load(SR_UTILS_NS::IDeserializer& deserializer) override;

    public:
        uint32_t editorOrder = 0;
        SR_UTILS_NS::StringAtom displayName;
        std::optional<ShaderPropertyVariant> data;

        /// @property
        SR_UTILS_NS::StringAtom id;
        /// @property
        ShaderVarType type = ShaderVarType::Unknown;
        /// @property
        bool pushConstant = false;

        bool operator==(const MaterialShaderProperty& other) const;
        bool operator!=(const MaterialShaderProperty& other) const { return !(*this == other); }

    };

    struct MaterialShaderData : public SR_UTILS_NS::Serializable {
        using Super = SR_UTILS_NS::Serializable;

        SR_STRUCT()

        ~MaterialShaderData() override;

        /// @property
        SR_UTILS_NS::Path shaderPath;

        /// @property
        std::vector<MaterialShaderProperty> uniforms;
        /// @property
        std::vector<MaterialShaderProperty> samplers;
        /// @property
        MaterialStageUseType useType = MaterialStageUseType::Full;

        void OnPreLoad() override;
        void OnPostLoad() override;

        void CloneTo(SR_UTILS_NS::SRClass& clone) const override;

        void SetShader(const SR_UTILS_NS::Path& path);
        void SetShader(SR_GTYPES_NS::Shader::Ptr pShader);

        void ForEachProperty(const SR_HTYPES_NS::Function<void(MaterialShaderProperty&)>& func);
        void ForEachProperty(const SR_HTYPES_NS::Function<void(const MaterialShaderProperty&)>& func) const;

        MaterialPropertyChangeResult SR_FASTCALL SetData(SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, ShaderVarType type) noexcept;

        SR_NODISCARD SR_GTYPES_NS::Texture::Ptr GetSamplerTexture(SR_UTILS_NS::StringAtom id) const noexcept;

        void UpdateProperties();
        void Init();

        void SR_FASTCALL OnSamplerChanged(SR_GTYPES_NS::Texture::Ptr pOldTexture, SR_GTYPES_NS::Texture::Ptr pNewTexture) noexcept;

        MaterialData* pOwnedMaterialData = nullptr;
        SR_GTYPES_NS::Shader::Ptr pShader = nullptr;

        SR_UTILS_NS::Subscription* m_shaderSubscription = nullptr;
        std::map<SR_GTYPES_NS::Texture::Ptr, std::pair<SR_UTILS_NS::Subscription*, uint32_t>> m_textureSubscriptions;

        bool operator==(const MaterialShaderData& other) const;
        bool operator!=(const MaterialShaderData& other) const { return !(*this == other); }

    };

    /// @inspector(MaterialDataPropertyDrawer)
    class MaterialData final : public SR_UTILS_NS::Serializable
        , public SR_UTILS_NS::NonCopyable
        , public SR_HTYPES_NS::SharedPtr<MaterialData>
        , public SR_UTILS_NS::SubscriptionHolder
    {
        using Super = SR_UTILS_NS::Serializable;
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<MaterialData>;

        SR_INLINE_STATIC const SR_UTILS_NS::StringAtom PROPERTY_CHANGED_EVENT = "PropertyChanged";
        SR_INLINE_STATIC const SR_UTILS_NS::StringAtom SHADER_CHANGED_EVENT = "ShaderChanged";
        SR_INLINE_STATIC const SR_UTILS_NS::StringAtom ONLY_UNIFORMS_BOOL_ID = "OnlyUniforms";

        MaterialData();
        ~MaterialData() override;

        void UseUniforms(SR_GTYPES_NS::Shader& shader);
        void UseSamplers(SR_GTYPES_NS::Shader& shader);

        SR_NODISCARD MaterialShaderData& GetDefaultShaderData() noexcept { return m_defaultShader; }
        SR_NODISCARD const MaterialShaderData& GetDefaultShaderData() const noexcept { return m_defaultShader; }
        SR_NODISCARD const std::map<SR_UTILS_NS::StringAtom, std::string>& GetShaderDefines() const noexcept { return m_shaderDefines; }

        void SR_FASTCALL SetSampler(SR_UTILS_NS::StringAtom id, const SR_UTILS_NS::Path& path) noexcept;
        void SR_FASTCALL SetData(SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, ShaderVarType type) noexcept;

        void OnPropertyChanged(bool onlyUniforms);
        void AddShaderDefine(SR_UTILS_NS::StringAtom define, const std::string& value = "");
        void RemoveShaderDefine(SR_UTILS_NS::StringAtom define);
        void SwitchShaderDefine(SR_UTILS_NS::StringAtom define, bool enabled);

        void OnShaderChanged();

    private:
        void OnShaderDefinesChanged();

        SR_NODISCARD bool IsNormalMappingEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_NORMAL) == 1; }
        SR_NODISCARD bool IsRoughnessEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_ROUGHNESS) == 1; }
        SR_NODISCARD bool IsDetailWeightEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_DETAIL_WEIGHT) == 1; }
        SR_NODISCARD bool IsSSSEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_SSS) == 1; }
        SR_NODISCARD bool IsAlphaMaskEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_ALPHA_MASK) == 1; }
        SR_NODISCARD bool IsSkeletalAnimationEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_SKELETON) == 1; }
        SR_NODISCARD bool IsAlphaEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_ALPHA) == 1; }
        SR_NODISCARD bool IsBlendingDisabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_DISABLE_BLENDING) == 1; }
        SR_NODISCARD bool IsEmissionEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_EMISSION) == 1; }
        SR_NODISCARD bool IsORMEnabled() const { return m_shaderDefines.count(SHADER_MACRO_SR_DEFINE_HAS_ORM) == 1; }

        void SetNormalMappingEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_NORMAL, enabled); }
        void SetRoughnessEnabled(bool disabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_ROUGHNESS, disabled); }
        void SetDetailWeightEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_DETAIL_WEIGHT, enabled); }
        void SetSSSEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_SSS, enabled); }
        void SetAlphaMaskEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_ALPHA_MASK, enabled); }
        void SetSkeletalAnimationEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_SKELETON, enabled); }
        void SetAlphaEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_ALPHA, enabled); }
        void SetBlendingDisabled(bool disabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_DISABLE_BLENDING, disabled); }
        void SetEmissionEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_EMISSION, enabled); }
        void SetORMEnabled(bool enabled) { SwitchShaderDefine(SHADER_MACRO_SR_DEFINE_HAS_ORM, enabled); }

    private:
        /// @property @onChanged(OnShaderDefinesChanged)
        std::map<SR_UTILS_NS::StringAtom, std::string> m_shaderDefines;

        /// @property @hidden
        MaterialShaderData m_defaultShader;

        /// @virtualProperty(hasSkeleton) @getter(IsSkeletalAnimationEnabled) @setter(SetSkeletalAnimationEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasAlphaMask) @getter(IsAlphaMaskEnabled) @setter(SetAlphaMaskEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasNormals) @getter(IsNormalMappingEnabled) @setter(SetNormalMappingEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasDetailWeight) @getter(IsDetailWeightEnabled) @setter(SetDetailWeightEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasRoughness) @getter(IsRoughnessEnabled) @setter(SetRoughnessEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasEmission) @getter(IsEmissionEnabled) @setter(SetEmissionEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasSSS) @getter(IsSSSEnabled) @setter(SetSSSEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasAlpha) @getter(IsAlphaEnabled) @setter(SetAlphaEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(disableBlending) @getter(IsBlendingDisabled) @setter(SetBlendingDisabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(hasORM) @getter(IsORMEnabled) @setter(SetORMEnabled) @dontSave @group(Params)
        SR_VIRTUAL_PROPERTY


    };
}

#endif //SR_ENGINE_GRAPHICS_MATERIAL_DATA_H
