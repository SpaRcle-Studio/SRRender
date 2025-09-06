//
// Created by Monika on 08.02.2025.
//

#ifndef SR_ENGINE_GRAPHICS_MATERIAL_DATA_H
#define SR_ENGINE_GRAPHICS_MATERIAL_DATA_H

#include <Utils/Serialization/Serializable.h>

#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Texture.h>

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
        ShaderPropertyVariant data;

        /// @property
        SR_UTILS_NS::StringAtom id;
        /// @property
        ShaderVarType type = ShaderVarType::Unknown;
        /// @property
        bool pushConstant = false;
    };

    struct MaterialShaderData : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        MaterialData* pOwnedMaterialData = nullptr;
        SR_GTYPES_NS::Shader::Ptr pShader = nullptr;

        /// @property
        std::vector<MaterialShaderProperty> uniforms;
        /// @property
        std::vector<MaterialShaderProperty> samplers;
        /// @property
        MaterialStageUseType useType = MaterialStageUseType::Full;

        void OnPreLoad() override;
        void OnPostLoad() override;

        void ForEachProperty(const SR_HTYPES_NS::Function<void(MaterialShaderProperty&)>& func);

        MaterialPropertyChangeResult SR_FASTCALL SetData(SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, ShaderVarType type) noexcept;
        void UpdateProperties();

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

        void Save(SR_UTILS_NS::ISerializer& serializer) const override;
        bool Load(SR_UTILS_NS::IDeserializer& deserializer) override;

        void Finalize();

        void UseUniforms(const Pipeline* pPipeline);
        void UseSamplers(const Pipeline* pPipeline);

        SR_NODISCARD bool HasStage(SR_UTILS_NS::StringAtom stage) const noexcept;
        SR_NODISCARD MaterialShaderData& GetDefaultShaderData() noexcept { return m_defaultShader; }
        SR_NODISCARD const MaterialShaderData& GetDefaultShaderData() const noexcept { return m_defaultShader; }
        SR_NODISCARD MaterialShaderData* GetShaderData(SR_UTILS_NS::StringAtom id) noexcept;
        SR_NODISCARD const MaterialShaderData* GetShaderData(SR_UTILS_NS::StringAtom id) const noexcept;
        SR_NODISCARD const std::map<SR_UTILS_NS::StringAtom, std::string>& GetShaderDefines() const noexcept { return m_shaderDefines; }

        void SR_FASTCALL SetSampler(SR_UTILS_NS::StringAtom id, const SR_UTILS_NS::Path& path) noexcept;
        void SR_FASTCALL SetShader(const SR_UTILS_NS::Path& path);
        void SR_FASTCALL SetShader(SR_GTYPES_NS::Shader::Ptr pShader);
        void SR_FASTCALL SetData(SR_UTILS_NS::StringAtom id, const ShaderPropertyVariant& v, ShaderVarType type) noexcept;
        void SR_FASTCALL OnSamplerChanged(SR_GTYPES_NS::Texture::Ptr pOldTexture, SR_GTYPES_NS::Texture::Ptr pNewTexture) noexcept;

        void OnPropertyChanged(bool onlyUniforms);

    private:
        void OnShaderChanged();

    private:
        std::map<SR_GTYPES_NS::Shader::Ptr, std::pair<SR_UTILS_NS::Subscription, uint32_t>> m_shaderSubscriptions;
        std::map<SR_GTYPES_NS::Texture::Ptr, std::pair<SR_UTILS_NS::Subscription, uint32_t>> m_textureSubscriptions;
        MaterialShaderData m_defaultShader;

        /// @property
        std::map<SR_UTILS_NS::StringAtom, std::string> m_shaderDefines;

    };
}

#endif //SR_ENGINE_GRAPHICS_MATERIAL_DATA_H
