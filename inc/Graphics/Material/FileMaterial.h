//
// Created by Monika on 19.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_FILE_MATERIAL_H
#define SR_ENGINE_GRAPHICS_FILE_MATERIAL_H

#include <Graphics/Material/BaseMaterial.h>

namespace SR_GRAPH_NS {
    class FileMaterial : public BaseMaterial, public SR_UTILS_NS::IResource {
    public:
        using Ptr = FileMaterial*;

    private:
        FileMaterial();

    public:
        ~FileMaterial() override;

    public:
        static FileMaterial::Ptr Load(SR_UTILS_NS::Path rawPath);

    public:
        SR_NODISCARD MaterialType GetMaterialType() const noexcept override { return MaterialType::File; }

        SR_NODISCARD uint32_t RegisterMesh(MeshPtr pMesh) override;
        void UnregisterMesh(uint32_t* pId) override;

        SR_NODISCARD SR_UTILS_NS::IResource::Ptr CopyResource(SR_UTILS_NS::IResource::Ptr pDestination) const override;

        SR_NODISCARD SR_UTILS_NS::Path GetAssociatedPath() const override;

        void OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) override;

    protected:
        void AddMaterialDependency(SR_UTILS_NS::IResource::Ptr pResource) override;
        void RemoveMaterialDependency(SR_UTILS_NS::IResource::Ptr pResource) override;

        void InitContext() override;

        bool LoadProperties(const SR_XML_NS::Node& propertiesNode);

    private:
        void DeleteResource() override;
        bool Reload() override;
        bool Load() override;
        bool Unload() override;

    };
}

#endif //SR_ENGINE_GRAPHICS_FILE_MATERIAL_H
