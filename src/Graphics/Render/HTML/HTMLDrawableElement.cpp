//
// Created by Monika on 02.10.2024.
//

#include <Graphics/Render/HTML/HTMLDrawableElement.h>

namespace SR_GRAPH_NS {
    /*HTMLDrawableElement::~HTMLDrawableElement() {
        SetShader(nullptr);
        SetTexture(nullptr);

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_virtualUBO != SR_ID_INVALID && !uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("HTMLDrawableElement::~HTMLDrawableElement() : failed to free virtual uniform buffer object!");
        }

        if (m_virtualDescriptor != SR_ID_INVALID) {
            descriptorManager.FreeDescriptorSet(&m_virtualDescriptor);
        }
    }

    void HTMLDrawableElement::SetShader(SR_GTYPES_NS::Shader::Ptr pShader) {
        m_dirtyMaterial |= m_pShader != pShader;
        m_pShader = pShader;
    }

    void HTMLDrawableElement::SetTexture(SR_GTYPES_NS::Texture::Ptr pTexture) {
        if (m_pTexture == pTexture) {
            return;
        }
        if (m_pTexture) {
            m_pTexture->RemoveUsePoint();
        }
        if ((m_pTexture = pTexture)) {
            pTexture->AddUsePoint();
        }
        m_dirtyMaterial = true;
    }

    HTMLRendererUpdateResult HTMLDrawableElement::Update(const HTMLRendererUpdateContext& context) {
        HTMLRendererUpdateResult result;

        if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("HTMLDrawableElement::Update() : virtual UBO is invalid!");
            return result;
        }

        m_pipeline->SetCurrentShader(m_pShader);

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        if (uboManager.BindNoDublicateUBO(m_virtualUBO) != Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
            return result;
        }

        auto&& pNode = m_pPage->GetNodeById(m_nodeId);
        auto&& style = pNode->GetStyle();

        SR_MATH_NS::FVector2 position = context.offset;
        position.x = position.x + context.size.x;
        position.y = -position.y - context.size.y;
        position = SR_MATH_NS::FVector2(-1, 1) + (position / context.resolution);

        m_pShader->SetVec2("position"_atom_hash, position);
        m_pShader->SetVec2("size"_atom_hash, context.size / context.resolution);

        if (style.backgroundColor.colorType == SR_UTILS_NS::Web::CSSColor::ColorType::RGBA) {
            m_pShader->SetVec4("backgroundColor"_atom_hash, style.backgroundColor.color.ToFColor());
            //m_pShader->SetVec4("backgroundColor"_atom_hash, SR_MATH_NS::FColor::Cyan());
        }

        SR_MAYBE_UNUSED_VAR m_pShader->Flush();

        return result;
    }

    const SR_UTILS_NS::Web::CSSStyle& HTMLDrawableElement::GetStyle() const {
        return m_pPage->GetNodeById(m_nodeId)->GetStyle();
    }

    void HTMLDrawableElement::Draw() {
        SR_TRACY_ZONE;

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                return;
            }

            m_virtualDescriptor = descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        uboManager.BindUBO(m_virtualUBO);

        const auto result = descriptorManager.Bind(m_virtualDescriptor);

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
                if (m_pTexture) {
                    m_pShader->SetSampler2D("image"_atom, m_pTexture);
                }
                m_pShader->FlushSamplers();
                descriptorManager.Flush();
            }
            m_pipeline->GetCurrentShader()->FlushConstants();
        }

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            m_pipeline->Draw(4);
        }

        m_dirtyMaterial = false;
    }*/
}