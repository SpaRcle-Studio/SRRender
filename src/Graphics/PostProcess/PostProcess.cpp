//
// Created by Monika on 13.02.2026.
//

#include <Graphics/PostProcess/PostProcess.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Pass/PostProcessPass.h>
#include <Graphics/Material/BaseMaterial.h>

#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/World/Scene.h>
#include <Utils/Types/DataStorage.h>

#include <Codegen/PostProcess.generated.hpp>

namespace SR_GTYPES_NS {
    RenderScene* PostProcessComponent::GetRenderScene() const {
        if (m_renderScene) {
            return m_renderScene.Get();
        }

        if (auto&& pScene = TryGetScene()) {
            if (auto&& pRenderScene = pScene->GetDataStorage().GetValue<RenderScene::Ptr>()) {
                m_renderScene = pRenderScene;
            }
        }
        return m_renderScene.Get();
    }

    void PostProcessModuleVignette::Apply(BaseMaterial& material, PostProcessModule* pFrom, float_t progress) const {
        SR_TRACY_ZONE;
        material.SetFloat(SHADER_POST_PROCESS_VIGNETTE_INTENSITY, m_intensity);
    }

    void PostProcessModuleChromaticAberration::Apply(BaseMaterial& material, PostProcessModule* pFrom, float_t progress) const {
        SR_TRACY_ZONE;
        material.SetFloat(SHADER_POST_PROCESS_CHROMATIC_ABERRATION_INTENSITY, m_intensity);
    }

    void PostProcessComponent::Update(float_t dt) {
        SR_TRACY_ZONE;

        Super::Update(dt);

        auto&& pSettings = m_settings.GetResource();
        if (!pSettings) {
            return;
        }

        auto&& pCamera = m_camera ? m_camera.Get() : GetRenderScene()->GetMainCamera();
        if (!pCamera) {
            return;
        }

        auto&& pRenderTechnique = pCamera->GetRenderTechnique();
        PostProcessPass* pPostProcessPass = pRenderTechnique ? pRenderTechnique->FindPassAs<PostProcessPass>() : nullptr;
        const BaseMaterial::Ptr& pMaterial = pPostProcessPass ? pPostProcessPass->GetMaterial() : nullptr;

        if (!pMaterial || !pPostProcessPass) {
            return;
        }

        if (!pSettings->shaderPath.empty()) {
            pPostProcessPass->SetShader(pSettings->shaderPath);
        }

        for (auto&& pModule : pSettings->modules) {
            if (pModule) {
                pModule->Apply(*pMaterial, nullptr, 1.f);
            }
        }
    }
}
