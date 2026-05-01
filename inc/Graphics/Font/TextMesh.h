//
// Created by Monika on 29.04.2026.
//

#ifndef SR_ENGINE_TEXT_MESH_H
#define SR_ENGINE_TEXT_MESH_H

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Font/FontAsset.h>
#include <Graphics/UI/Canvas.h>

#include <Utils/Types/UnicodeString.h>
#include <Utils/Common/Subscription.h>

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
        void OnTextDirty();

    private:
        int32_t m_VBO = SR_ID_INVALID;
        uint32_t m_instancesCount = 0;
        SR_UTILS_NS::Subscription m_onFontReloadedSubscription;

    private:
        /// @property @onChanged(OnTextDirty)
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: font)
        SR_UTILS_NS::ResourceRef<FontAsset> m_font;
        /// @property @onChanged(OnTextDirty)
        /// @customArg(text-box: enabled)
        std::string m_text;
        /// @property @range(0.1f, std::numeric_limits<float_t>::max())
        /// @onChanged(OnTextDirty)
        float_t m_fontSize = 16.f;

        /// @property @group(Features) @onChanged(OnTextDirty)
        bool m_kerning = true;

    };
}

#endif //SR_ENGINE_TEXT_MESH_H