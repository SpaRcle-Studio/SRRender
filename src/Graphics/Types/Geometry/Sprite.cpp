//
// Created by Monika on 30.07.2022.
//

#include <Graphics/Types/Geometry/Sprite.h>
#include <Graphics/Types/Material.h>
#include <Graphics/Types/Uniforms.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Utils/MeshUtils.h>

namespace SR_GTYPES_NS {
    Sprite::Sprite()
        : Super(MeshType::Sprite)
    { }

    std::string Sprite::GetMeshIdentifier() const {
        static const std::string id = "SpriteFromMemory";
        return id;
    }

    bool Sprite::InitializeEntity() noexcept {
        m_properties.AddStandardProperty("Sliced", &m_sliced);

        m_properties.AddStandardProperty("Texture border", &m_textureBorder)
            .SetDrag(0.01f)
            .SetResetValue(0.15f)
            .SetActiveCondition([this]() { return m_sliced; })
            .SetWidth(90.f);

        m_properties.AddStandardProperty("Window border", &m_windowBorder)
            .SetDrag(0.01f)
            .SetResetValue(0.15f)
            .SetActiveCondition([this]() { return m_sliced; })
            .SetWidth(90.f);

        return Super::InitializeEntity();
    }

    bool Sprite::Calculate() {
        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (!CalculateVBO<Vertices::VertexType::UIVertex, Vertices::UIVertex>([]() { return SR_SPRITE_VERTICES; })) {
            return false;
        }

        return IndexedMesh::Calculate();
    }

    void Sprite::Draw() {
        SR_TRACY_ZONE;

        auto&& pShader = GetRenderContext()->GetCurrentShader();

        if (!pShader || !IsActive()) {
            return;
        }

        if ((!IsCalculated() && !Calculate()) || m_hasErrors) {
            return;
        }

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_dirtyMaterial = false;

            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            m_material->UseSamplers();
        }

        m_pipeline->BindVBO(m_VBO);
        m_pipeline->BindIBO(m_IBO);
        m_uboManager.BindUBO(m_virtualUBO);

        if (m_descriptorManager.Bind(m_virtualDescriptor) != DescriptorManager::BindResult::Failed) {
            m_pipeline->DrawIndices(m_countIndices);
        }
    }

    std::vector<uint32_t> Sprite::GetIndices() const {
        return SR_SPRITE_INDICES;
    }

    void Sprite::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void Sprite::UseModelMatrix() {
        if (auto&& pShader = GetRenderContext()->GetCurrentShader()) {
            pShader->SetMat4(SHADER_MODEL_MATRIX, m_modelMatrix);

            if (m_sliced) {
                pShader->SetVec2(SHADER_SLICED_TEXTURE_BORDER, m_textureBorder);
                pShader->SetVec2(SHADER_SLICED_WINDOW_BORDER, m_windowBorder);
            }
        }
        else {
            SRHaltOnce("Shader is nullptr!");
        }
        Super::UseModelMatrix();
    }

    void Sprite::OnPriorityChanged() {
        ReRegisterMesh();
        Component::OnPriorityChanged();
    }
}