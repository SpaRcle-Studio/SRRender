//
// Created by Monika on 13.02.2026.
//

#include <Graphics/Types/SkyboxComponent.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Pass/SkyboxPass.h>

#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/World/Scene.h>
#include <Utils/Types/DataStorage.h>

#include <Codegen/SkyboxComponent.generated.hpp>

namespace SR_GTYPES_NS {
    void SkyboxComponent::OnDisable() {
        Super::OnDisable();

        if (auto&& pSkyboxPass = FindSkyboxPass()) {
            pSkyboxPass->SetParams("", "", false);
        }
    }

    void SkyboxComponent::OnDetached() {
        Super::OnDetached();

        if (auto&& pSkyboxPass = FindSkyboxPass()) {
            pSkyboxPass->SetParams("", "", false);
        }
    }

    void SkyboxComponent::SetParams(const SR_UTILS_NS::Path& skyboxPath, const SR_UTILS_NS::Path& shaderPath, bool isQuad) {
        SR_TRACY_ZONE;

        m_skyboxPath = skyboxPath;
        m_shaderPath = shaderPath;
        m_isQuad = isQuad;
    }

    SkyboxPass* SkyboxComponent::FindSkyboxPass() const {
        SR_TRACY_ZONE;

        Camera::Ptr pCamera = m_camera.Get();

        if (!pCamera) {
            if (auto&& pRenderScene = GetRenderScene()) {
                pCamera = pRenderScene->GetMainCamera();
            }
            if (!pCamera) {
                return nullptr;
            }
        }

        if (auto&& pRenderTechnique = pCamera->GetRenderTechnique()) {
            return pRenderTechnique->FindPassAs<SkyboxPass>();
        }

        return nullptr;
    }

    RenderScene* SkyboxComponent::GetRenderScene() const {
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

    void SkyboxComponent::Update(float_t dt) {
        SR_TRACY_ZONE;

        Super::Update(dt);

        if (auto&& pSkyboxPass = FindSkyboxPass()) {
            pSkyboxPass->SetParams(m_skyboxPath, m_shaderPath, m_isQuad);
        }
    }
}
