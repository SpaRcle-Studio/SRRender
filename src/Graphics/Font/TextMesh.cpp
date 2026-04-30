//
// Created by Monika on 29.04.2026.
//

#include <Graphics/Font/TextMesh.h>
#include <Graphics/Types/Shader.h>

#include <Utils/ECS/TransformUtils.h>
#include <Utils/ECS/TransformRect.h>

#include <Codegen/TextMesh.generated.hpp>

namespace SR_GTYPES_NS {
    void TextMesh::Draw() {
        if (auto&& pFontAsset = m_font.GetResource()) {
            static std::vector<PositionedGlyph> glyphs;
            pFontAsset->BuildText(m_text, 1.f, glyphs);
        }

        DrawRenderObject(this, 4, m_virtualUBO, m_virtualDescriptor, m_dirtyMaterial, m_hasErrors);
    }

    void TextMesh::UseMaterial(Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void TextMesh::UseModelMatrix(Shader& shader) {
        Super::UseModelMatrix(shader);

        shader.SetInt(SHADER_SPRITE_MODE, static_cast<int32_t>(0));

        if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
            SR_MATH_NS::FRect layout = pTransformRect->GetLayoutRect();
            shader.SetVec4(SHADER_NDC_RECT, layout.vec4);
            shader.SetMat4(SHADER_MODEL_MATRIX, pTransformRect->GetMatrix());
        }
    }

    void TextMesh::UseSamplers(Shader& shader) {
        Super::UseSamplers(shader);
        static const SR_UTILS_NS::StringAtom id = "diffuse";
        //shader.SetSampler2D(id, 0);
    }
}