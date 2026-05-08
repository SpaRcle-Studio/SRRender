//
// Created by Monika on 08.02.2026.
//

#include <Graphics/Render/RenderTechniquePreset.h>
#include <Graphics/Pass/CascadedShadowMapPass.h>
#include <Graphics/Pass/ColorBufferPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Pass/SwapchainPass.h>
#include <Graphics/Pass/PostProcessPass.h>
#include <Graphics/Pass/AutoExposurePass.h>
#include <Graphics/Pass/SSAOPass.h>
#include <Graphics/Pass/BlurPass.h>
#include <Graphics/Settings/RenderSettings.h>

#include <Utils/ECS/LayerManager.h>

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
        pShadowPass->SetLightFrustumCount(shadowPreset.frustumCount);
        pShadowPass->SetFrustumCulling(shadowPreset.frustumCount > 0);
        pShadowPass->AddShaderDefine(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);

        if (instancing) {
            pShadowPass->SetInstancing(true);
            pShadowPass->SetRenderLayers(1);
        }
        else {
            pShadowPass->SetInstancing(false);
            pShadowPass->SetRenderLayers(shadowPreset.cascadesCount);
        }

        auto&& layers = SR_UTILS_NS::LayerManager::Instance().GetLayersInfo();
        for (auto&& layerInfo : layers) {
            if (!params.IsLayerApplicable(layerInfo.name)) {
                continue;
            }

            if (!layerInfo.castShadows || (layerInfo.editorOnly && !params.editor)) {
                continue;
            }
            pShadowPass->GetAllowedLayers().insert(layerInfo.name);
        }

        //for (auto&& pLayer : technique.GetLayers()) {
        //    if (auto&& pMeshLayer = SR_UTILS_NS::DynamicPointerCast<RenderTechniqueLayerMesh>(pLayer)) {
        //        if (!pMeshLayer->castShadows || (pMeshLayer->editorOnly && !params.editor)) {
        //            continue;
        //        }
        //        for (auto&& layer : pMeshLayer->allowedLayers) {
        //            pShadowPass->GetAllowedLayers().insert(layer);
        //        }
        //        for (auto&& layer : pMeshLayer->disallowedLayers) {
        //            pShadowPass->GetDisallowedLayers().insert(layer);
        //        }
        //    }
        //}

        for (const SR_UTILS_NS::StringAtom& layer : m_specialShadowLayers) {
            pShadowPass->GetAllowedLayers().insert(layer);
        }

        pFrameBufferPass->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pShadowPass));
        SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass));
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

        auto&& layers = SR_UTILS_NS::LayerManager::Instance().GetLayersInfo();
        for (auto&& layerInfo : layers) {
            if (!params.IsLayerApplicable(layerInfo.name)) {
                continue;
            }
            if (!layerInfo.colorBuffer || (layerInfo.editorOnly && !params.editor)) {
                continue;
            }
            pColorBufferPass->GetAllowedLayers().insert(layerInfo.name);
        }

        //for (auto&& pLayer : technique.GetLayers()) {
        //    if (auto&& pMeshLayer = SR_UTILS_NS::DynamicPointerCast<RenderTechniqueLayerMesh>(pLayer)) {
        //        if (!pMeshLayer->colorBuffer || (pMeshLayer->editorOnly && !params.editor)) {
        //            continue;
        //        }
        //        for (auto&& layer : pMeshLayer->allowedLayers) {
        //            pColorBufferPass->GetAllowedLayers().insert(layer);
        //        }
        //        for (auto&& layer : pMeshLayer->disallowedLayers) {
        //            pColorBufferPass->GetDisallowedLayers().insert(layer);
        //        }
        //    }
        //}

        pFrameBufferPass->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pColorBufferPass));
        SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass));
    }

    void RenderTechniquePresetIntegrationMainView::AddLayers(GroupPass& groupPass,
        const Technique& technique,
        const Params& params,
        bool useOffscreenRender,
        bool isSceneView
    ) const {
        SR_TRACY_ZONE;

        auto&& pShadowIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationShadows>();

        MeshDrawerPass::Ptr pMeshDrawerPass = nullptr;
        SR_UTILS_NS::RenderLayerInfo lastMeshLayerInfo;
        auto&& layers = SR_UTILS_NS::LayerManager::Instance().GetLayersInfo();
        for (auto&& layerInfo : layers) {
            if (!params.IsLayerApplicable(layerInfo.name)) {
                continue;
            }

            if (layerInfo.editorOnly && !params.editor) {
                continue;
            }

            if (useOffscreenRender && layerInfo.noPostProcess != isSceneView) {
                continue;
            }

            if (layerInfo.clearDepth) {
                ClearDepthAttachmentPass::Ptr pClearDepthPass = new ClearDepthAttachmentPass();
                pClearDepthPass->SetCustomName("ClearDepth_" + layerInfo.name.ToStringRef());
                groupPass.AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pClearDepthPass));
            }

            if (layerInfo.mainRenderer && !layerInfo.isCustom) {
                if (!layerInfo.CompareParams(lastMeshLayerInfo)) {
                    pMeshDrawerPass = new MeshDrawerPass();
                    groupPass.AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pMeshDrawerPass));

                    if (layerInfo.applyShadows && params.activeGraphicsSettings.shadowsQuality != Quality::None) {
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
                    pMeshDrawerPass->SetFrustumCulling(layerInfo.frustumCulling);
                }

                pMeshDrawerPass->GetAllowedLayers().insert(layerInfo.name);
            }

            if (layerInfo.isCustom) {
                for (auto&& pLayer : technique.GetCustomLayers()) {
                    if (pLayer->GetLayerName() == layerInfo.name) {
                        if (auto&& pCustomPass = SR_UTILS_NS::DynamicPointerCast<RenderTechniqueLayerCustomPass>(pLayer); pCustomPass && pCustomPass->pass) {
                            BasePass::Ptr pPass = SR_UTILS_NS::Factory::Instance().Create<BasePass>(pCustomPass->pass->GetMeta()->GetFactoryName());
                            pCustomPass->pass->CloneTo(*pPass);
                            groupPass.AddPass(pPass);
                        }
                        break;
                    }
                }
            }

            lastMeshLayerInfo = layerInfo;
        }
    }

    void RenderTechniquePresetIntegrationMainView::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        GroupPass::Ptr pMainGroupPass;

        auto&& data = technique.GetInternalData();

        const bool useOffscreenRender = params.activeGraphicsSettings.postProcess;

        if (useOffscreenRender || params.editor || params.offscreen) {
            const SR_UTILS_NS::StringAtom fboName = useOffscreenRender ? offscreenControllerName : params.sceneViewName;

            FrameBufferPass::Ptr pFrameBuffer = new FrameBufferPass();
            pFrameBuffer->SetCustomName(fboName);
            pFrameBuffer->SetFrameBufferName(fboName);

            for (uint8_t i = 0; i < mainRenderColorLayers; ++i) {
                pFrameBuffer->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
            }

            pMainGroupPass = SR_UTILS_NS::StaticPointerCast<GroupPass>(pFrameBuffer);

            std::vector<ImageFormat> colorFormats;
            colorFormats.reserve(mainRenderColorLayers);

            if (params.activeGraphicsSettings.hdr) {
                colorFormats.emplace_back(ImageFormat::B10G11R11_UFLOAT_PACK32);
            }
            else {
                colorFormats.emplace_back(ImageFormat::RGBA8_UNORM);
            }

            if (mainRenderColorLayers > 1) { /// depth layer
                colorFormats.emplace_back(ImageFormat::R32_SFLOAT);
            }

            if (mainRenderColorLayers > 2) { /// position layer
                colorFormats.emplace_back(ImageFormat::RGBA16_SFLOAT);
            }

            if (mainRenderColorLayers > 3) { /// normal layer
                colorFormats.emplace_back(ImageFormat::RGBA8_UNORM);
            }

            FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
            pFrameBufferController->SetName(fboName);
            pFrameBufferController->SetColorFormats(colorFormats);

            if (params.pCameraParams) {
                if (params.pCameraParams->screenSize) {
                    pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
                    pFrameBufferController->SetDynamicResizing(false);
                }
                if (params.pCameraParams->screenScale) {
                    pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value());
                }
                if (params.pCameraParams->multisampling && !params.pCameraParams->multisampling.value()) {
                    pFrameBufferController->SetSamples(1);
                }
            }

            data.frameBuffers.emplace_back(pFrameBufferController);
        }
        else {
            SwapchainPass::Ptr pSwapchainPass = new SwapchainPass();
            pSwapchainPass->SetClearColor(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
            pSwapchainPass->SetCustomName(params.sceneViewName);
            pMainGroupPass = SR_UTILS_NS::StaticPointerCast<GroupPass>(pSwapchainPass);
        }

        SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pMainGroupPass));

        if (useOffscreenRender) {
            AddLayers(*pMainGroupPass, technique, params, useOffscreenRender, false);

            GroupPass::Ptr pPostProcessGroupPass;

            if (params.editor || params.offscreen) {
                FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
                pFrameBufferController->SetName(params.sceneViewName);
                if (params.pCameraParams) {
                    if (params.pCameraParams->screenSize) {
                        pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
                        pFrameBufferController->SetDynamicResizing(false);
                    }
                    if (params.pCameraParams->screenScale) {
                        pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value());
                    }
                    if (params.pCameraParams->multisampling && !params.pCameraParams->multisampling.value()) {
                        pFrameBufferController->SetSamples(1);
                    }
                }
                data.frameBuffers.emplace_back(pFrameBufferController);

                FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
                pFrameBufferPass->SetCustomName(params.sceneViewName);
                pFrameBufferPass->SetFrameBufferName(params.sceneViewName);
                pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));

                SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass));
                pPostProcessGroupPass = SR_UTILS_NS::StaticPointerCast<GroupPass>(pFrameBufferPass);
            }
            else {
                SwapchainPass::Ptr pSwapchainPass = new SwapchainPass();
                pSwapchainPass->SetClearColor(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
                pSwapchainPass->SetCustomName(params.sceneViewName);

                SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pSwapchainPass));
                pPostProcessGroupPass = SR_UTILS_NS::StaticPointerCast<GroupPass>(pSwapchainPass);
            }

            PostProcessPass::Ptr pPostProcessPass = new PostProcessPass();
            if (defaultPostProcessPass) {
                defaultPostProcessPass->CloneTo(*pPostProcessPass);
            }

            if (auto&& pShadowIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationShadows>()) {
                SamplerData shadowSampler;
                shadowSampler.fboName = pShadowIntegration->shadowMapControllerName;
                shadowSampler.id = pShadowIntegration->shaderVariableName;
                shadowSampler.usageType = SamplerDataUsageType::FrameBufferDepth;
                pPostProcessPass->GetSamplersData().AddSampler(shadowSampler);
            }

            pPostProcessGroupPass->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pPostProcessPass));
            AddLayers(*pPostProcessGroupPass, technique, params, useOffscreenRender, true);
        }
        else {
            AddLayers(*pMainGroupPass, technique, params, useOffscreenRender, true);
        }
    }

    void RenderTechniquePresetIntegrationAutoExposure::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        if (!params.activeGraphicsSettings.autoExposure || !params.activeGraphicsSettings.hdr || !params.activeGraphicsSettings.postProcess) {
            return;
        }

        auto&& pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationAutoExposure::Integrate() : failed to find main view integration for auto exposure integration!");
            return;
        }

        auto&& pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(technique.GetInternalData().pass);

        const int32_t index = pMainGroup->IndexOfPass(pMainViewIntegration->offscreenControllerName);
        if (index >= 0) {
            AutoExposurePass::Ptr pAutoExposurePass = new AutoExposurePass();
            SamplerData shadowSampler;
            shadowSampler.fboName = pMainViewIntegration->offscreenControllerName;
            shadowSampler.id = "hdrTexture";
            shadowSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            pAutoExposurePass->GetSamplersData().AddSampler(shadowSampler);
            pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pAutoExposurePass), index + 1);

            if (auto&& pPostProcessPass = pMainGroup->FindPassAs<PostProcessPass>(PostProcessPass::GetClassStaticName())) {
                pPostProcessPass->AddSSBOUsageFromPass(AutoExposurePass::GetClassStaticName());
            }
            else {
                SR_ERROR("RenderTechniquePresetIntegrationAutoExposure::Integrate() : failed to find post process pass for auto exposure integration! \n\tController name: {}", pMainViewIntegration->offscreenControllerName);
            }
        }
        else {
            SRHalt("RenderTechniquePresetIntegrationAutoExposure::Integrate() : failed to find offscreen controller pass for auto exposure integration! \n\tController name: {}", pMainViewIntegration->offscreenControllerName);
        }
    }

    void RenderTechniquePresetIntegrationSSAO::Integrate(const Technique& technique, const Params& params) const {

        GroupPass::Ptr pMainGroupPass;

        auto&& data = technique.GetInternalData();

        if (!params.activeGraphicsSettings.SSAO || !params.activeGraphicsSettings.postProcess) {
            return;
        }

        FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
        pFrameBufferController->SetName(m_SSAOname);
        if (params.pCameraParams) {
            if (params.pCameraParams->screenSize) {
                pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
                pFrameBufferController->SetDynamicResizing(false);
            }
            if (params.pCameraParams->screenScale) {
                pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value());
            }
            if (params.pCameraParams->multisampling && !params.pCameraParams->multisampling.value()) {
                pFrameBufferController->SetSamples(1);
            }
        }

        data.frameBuffers.emplace_back(pFrameBufferController);

        auto&& pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationAutoExposure::Integrate() : failed to find main view integration for auto exposure integration!");
            return;
        }

        auto&& pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(technique.GetInternalData().pass);

        const int32_t index = pMainGroup->IndexOfPass(pMainViewIntegration->offscreenControllerName);
        if (index >= 0) {
            SSAOPass::Ptr pSSAO = new SSAOPass();
            SamplerData ssaoSampler;

            ssaoSampler.fboName = pMainViewIntegration->offscreenControllerName;
            ssaoSampler.index = 1;
            ssaoSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            ssaoSampler.id = "Depth";
            pSSAO->GetSamplersData().AddSampler(ssaoSampler);

            ssaoSampler.fboName = pMainViewIntegration->offscreenControllerName;
            ssaoSampler.index = 2;
            ssaoSampler.id = "Position";
            pSSAO->GetSamplersData().AddSampler(ssaoSampler);

            ssaoSampler.fboName = pMainViewIntegration->offscreenControllerName;
            ssaoSampler.index = 3;
            ssaoSampler.id = "Normal";
            pSSAO->GetSamplersData().AddSampler(ssaoSampler);

            FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
            pFrameBufferPass->SetCustomName(m_SSAOname);
            pFrameBufferPass->SetFrameBufferName(m_SSAOname);
            pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));

            pFrameBufferPass->AddPass(pSSAO.StaticCast<BasePass>());
            pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), index + 1);

            //SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass));
            //pPostProcessGroupPass = SR_UTILS_NS::StaticPointerCast<GroupPass>(pFrameBufferPass);
        }
        else {
            SRHalt("RenderTechniquePresetIntegrationSSAOPass::Integrate() : failed to find offscreen controller pass for auto exposure integration! \n\tController name: {}", pMainViewIntegration->offscreenControllerName);
        }

        //BlurPass
        pFrameBufferController = new FrameBufferController();
        pFrameBufferController->SetName(m_SSAOBlurname);

        if (params.pCameraParams) {
            if (params.pCameraParams->screenSize) {
                pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
                pFrameBufferController->SetDynamicResizing(false);
            }
            if (params.pCameraParams->screenScale) {
                pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value());
            }
            if (params.pCameraParams->multisampling && !params.pCameraParams->multisampling.value()) {
                pFrameBufferController->SetSamples(1);
            }
        }

        data.frameBuffers.emplace_back(pFrameBufferController);

        pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationAutoExposure::Integrate() : failed to find main view integration for auto exposure integration!");
            return;
        }

        pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(technique.GetInternalData().pass);

        BlurPass::Ptr pBlur = new BlurPass();
        SamplerData blurSampler;

        blurSampler.fboName = m_SSAOname;
        blurSampler.index = 0;
        blurSampler.usageType = SamplerDataUsageType::FrameBufferColor;
        blurSampler.id = "image";
        pBlur->GetSamplersData().AddSampler(blurSampler);

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        pFrameBufferPass->SetCustomName(m_SSAOBlurname);
        pFrameBufferPass->SetFrameBufferName(m_SSAOBlurname);
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));

        pFrameBufferPass->AddPass(pBlur.StaticCast<BasePass>());
        pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), index + 2);

        //SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass)->AddPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass));
        //pPostProcessGroupPass = SR_UTILS_NS::StaticPointerCast<GroupPass>(pFrameBufferPass);
        //if () {
        //    SRHalt("RenderTechniquePresetIntegrationSSAOPass::Integrate() : failed to find offscreen controller pass for auto exposure integration! \n\tController name: {}", pMainViewIntegration->offscreenControllerName);
        //}

        if (auto&& pPostProcessPass = pMainGroup->FindPassAs<PostProcessPass>(PostProcessPass::GetClassStaticName())) {

            SamplerData finalSampler;

            finalSampler.fboName = m_SSAOBlurname;
            finalSampler.index = 0;
            finalSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            finalSampler.id = "SSAO";
            pPostProcessPass->GetSamplersData().AddSampler(finalSampler);
        }
        else {
            SR_ERROR("RenderTechniquePresetIntegrationAutoExposure::Integrate() : failed to find post process pass for auto exposure integration! \n\tController name: {}", pMainViewIntegration->offscreenControllerName);
        }
    }
}