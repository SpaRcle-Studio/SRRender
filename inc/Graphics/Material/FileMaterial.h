//
// Created by Monika on 19.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_FILE_MATERIAL_H
#define SR_ENGINE_GRAPHICS_FILE_MATERIAL_H

#include <Graphics/Material/BaseMaterial.h>

#include <Utils/Resources/Asset.h>

namespace SR_GRAPH_NS {
    /// @extension(mat)
    class FileMaterialResource final : public SR_UTILS_NS::Asset {
        SR_CLASS()
        using Super = SR_UTILS_NS::Asset;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FileMaterialResource>;

    public:
        FileMaterialResource();

    public:
        SR_NODISCARD static bool CreateTemplateMaterial(const SR_UTILS_NS::Path& path);

    public:
        SR_NODISCARD const SR_GRAPH_NS::MaterialData::Ptr& GetData() const noexcept { return m_data; }
        void SetData(const SR_GRAPH_NS::MaterialData::Ptr& pData) noexcept { m_data = pData; }

    private:
        /// @property
        SR_GRAPH_NS::MaterialData::Ptr m_data;

    };

    class FileMaterial final : public BaseMaterial {
        using Super = BaseMaterial;
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FileMaterial>;

        ~FileMaterial() override;

    public:
        SR_NODISCARD static BaseMaterial::Ptr Load(const SR_UTILS_NS::Path& rawPath);
        SR_NODISCARD static BaseMaterial::Ptr LoadAsUnique(const SR_UTILS_NS::Path& rawPath);
        SR_NODISCARD BaseMaterial::Ptr MakeUnique() const;

    public:
        SR_NODISCARD MaterialType GetMaterialType() const noexcept override { return MaterialType::File; }
        SR_NODISCARD const MaterialData::Ptr& GetMaterialData() const noexcept override;

        SR_NODISCARD SR_UTILS_NS::Path GetMaterialPath() const noexcept { return m_pResource ? m_pResource->GetResourcePath() : SR_UTILS_NS::Path(); }
        void SetMaterialPath(const SR_UTILS_NS::Path& path) noexcept;

    private:
        void Init();

    private:
        /// @virtualProperty(materialPath) @getter(GetMaterialPath) @setter(SetMaterialPath)
        /// @customArgs(pick: enabled, filter name: Material, relative: resources)
        /// @customArg(filter value: mat)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(materialData) @getter(GetMaterialData) @dontSave @readOnly @noHeader
        SR_VIRTUAL_PROPERTY

        FileMaterialResource::Ptr m_pResource = nullptr;

        SR_UTILS_NS::Subscription m_reloadBeginSubscription;
        SR_UTILS_NS::Subscription m_reloadDoneSubscription;

    };
}

#endif //SR_ENGINE_GRAPHICS_FILE_MATERIAL_H
