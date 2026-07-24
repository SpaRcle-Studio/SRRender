//
// Created by Monika on 19.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_BASE_MATERIAL_H
#define SR_ENGINE_GRAPHICS_BASE_MATERIAL_H

#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/Material/MaterialType.h>
#include <Graphics/Material/MaterialData.h>

#include <Utils/Resources/IResource.h>
#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>
#include <Utils/Types/ObjectPool.h>
#include <Utils/Types/SafePointer.h>
#include <Utils/Serialization/Serializable.h>

namespace SR_GTYPES_NS {
    class IRenderComponent;
    class Texture;
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderContext;

    /// @abstract
    class BaseMaterial : public SR_UTILS_NS::Serializable, public SR_UTILS_NS::NonCopyable, public SR_HTYPES_NS::SharedPtr<BaseMaterial> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<BaseMaterial>;

    protected:
        using RenderContextPtr = SR_HTYPES_NS::SafePtr<RenderContext>;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
        using TexturePtr = SR_GTYPES_NS::Texture*;

    public:
        BaseMaterial();
        ~BaseMaterial() override;

    public:
        void SR_FASTCALL SetVec4(SR_UTILS_NS::StringAtom id, const SR_MATH_NS::FVector4& v) noexcept;
        void SR_FASTCALL SetColor(SR_UTILS_NS::StringAtom id, const SR_MATH_NS::FColor& v) noexcept;
        void SR_FASTCALL SetFloat(SR_UTILS_NS::StringAtom id, float_t v) noexcept;
        void SR_FASTCALL SetBool(SR_UTILS_NS::StringAtom id, bool v) noexcept;
        void SR_FASTCALL SetTexture(SR_UTILS_NS::StringAtom id, const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Texture>& pTexture) noexcept;

        SR_NODISCARD bool IsValid() const;
        SR_NODISCARD RenderContextPtr GetContext() const { return m_context; }
        SR_NODISCARD virtual const MaterialData::Ptr& GetMaterialData() const noexcept;
        SR_NODISCARD SR_GTYPES_NS::Shader* GetDefaultShader() const noexcept;
        SR_NODISCARD SR_GTYPES_NS::Shader* GetShader(const SR_SRSL_NS::ShaderParams& params) const noexcept;

        SR_NODISCARD virtual MaterialType GetMaterialType() const noexcept { return MaterialType::None; }

        SR_NODISCARD virtual uint32_t Register(SR_GTYPES_NS::IRenderComponent* pObject);
        virtual void Unregister(uint32_t* pId);

        virtual void SetShader(ShaderPtr pShader);
        void SetShader(const SR_UTILS_NS::Path& path);

        void OnPropertyChanged(bool onlyUniforms) const;
        void OnShaderChanged();

        void Use(SR_GTYPES_NS::Shader& shader);
        void UseSamplers(SR_GTYPES_NS::Shader& shader);

    protected:
        virtual void InitContext() const;
        void InitMaterialDataSubscriptions();
        void DeInitMaterialDataSubscriptions();

    protected:
        mutable RenderContextPtr m_context;

        SR_HTYPES_NS::ObjectPool<SR_GTYPES_NS::IRenderComponent*, uint32_t> m_registerObjects;

        mutable SR_UTILS_NS::Subscription m_shaderChangedSubscription;
        mutable SR_UTILS_NS::Subscription m_propertyChangedSubscription;

        mutable SR_UTILS_NS::Map<SR_UTILS_NS::SRHashType, SR_GTYPES_NS::Shader::Ptr> m_variants;
        mutable SR_UTILS_NS::Map<SR_UTILS_NS::SRHashType, SR_SRSL_NS::ShaderParams> m_hashRedirect;

    };
}

#endif //SR_ENGINE_GRAPHICS_BASE_MATERIAL_H
