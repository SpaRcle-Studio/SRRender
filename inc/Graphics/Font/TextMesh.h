//
// Created by Monika on 29.04.2026.
//

#ifndef SR_ENGINE_TEXT_MESH_H
#define SR_ENGINE_TEXT_MESH_H

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Font/FontAsset.h>
#include <Graphics/UI/Canvas.h>

#include <Utils/Types/UnicodeString.h>

namespace SR_GTYPES_NS {
    class TextMesh : public UIRenderComponent, public UI::IFindCanvasOwner {
        SR_CLASS()
        using Super = UIRenderComponent;
    public:
        const SR_UTILS_NS::VertexLayoutDescription& GetShaderVertexLayoutDescription() const noexcept override;

        void Draw() override;
        void UseSamplers(Shader& shader) override;
        void UseMaterial(Shader& shader) override;
        void UseModelMatrix(Shader& shader) override;
        void FreeVideoMemory() override;

        SR_NODISCARD std::optional<int32_t> GetVBO() const override;
        bool Bind() override;

    private:
        bool Calculate();

    private:
        int32_t m_VBO = SR_ID_INVALID;
        uint32_t m_instancesCount = 0;
        bool m_dirty = true;

    private:
        /// @property
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: font)
        SR_UTILS_NS::ResourceRef<FontAsset> m_font;
        /// @property
        /// @customArg(text-box: enabled)
        std::string m_text;
        /// @property @range(0.1f, std::numeric_limits<float_t>::max())
        float_t m_fontSize = 16.f;

    };
}

#endif //SR_ENGINE_TEXT_MESH_H