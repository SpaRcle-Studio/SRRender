//
// Created by Monika on 13.02.2026.
//

#include <Graphics/Types/SkyboxComponent.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Material/UniqueMaterial.h>
#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Types/Skybox.h>

#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/World/Scene.h>
#include <Utils/Types/DataStorage.h>

#include <Codegen/SkyboxComponent.generated.hpp>

namespace SR_GTYPES_NS {
    SkyboxComponent::~SkyboxComponent() {
        if (m_skybox) {
            m_skybox->RemoveUsePoint();
        }
    }

    const SR_UTILS_NS::VertexLayoutDescription& SkyboxComponent::GetShaderVertexLayoutDescription() const noexcept {
        static SR_UTILS_NS::VertexLayoutDescription description = SR_UTILS_NS::VertexLayoutDescription()
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        ;
        return description;
    }

    void SkyboxComponent::SetParams(const SR_UTILS_NS::Path& skyboxPath, const SR_UTILS_NS::Path& shaderPath, bool isQuad) {
        SR_TRACY_ZONE;

        if (m_skyboxPath != skyboxPath || m_isQuad != isQuad) {
            m_isSkyboxDirty = true;
        }

        m_skyboxPath = skyboxPath;
        m_isQuad = isQuad;

        BaseMaterial::Ptr pMaterial = GetMaterial();
        if (!pMaterial) {
            pMaterial = new UniqueMaterial();
        }
        else if (pMaterial->GetMaterialType() == MaterialType::File) {
            pMaterial = pMaterial.StaticCast<SR_GRAPH_NS::FileMaterial>()->MakeUnique();
        }

        if (pMaterial) {
            pMaterial->SetShader(shaderPath);
            SetMaterial(pMaterial);
        }
    }

    void SkyboxComponent::Draw() {
        SR_TRACY_ZONE;

        m_isRendered = false;

        if (m_isSkyboxDirty) {
            if (m_skybox) {
                m_skybox->RemoveUsePoint();
                m_skybox = nullptr;
            }

            if (!(m_skybox = m_skyboxPath.empty() ? SR_GTYPES_NS::Skybox::CreateEmpty(m_isQuad) : CoreResLoader::Load<SR_GTYPES_NS::Skybox>(m_skyboxPath))) {
                SR_ERROR("SkyboxComponent::Draw() : failed to load skybox!\n\tPath: {}", m_skyboxPath);
                return;
            }
            else {
                m_skybox->AddUsePoint();
            }
            m_isSkyboxDirty = false;
        }

        if (!m_skybox) {
            return;
        }

        if (auto&& pShader = GetPipeline()->GetCurrentShader(); SRVerify(pShader)) {
            m_isRendered = m_skybox->Draw(pShader, m_dirtyMaterial, m_hasErrors, m_virtualUBO, m_virtualDescriptor);
        }
    }

    void SkyboxComponent::FreeVideoMemory() {
        if (auto&& pPipeline = TryGetPipeline()) {
            pPipeline->GetUBOManager().TryFreeUBO(&m_virtualUBO);
            pPipeline->GetDescriptorManager().TryFreeDescriptorSet(&m_virtualDescriptor);
        }
        Super::FreeVideoMemory();
    }
}
