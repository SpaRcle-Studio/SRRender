//
// Created by Nikita on 17.11.2020.
//

#ifndef SR_ENGINE_GRAPHICS_MATERIAL_H
#define SR_ENGINE_GRAPHICS_MATERIAL_H

#include <Utils/Resources/IResource.h>
#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>
#include <Utils/Types/ObjectPool.h>

#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GRAPH_NS {
    class RenderContext;
}

namespace SR_GTYPES_NS {
    class Mesh;
    class Mesh3D;
    class Texture;
    class Shader;

    SR_ENUM_NS_CLASS_T(MaterialShader, uint16_t,
        Simple,
        Shadows, SSAO, HDAO, HBAO, VXAO, Bloom,
        SSAOShadows, HDAOhadows, HBAOShadows, VXAOShadows,
        SSAOShadowsBloom
    );

    class Material : public SR_UTILS_NS::IResource {
        using Super = SR_UTILS_NS::IResource;
        using RenderContextPtr = SR_HTYPES_NS::SafePtr<RenderContext>;
    private:
        Material();
        ~Material() override;

    public:
        static Material* Load(SR_UTILS_NS::Path rawPath);

    public:
        SR_NODISCARD Super* CopyResource(Super* destination) const override;

        SR_NODISCARD bool ContainsTexture(SR_GTYPES_NS::Texture* pTexture) const;
        SR_NODISCARD bool IsTransparent() const;
        SR_NODISCARD Shader* GetShader() const { return m_shader; }
        SR_NODISCARD MaterialProperties& GetProperties() { return m_properties; }
        SR_NODISCARD MaterialProperty* GetProperty(const SR_UTILS_NS::StringAtom& id);
        SR_NODISCARD MaterialProperty* GetProperty(uint64_t hashId);
        SR_NODISCARD SR_UTILS_NS::Path GetAssociatedPath() const override;

        void SetTexture(MaterialProperty* property, Texture* pTexture);

        void OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) override;

        SR_NODISCARD uint32_t RegisterMesh(Mesh* mesh);
        void UnregisterMesh(uint32_t* pId);

        void SetShader(Shader* shader);

        void OnPropertyChanged();

        void Use();
        void UseSamplers();
        bool Destroy() override;

    private:
        bool Reload() override;
        bool Load() override;
        bool Unload() override;

        void InitContext();

        bool LoadProperties(const SR_XML_NS::Node& propertiesNode);

    private:
        SR_HTYPES_NS::ObjectPool<Mesh*, uint32_t> m_meshes;
        SR_GTYPES_NS::Shader* m_shader = nullptr;
        std::atomic<bool> m_dirtyShader = false;
        MaterialProperties m_properties;
        RenderContextPtr m_context;

    };
}

#endif //SR_ENGINE_GRAPHICS_MATERIAL_H
