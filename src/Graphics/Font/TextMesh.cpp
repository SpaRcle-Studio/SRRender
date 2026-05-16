//
// Created by Monika on 29.04.2026.
//

#include <Graphics/Font/TextMesh.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Font/TextMeshDetails.h>

#include <Utils/ECS/TransformUtils.h>
#include <Utils/ECS/TransformRect.h>
#include <Utils/Common/Vertices.h>
#include <Utils/Events/Broadcaster.h>

#include <Codegen/TextMesh.generated.hpp>

namespace SR_GTYPES_NS {
    const SR_UTILS_NS::VertexLayoutDescription& TextMesh::GetShaderVertexLayoutDescription() const noexcept {
        static const auto description = SR_UTILS_NS::VertexLayoutDescription()
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position0, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position1, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::UV1, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::UV2, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .SetInstanced(true);
        return description;
    }

    void TextMesh::Draw() {
        Calculate();
        GetPipeline()->SetDrawInstancesCount(m_instancesCount);
        DrawRenderObject(this, 6, m_virtualUBO, m_virtualDescriptor, m_dirtyMaterial, m_hasErrors);
        GetPipeline()->ResetDrawInstancesCount();
    }

    void TextMesh::UseMaterial(Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void TextMesh::UseModelMatrix(Shader& shader) {
        Super::UseModelMatrix(shader);

        shader.SetInt(SHADER_SPRITE_MODE, static_cast<int32_t>(0));

        if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
            shader.SetMat4(SHADER_MODEL_MATRIX, pTransformRect->GetMatrix());
            shader.SetVec4(SHADER_NDC_RECT, pTransformRect->GetLayoutRect().vec4);
        }
    }

    void TextMesh::UseSamplers(Shader& shader) {
        Super::UseSamplers(shader);
        static const SR_UTILS_NS::StringAtom id = "diffuse";
        //shader.SetSampler2D(id, 0);
    }

    bool TextMesh::Calculate() {
        SR_TRACY_ZONE;

        if (m_isCalculated) {
            return true;
        }

        auto&& pFontAsset = m_font.GetResource();
        if (!pFontAsset) {
            SR_ERROR("TextMesh::Calculate() : failed to load font asset!");
            m_hasErrors = true;
            return false;
        }

        if (!m_onFontReloadedSubscription.IsValid()) {
            m_onFontReloadedSubscription = SR_UTILS_NS::Broadcaster::Instance().Subscribe(SR_UTILS_NS::Events::EVENT_ON_FONT_RELOADED_ID, [this](auto&&) {
                m_isCalculated = false;
            });
        }

        static SR_THREAD_LOCAL std::vector<PositionedGlyph> glyphs;
        pFontAsset->BuildText(m_text, m_fontSize, glyphs);
        m_instancesCount = 0;

        if (glyphs.empty()) {
            return true;
        }

        static SR_THREAD_LOCAL SR_UTILS_NS::VertexDataBuffer buffer;
        buffer.SetLayout(GetShaderVertexLayoutDescription());
        buffer.Allocate(glyphs.size());

        SetVertexLayoutDescription(GetShaderVertexLayoutDescription());

        SR_MATH_NS::FVector2 layoutSize;
        if (auto&& pSceneObject = GetSceneObject().Get()) {
            if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(pSceneObject)) {
                layoutSize = pTransformRect->GetLayoutRect().Size();
            }
        }

        TextMeshDetails::GlyphPlacementContext context;
        context.fontSize = m_fontSize;
        context.kerning = m_kerning;
        context.horizontalAlignment = m_horizontalAlignment;
        context.verticalAlignment = m_verticalAlignment;
        context.layoutSize = layoutSize;

        static SR_THREAD_LOCAL std::vector<TextMeshDetails::GlyphPlacement> placements;
        if (!TextMeshDetails::CalculateGlyphPlacements(*pFontAsset, glyphs, context, placements)) {
            m_isCalculated = true;
            return true;
        }

        for (auto&& placement : placements) {
            glyphs[placement.glyphIndex].AddInstance(m_instancesCount++, placement.pos, placement.size, buffer);
        }

        m_VBO = GetPipeline()->AllocateVBO(m_VBO, m_instancesCount * buffer.GetLayout().GetStride(), buffer.GetRawData());

        if (m_VBO == SR_ID_INVALID) {
            SR_ERROR("TextMesh::Calculate() : failed to allocate VBO for text mesh!");
            m_hasErrors = true;
            ReRegisterRenderObject();
            return false;
        }

        m_isCalculated = true;
        return true;
    }

    void TextMesh::FreeVideoMemory() {
        Super::FreeVideoMemory();
        if (m_VBO != SR_ID_INVALID) {
            GetPipeline()->FreeVBO(&m_VBO);
        }
    }

    std::optional<int32_t> TextMesh::GetVBO() const {
        const_cast<TextMesh*>(this)->Calculate();
        return m_VBO != SR_ID_INVALID ? std::optional<int32_t>(m_VBO) : std::nullopt;
    }

    bool TextMesh::Bind() {
        Calculate();
        if (m_VBO == SR_ID_INVALID) {
            return false;
        }
        GetPipeline()->BindVBO(m_VBO);
        return true;
    }

    void TextMesh::OnMatrixDirty() {
        OnTextDirty();
        Super::OnMatrixDirty();
    }

    void TextMesh::OnTextDirty() {
        m_isCalculated = false;
        m_dirtyMaterial = true;
        if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->SetDirty();
        }
    }
}