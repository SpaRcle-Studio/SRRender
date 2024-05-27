//
// Created by Monika on 19.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_BASE_MATERIAL_H
#define SR_ENGINE_GRAPHICS_BASE_MATERIAL_H

#include <Utils/Resources/IResource.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>
#include <Utils/Types/ObjectPool.h>

#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Material/MaterialType.h>
#include <Graphics/Material/MaterialProperty.h>

namespace SR_GTYPES_NS {
    class Mesh;
    class Texture;
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderContext;

    SR_ENUM_NS_CLASS_T(MaterialShader, uint16_t,
        Simple,
        Shadows, SSAO, HDAO, HBAO, VXAO, Bloom,
        SSAOShadows, HDAOhadows, HBAOShadows, VXAOShadows,
        SSAOShadowsBloom
    );

    class BaseMaterial {
    protected:
        using RenderContextPtr = SR_HTYPES_NS::SafePtr<RenderContext>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using TexturePtr = SR_GTYPES_NS::Texture*;

    protected:
        BaseMaterial();
        virtual ~BaseMaterial();

    public:
        SR_NODISCARD bool ContainsTexture(SR_GTYPES_NS::Texture* pTexture) const;
        SR_NODISCARD bool IsTransparent() const;
        SR_NODISCARD ShaderPtr GetShader() const { return m_shader; }
        SR_NODISCARD MaterialProperties& GetProperties() { return m_properties; }
        SR_NODISCARD MaterialProperty* GetProperty(const SR_UTILS_NS::StringAtom& id);
        SR_NODISCARD MaterialProperty* GetProperty(uint64_t hashId);

        SR_NODISCARD virtual MaterialType GetMaterialType() const noexcept = 0;

        SR_NODISCARD virtual uint32_t RegisterMesh(MeshPtr pMesh);
        virtual void UnregisterMesh(uint32_t* pId);

        void SetTexture(MaterialProperty* property, TexturePtr pTexture);
        virtual void SetShader(ShaderPtr pShader);

        void OnPropertyChanged();

        void Use();
        void UseSamplers();

        void FinalizeMaterial();

        virtual void AddMaterialDependency(SR_UTILS_NS::IResource* pResource);
        virtual void RemoveMaterialDependency(SR_UTILS_NS::IResource* pResource);

    protected:
        void InitMaterialProperties();

        virtual void InitContext();

    protected:
        SR_HTYPES_NS::ObjectPool<MeshPtr, uint32_t> m_meshes;
        ShaderPtr m_shader = nullptr;
        std::atomic<bool> m_dirtyShader = false;
        MaterialProperties m_properties;
        RenderContextPtr m_context;
        SR_UTILS_NS::Subscription m_shaderReloadDoneSubscription;

    private:
        bool m_isFinalized = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_BASE_MATERIAL_H
