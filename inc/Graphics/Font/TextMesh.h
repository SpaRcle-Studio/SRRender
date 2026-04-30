//
// Created by Monika on 29.04.2026.
//

#ifndef SR_ENGINE_TEXT_MESH_H
#define SR_ENGINE_TEXT_MESH_H

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Font/FontAsset.h>

#include <Utils/Types/UnicodeString.h>

namespace SR_GTYPES_NS {
    class TextMesh : public UIRenderComponent {
        SR_CLASS()
        using Super = UIRenderComponent;
    public:
        void Draw() override;
        void UseSamplers(Shader& shader) override;
        void UseMaterial(Shader& shader) override;
        void UseModelMatrix(Shader& shader) override;

    public:
        /// @property
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: font)
        SR_UTILS_NS::ResourceRef<FontAsset> m_font;
        /// @property
        /// @customArg(text-box: enabled)
        std::string m_text;

    };
}

#endif //SR_ENGINE_TEXT_MESH_H