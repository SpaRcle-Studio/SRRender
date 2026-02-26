//
// Created by Monika on 08.02.2026.
//

#include <Graphics/Render/RenderTechniquePreset.h>
#include <Graphics/Pass/CascadedShadowMapPass.h>
#include <Graphics/Pass/ColorBufferPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Pass/SwapchainPass.h>
#include <Graphics/Pass/PostProcessPass.h>
#include <Graphics/Pass/SkyboxPass.h>
#include <Graphics/Settings/RenderSettings.h>

#include <Codegen/RenderTechniquePreset.generated.hpp>

namespace SR_GRAPH_NS {
    FileRenderTechniquePresetResource::Ptr FileRenderTechniquePresetResource::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;
        return SR_UTILS_NS::Asset::Load<FileRenderTechniquePresetResource>(rawPath);
    }

    const RenderTechniqueData& FileRenderTechniquePresetResource::GetData(const Params& params) const noexcept {
        SR_TRACY_ZONE;

        if (!params.pRenderSettings) {
            SR_ERROR("FileRenderTechniquePresetResource::GetData() : graphics settings is nullptr!");
            return m_data;
        }

        m_data = RenderTechniqueData();
        m_data.name = "Preset";
        m_data.pass = new GroupPass();

        for (auto&& pIntegration : m_integrations) {
            pIntegration->Integrate(*this, params);
        }

        return m_data;
    }

    void RenderTechniquePresetIntegrationShadows::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        auto&& data = technique.GetInternalData();

        const Quality shadowsQuality = params.activeGraphicsSettings.shadowsQuality;
        if (shadowsQuality == Quality::None) {
            return;
        }
        const ShadowQualityPreset& shadowPreset = params.pRenderSettings->GetShadowQualityPreset(shadowsQuality);

        const bool instancing = params.instancing &&
            shadowPreset.instancing &&
            params.activeGraphicsSettings.instancing &&
            params.activeGraphicsSettings.shadowsInstancing;

        FrameBufferController::Ptr pShadowMapController = new FrameBufferController();
        pShadowMapController->SetName(shadowMapControllerName);
        pShadowMapController->SetSize(SR_MATH_NS::IVector2(static_cast<int32_t>(shadowPreset.shadowMapResolution)));
        pShadowMapController->SetColorFormats({});
        pShadowMapController->GetFeatures().colorShaderRead = false;
        pShadowMapController->GetFeatures().depthShaderRead = true;
        pShadowMapController->SetDepthAspect(ImageAspect::Depth);
        pShadowMapController->SetSamples(1);
        pShadowMapController->SetDynamicResizing(false);

        if (instancing) {
            pShadowMapController->SetLayersCount(1);
            pShadowMapController->SetArrayLayersCount(shadowPreset.cascadesCount);
        }
        else {
            pShadowMapController->SetLayersCount(shadowPreset.cascadesCount);
            pShadowMapController->SetArrayLayersCount(1);
        }

        data.frameBuffers.emplace_back(pShadowMapController);

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        pFrameBufferPass->SetCustomName(shadowMapControllerName);
        pFrameBufferPass->SetFrameBufferName(shadowMapControllerName);

        CascadedShadowMapPass::Ptr pShadowPass = new CascadedShadowMapPass();
        pShadowPass->SetCustomName(shadowMapControllerName);
        pShadowPass->SetCascadeCount(shadowPreset.cascadesCount);
        pShadowPass->SetSplitDepths(shadowPreset.split1, shadowPreset.split2, shadowPreset.split3);
        pShadowPass->SetMaxShadowDistance(shadowPreset.maxShadowDistance);
        pShadowPass->SetOneMeterUnit(shadowPreset.oneMeterUnit);
        pShadowPass->SetLightFrustumCount(0);
        pShadowPass->SetFrustumCulling(false);
        pShadowPass->AddShaderDefine(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);

        if (instancing) {
            pShadowPass->SetInstancing(true);
            pShadowPass->SetRenderLayers(1);
        }
        else {
            pShadowPass->SetInstancing(false);
            pShadowPass->SetRenderLayers(shadowPreset.cascadesCount);
        }

        for (auto&& pLayer : technique.GetLayers()) {
            if (auto&& pMeshLayer = pLayer.DynamicCast<RenderTechniqueLayerMesh>()) {
                if (!pMeshLayer->castShadows || (pMeshLayer->editorOnly && !params.editor)) {
                    continue;
                }
                for (auto&& layer : pMeshLayer->allowedLayers) {
                    pShadowPass->GetAllowedLayers().insert(layer);
                }
                for (auto&& layer : pMeshLayer->disallowedLayers) {
                    pShadowPass->GetDisallowedLayers().insert(layer);
                }
            }
        }

        for (const SR_UTILS_NS::StringAtom& layer : m_specialShadowLayers) {
            pShadowPass->GetAllowedLayers().insert(layer);
        }

        pFrameBufferPass->GetPasses().emplace_back(pShadowPass.StaticCast<BasePass>());
        data.pass.DynamicCast<GroupPass>()->GetPasses().emplace_back(pFrameBufferPass.StaticCast<BasePass>());
    }

    void RenderTechniquePresetIntegrationColorBuffer::Integrate(const Technique& technique, const Params& params) const {
       SR_TRACY_ZONE;

        auto&& data = technique.GetInternalData();
        const Quality colorBufferQuality = params.activeGraphicsSettings.colorBufferQuality;
        if (colorBufferQuality == Quality::None) {
            return;
        }
        const float_t colorBufferCoefficient = params.pRenderSettings->GetColorBufferResolutionCoefficient(colorBufferQuality);

        FrameBufferController::Ptr pColorBufferController = new FrameBufferController();
        pColorBufferController->SetName(colorBufferControllerName);
        pColorBufferController->SetPreScale(colorBufferCoefficient);
        pColorBufferController->SetSize(SR_MATH_NS::IVector2(1024));
        pColorBufferController->GetFeatures().colorTransferSrc = true;
        pColorBufferController->SetSamples(1);
        pColorBufferController->SetDynamicResizing(false);

        data.frameBuffers.emplace_back(pColorBufferController);

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        pFrameBufferPass->SetCustomName(colorBufferControllerName);
        pFrameBufferPass->SetFrameBufferName(colorBufferControllerName);
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));

        ColorBufferPass::Ptr pColorBufferPass = new ColorBufferPass();
        pColorBufferPass->SetRenderLayers(1);
        pColorBufferPass->AddShaderDefine(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        pColorBufferPass->SetColorMultiplier(colorMultiplier);

        for (auto&& pLayer : technique.GetLayers()) {
            if (auto&& pMeshLayer = pLayer.DynamicCast<RenderTechniqueLayerMesh>()) {
                if (!pMeshLayer->colorBuffer || (pMeshLayer->editorOnly && !params.editor)) {
                    continue;
                }
                for (auto&& layer : pMeshLayer->allowedLayers) {
                    pColorBufferPass->GetAllowedLayers().insert(layer);
                }
                for (auto&& layer : pMeshLayer->disallowedLayers) {
                    pColorBufferPass->GetDisallowedLayers().insert(layer);
                }
            }
        }

        pFrameBufferPass->GetPasses().emplace_back(pColorBufferPass.StaticCast<BasePass>());
        data.pass.DynamicCast<GroupPass>()->GetPasses().emplace_back(pFrameBufferPass.StaticCast<BasePass>());
    }

    void RenderTechniquePresetIntegrationMainView::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        GroupPass::Ptr pMainGroupPass;

        auto&& data = technique.GetInternalData();

        const bool useOffscreenRender = params.activeGraphicsSettings.postProcess;

        if (useOffscreenRender || params.editor) {
            const SR_UTILS_NS::StringAtom fboName = useOffscreenRender ? offscreenControllerName : params.sceneViewName;

            FrameBufferPass::Ptr pFrameBuffer = new FrameBufferPass();
            pFrameBuffer->SetCustomName(fboName);
            pFrameBuffer->SetFrameBufferName(fboName);

            for (uint8_t i = 0; i < mainRenderColorLayers; ++i) {
                pFrameBuffer->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
            }

            pMainGroupPass = pFrameBuffer.StaticCast<GroupPass>();

            std::vector<ImageFormat> colorFormats;
            colorFormats.reserve(mainRenderColorLayers);

            if (params.activeGraphicsSettings.hdr) {
                colorFormats.emplace_back(ImageFormat::B10G11R11_UFLOAT_PACK32);
            }
            else {
                colorFormats.emplace_back(ImageFormat::RGBA8_UNORM);
            }

            FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
            pFrameBufferController->SetName(fboName);
            pFrameBufferController->SetColorFormats(colorFormats);
            data.frameBuffers.emplace_back(pFrameBufferController);
        }
        else {
            SwapchainPass::Ptr pSwapchainPass = new SwapchainPass();
            pSwapchainPass->SetClearColor(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
            pSwapchainPass->SetCustomName(params.sceneViewName);
            pMainGroupPass = pSwapchainPass.StaticCast<GroupPass>();
        }

        auto&& pShadowIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationShadows>();

        data.pass.DynamicCast<GroupPass>()->GetPasses().emplace_back(pMainGroupPass.StaticCast<BasePass>());

        for (auto&& pLayer : technique.GetLayers()) {
            if ((pLayer->editorOnly && !params.editor)) {
                continue;
            }

            if (auto&& pMeshLayer = pLayer.DynamicCast<RenderTechniqueLayerMesh>()) {
                if (!pMeshLayer->mainRenderer) {
                    continue;
                }

                MeshDrawerPass::Ptr pMeshDrawerPass = new MeshDrawerPass();

                if (pMeshLayer->castShadows && params.activeGraphicsSettings.shadowsQuality != Quality::None) {
                    if (pShadowIntegration) {
                        pMeshDrawerPass->AddShaderDefine(SHADER_MACRO_SR_DEFINE_USE_CASCADED_SHADOW_MAP);
                        SamplerData shadowSampler;
                        shadowSampler.fboName = pShadowIntegration->shadowMapControllerName;
                        shadowSampler.id = pShadowIntegration->shaderVariableName;
                        shadowSampler.usageType = SamplerDataUsageType::FrameBufferDepth;
                        pMeshDrawerPass->GetSamplersData().AddSampler(shadowSampler);
                        pMeshDrawerPass->GetUniformsData().shared.useFromPass.insert(pShadowIntegration->shadowMapControllerName);
                    }
                }

                pMeshDrawerPass->SetFrustumCulling(pMeshLayer->frustumCulling);
                for (auto&& layer : pMeshLayer->allowedLayers) {
                    pMeshDrawerPass->GetAllowedLayers().insert(layer);
                }
                for (auto&& layer : pMeshLayer->disallowedLayers) {
                    pMeshDrawerPass->GetDisallowedLayers().insert(layer);
                }
                pMainGroupPass->GetPasses().emplace_back(pMeshDrawerPass.StaticCast<BasePass>());
            }
            else if (auto&& pSkyboxLayer = pLayer.DynamicCast<RenderTechniqueLayerSkybox>()) {
                SkyboxPass::Ptr pSkyboxPass = new SkyboxPass();
                pMainGroupPass->GetPasses().emplace_back(pSkyboxPass.StaticCast<BasePass>());
            }
            else if (auto&& pCustomPass = pLayer.DynamicCast<RenderTechniqueLayerCustomPass>(); pCustomPass && pCustomPass->pass) {
                BasePass::Ptr& pPass = pMainGroupPass->GetPasses().emplace_back();
                pPass = SR_UTILS_NS::Factory::Instance().Create<BasePass>(pCustomPass->pass->GetMeta()->GetFactoryName());
                pCustomPass->pass->CloneTo(*pPass);
            }
            else if (auto&& pClearDepthLayer = pLayer.DynamicCast<RenderTechniqueLayerClearDepth>()) {
                ClearDepthAttachmentPass::Ptr pClearDepthPass = new ClearDepthAttachmentPass();
                pMainGroupPass->GetPasses().emplace_back(pClearDepthPass.StaticCast<BasePass>());
            }
        }

        if (useOffscreenRender) {
            GroupPass::Ptr pPostProcessGroupPass;

            if (params.editor) {
                FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
                pFrameBufferController->SetName(params.sceneViewName);
                data.frameBuffers.emplace_back(pFrameBufferController);

                FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
                pFrameBufferPass->SetCustomName(params.sceneViewName);
                pFrameBufferPass->SetFrameBufferName(params.sceneViewName);
                pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));

                data.pass.DynamicCast<GroupPass>()->GetPasses().emplace_back(pFrameBufferPass.StaticCast<BasePass>());
                pPostProcessGroupPass = pFrameBufferPass.StaticCast<GroupPass>();
            }
            else {
                SwapchainPass::Ptr pSwapchainPass = new SwapchainPass();
                pSwapchainPass->SetClearColor(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
                pSwapchainPass->SetCustomName(params.sceneViewName);

                data.pass.DynamicCast<GroupPass>()->GetPasses().emplace_back(pSwapchainPass.StaticCast<BasePass>());
                pPostProcessGroupPass = pSwapchainPass.StaticCast<GroupPass>();
            }

            PostProcessPass::Ptr pPostProcessPass = new PostProcessPass();
            if (defaultPostProcessPass) {
                defaultPostProcessPass->CloneTo(*pPostProcessPass);
            }

            if (pShadowIntegration) {
                SamplerData shadowSampler;
                shadowSampler.fboName = pShadowIntegration->shadowMapControllerName;
                shadowSampler.id = pShadowIntegration->shaderVariableName;
                shadowSampler.usageType = SamplerDataUsageType::FrameBufferDepth;
                pPostProcessPass->GetSamplersData().AddSampler(shadowSampler);
            }

            pPostProcessGroupPass->GetPasses().emplace_back(pPostProcessPass.StaticCast<BasePass>());
        }
    }
}