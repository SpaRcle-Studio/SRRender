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
#include <Graphics/Pass/BloomPass.h>
#include <Graphics/Pass/SSRPass.h>
#include <Graphics/Pass/SceneDistancePass.h>
#include <Graphics/Pass/VolumetricCloudsPass.h>
#include <Graphics/Pass/VolumetricFogPass.h>
#include <Graphics/Settings/RenderSettings.h>

#include <Utils/ECS/LayerManager.h>

#include <Codegen/RenderTechniquePreset.generated.hpp>

namespace SR_GRAPH_NS {
    FileRenderTechniquePresetResource::Ptr FileRenderTechniquePresetResource::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;
        return SR_UTILS_NS::Asset::Load<FileRenderTechniquePresetResource>(rawPath);
    }

    /// G-buffer слои Offscreen: 1 - покрытие/metallic/roughness, 2 - позиция, 3 - нормаль (см. Common/gbuffer.srsl)
    static void AddGBufferSamplers(SamplersPassData& samplers, SR_UTILS_NS::StringAtom offscreenName, bool withNormal) {
        auto&& add = [&](uint64_t index, SR_UTILS_NS::StringAtom id) {
            if (samplers.HasSampler(id)) {
                return;
            }
            SamplerData sampler;
            sampler.fboName = offscreenName;
            sampler.index = index;
            sampler.usageType = SamplerDataUsageType::FrameBufferColor;
            sampler.id = id;
            samplers.AddSampler(sampler);
        };

        add(1, "coverageMap");
        add(2, "positionMap");
        if (withNormal) {
            add(3, "normalMap");
        }
    }

    static const SR_UTILS_NS::StringAtom SCENE_DISTANCE_CONTROLLER_NAME = "SceneDistance";

    /// Общий для проходов пониженного разрешения буфер дистанции до сцены (половинное разрешение,
    /// см. Common/scene-distance.srsl). Создается один раз сразу после offscreen.
    /// Возвращает индекс, после которого вставлять проход-потребитель.
    static int32_t EnsureSceneDistancePass(GroupPass& mainGroup, RenderTechniqueData& data,
        const RenderTechniqueLoadParams& params, SR_UTILS_NS::StringAtom offscreenName
    ) {
        const int32_t existingIndex = mainGroup.IndexOfPass(SCENE_DISTANCE_CONTROLLER_NAME);
        if (existingIndex >= 0) {
            return existingIndex;
        }

        const int32_t offscreenIndex = mainGroup.IndexOfPass(offscreenName);
        if (offscreenIndex < 0) {
            return SR_ID_INVALID;
        }

        constexpr float_t preScale = 0.5f;

        FrameBufferController::Ptr pController = new FrameBufferController();
        pController->SetName(SCENE_DISTANCE_CONTROLLER_NAME);
        pController->SetColorFormats({ ImageFormat::R32_SFLOAT });
        pController->SetDepthEnabled(false);
        pController->SetSamples(1);

        if (params.pCameraParams && params.pCameraParams->screenSize) {
            pController->SetSize(params.pCameraParams->screenSize.value());
            pController->SetDynamicResizing(false);
        }
        if (params.pCameraParams && params.pCameraParams->screenScale) {
            pController->SetPreScale(params.pCameraParams->screenScale.value() * preScale);
        }
        else {
            pController->SetPreScale(SR_MATH_NS::FVector2(preScale));
        }

        data.frameBuffers.emplace_back(pController);

        SceneDistancePass::Ptr pPass = new SceneDistancePass();
        pPass->SetCustomName("SceneDistancePass");
        AddGBufferSamplers(pPass->GetSamplersData(), offscreenName, false);

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        pFrameBufferPass->SetCustomName(SCENE_DISTANCE_CONTROLLER_NAME);
        pFrameBufferPass->SetFrameBufferName(SCENE_DISTANCE_CONTROLLER_NAME);
        pFrameBufferPass->GetFrameBufferPassData().SetClearDepth(SR_UTILS_NS::Optional<float_t>());
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(1e6f, 0.f, 0.f, 1.f));
        pFrameBufferPass->AddPass(pPass.StaticCast<BasePass>());

        mainGroup.InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), static_cast<uint32_t>(offscreenIndex + 1));
        return offscreenIndex + 1;
    }

    static void AddSceneDistanceSampler(SamplersPassData& samplers) {
        SamplerData sampler;
        sampler.fboName = SCENE_DISTANCE_CONTROLLER_NAME;
        sampler.index = 0;
        sampler.usageType = SamplerDataUsageType::FrameBufferColor;
        sampler.id = "sceneDistanceMap";
        samplers.AddSampler(sampler);
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

            SR_UTILS_NS::Vector<ImageFormat> colorFormats;
            colorFormats.reserve(mainRenderColorLayers);

            if (params.activeGraphicsSettings.hdr) {
                colorFormats.emplace_back(ImageFormat::B10G11R11_UFLOAT_PACK32);
            }
            else {
                colorFormats.emplace_back(ImageFormat::RGBA8_UNORM);
            }

            if (mainRenderColorLayers > 1) { /// coverage layer (r - покрытие, g - metallic, b - roughness), см. Common/gbuffer.srsl
                colorFormats.emplace_back(ImageFormat::RGBA8_UNORM);
            }

            if (mainRenderColorLayers > 2) { /// position layer
                colorFormats.emplace_back(ImageFormat::RGBA16_SFLOAT);
            }

            if (mainRenderColorLayers > 3) { /// normal layer (rgb - нормаль с normal map * покрытие)
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

        if ((params.activeGraphicsSettings.SSAO == Quality::None) || !params.activeGraphicsSettings.postProcess) {
            return;
        }

        FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
        pFrameBufferController->SetName(m_SSAOname);

        const Quality SSAOQuality = params.activeGraphicsSettings.SSAO;
        //if (SSAOQuality == Quality::None) {
        //    return;
        //}
        SSAOPreset SSAOpreset = params.pRenderSettings->GetSSAOResolutionPreset(SSAOQuality);

        if (params.pCameraParams) {
            if (params.pCameraParams->screenSize) {
                pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
                pFrameBufferController->SetDynamicResizing(false);
            }
            if (params.pCameraParams->screenScale) {
                pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value() * SSAOpreset.preScale);
            }
            else{
                pFrameBufferController->SetPreScale(SSAOpreset.preScale);
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
            ssaoSampler.id = "Coverage";
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
                pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value() * SSAOpreset.preScale);
            }
            else{
                pFrameBufferController->SetPreScale(SSAOpreset.preScale);
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

    static void AddPostProcessPositionSampler(PostProcessPass& postProcessPass, SR_UTILS_NS::StringAtom offscreenName) {
        /// позиции полного разрешения - для апсемпла с учетом глубины (без лесенки на границах объектов)
        AddGBufferSamplers(postProcessPass.GetSamplersData(), offscreenName, false);
    }

    void RenderTechniquePresetIntegrationVolumetricClouds::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        const Quality quality = params.activeGraphicsSettings.volumetricClouds;
        if (quality == Quality::None || !params.activeGraphicsSettings.postProcess) {
            return;
        }

        auto&& pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationVolumetricClouds::Integrate() : failed to find main view integration!");
            return;
        }

        if (pMainViewIntegration->mainRenderColorLayers < 3) {
            SR_ERROR("RenderTechniquePresetIntegrationVolumetricClouds::Integrate() : main view has no position layer!");
            return;
        }

        float_t preScale = 0.5f;
        switch (quality) {
            case Quality::Low: preScale = 0.25f; break;
            case Quality::Medium: preScale = 0.35f; break;
            case Quality::High: preScale = 0.5f; break;
            default: preScale = 0.75f; break;
        }

        auto&& data = technique.GetInternalData();

        FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
        pFrameBufferController->SetName(cloudsControllerName);
        /// 0 - облака, 1 - дистанция, для которой посчитан пиксель (для апсемпла с учетом глубины)
        pFrameBufferController->SetColorFormats({ ImageFormat::RGBA16_SFLOAT, ImageFormat::R32_SFLOAT });
        pFrameBufferController->SetSamples(1);

        if (params.pCameraParams && params.pCameraParams->screenSize) {
            pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
            pFrameBufferController->SetDynamicResizing(false);
        }
        if (params.pCameraParams && params.pCameraParams->screenScale) {
            pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value() * preScale);
        }
        else {
            pFrameBufferController->SetPreScale(preScale);
        }

        data.frameBuffers.emplace_back(pFrameBufferController);

        auto&& pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass);

        const int32_t index = EnsureSceneDistancePass(*pMainGroup, data, params, pMainViewIntegration->offscreenControllerName);
        if (index < 0) {
            SRHalt("RenderTechniquePresetIntegrationVolumetricClouds::Integrate() : failed to find offscreen controller pass! Controller name: {}", pMainViewIntegration->offscreenControllerName);
            return;
        }

        VolumetricCloudsPass::Ptr pCloudsPass = new VolumetricCloudsPass();
        pCloudsPass->SetCustomName("VolumetricCloudsPass");

        AddSceneDistanceSampler(pCloudsPass->GetSamplersData());

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        pFrameBufferPass->SetCustomName(cloudsControllerName);
        pFrameBufferPass->SetFrameBufferName(cloudsControllerName);
        /// пропускание = 1, если облаков нет
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(1e6f, 0.f, 0.f, 1.f));
        pFrameBufferPass->AddPass(pCloudsPass.StaticCast<BasePass>());

        pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), index + 1);

        if (auto&& pPostProcessPass = pMainGroup->FindPassAs<PostProcessPass>(PostProcessPass::GetClassStaticName())) {
            SamplerData cloudsSampler;
            cloudsSampler.fboName = cloudsControllerName;
            cloudsSampler.index = 0;
            cloudsSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            cloudsSampler.id = shaderVariableName;
            pPostProcessPass->GetSamplersData().AddSampler(cloudsSampler);

            cloudsSampler.index = 1;
            cloudsSampler.id = "cloudsDepthMap";
            pPostProcessPass->GetSamplersData().AddSampler(cloudsSampler);
            AddPostProcessPositionSampler(*pPostProcessPass, pMainViewIntegration->offscreenControllerName);
        }
        else {
            SR_ERROR("RenderTechniquePresetIntegrationVolumetricClouds::Integrate() : failed to find post process pass!");
        }
    }

    void RenderTechniquePresetIntegrationVolumetricFog::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        const Quality quality = params.activeGraphicsSettings.volumetricFog;
        if (quality == Quality::None || !params.activeGraphicsSettings.postProcess) {
            return;
        }

        auto&& pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationVolumetricFog::Integrate() : failed to find main view integration!");
            return;
        }

        if (pMainViewIntegration->mainRenderColorLayers < 3) {
            SR_ERROR("RenderTechniquePresetIntegrationVolumetricFog::Integrate() : main view has no position layer!");
            return;
        }

        float_t preScale = 0.5f;
        switch (quality) {
            case Quality::Low: preScale = 0.25f; break;
            case Quality::Medium: preScale = 0.35f; break;
            case Quality::High: preScale = 0.5f; break;
            default: preScale = 0.75f; break;
        }

        auto&& data = technique.GetInternalData();

        FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
        pFrameBufferController->SetName(fogControllerName);
        /// 0 - туман, 1 - дистанция, для которой посчитан пиксель (для апсемпла с учетом глубины)
        pFrameBufferController->SetColorFormats({ ImageFormat::RGBA16_SFLOAT, ImageFormat::R32_SFLOAT });
        pFrameBufferController->SetSamples(1);

        if (params.pCameraParams && params.pCameraParams->screenSize) {
            pFrameBufferController->SetSize(params.pCameraParams->screenSize.value());
            pFrameBufferController->SetDynamicResizing(false);
        }
        if (params.pCameraParams && params.pCameraParams->screenScale) {
            pFrameBufferController->SetPreScale(params.pCameraParams->screenScale.value() * preScale);
        }
        else {
            pFrameBufferController->SetPreScale(preScale);
        }

        data.frameBuffers.emplace_back(pFrameBufferController);

        auto&& pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass);

        const int32_t index = EnsureSceneDistancePass(*pMainGroup, data, params, pMainViewIntegration->offscreenControllerName);
        if (index < 0) {
            SRHalt("RenderTechniquePresetIntegrationVolumetricFog::Integrate() : failed to find offscreen controller pass! Controller name: {}", pMainViewIntegration->offscreenControllerName);
            return;
        }

        VolumetricFogPass::Ptr pFogPass = new VolumetricFogPass();
        pFogPass->SetCustomName("VolumetricFogPass");

        AddSceneDistanceSampler(pFogPass->GetSamplersData());

        /// объемные лучи используют каскадные тени
        auto&& pShadowIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationShadows>();
        if (pShadowIntegration && params.activeGraphicsSettings.shadowsQuality != Quality::None) {
            SamplerData shadowSampler;
            shadowSampler.fboName = pShadowIntegration->shadowMapControllerName;
            shadowSampler.id = pShadowIntegration->shaderVariableName;
            shadowSampler.usageType = SamplerDataUsageType::FrameBufferDepth;
            pFogPass->GetSamplersData().AddSampler(shadowSampler);
            pFogPass->SetShadowPassName(pShadowIntegration->shadowMapControllerName);
        }

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        pFrameBufferPass->SetCustomName(fogControllerName);
        pFrameBufferPass->SetFrameBufferName(fogControllerName);
        /// пропускание = 1, если тумана нет
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
        pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(1e6f, 0.f, 0.f, 1.f));
        pFrameBufferPass->AddPass(pFogPass.StaticCast<BasePass>());

        pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), index + 1);

        if (auto&& pPostProcessPass = pMainGroup->FindPassAs<PostProcessPass>(PostProcessPass::GetClassStaticName())) {
            SamplerData fogSampler;
            fogSampler.fboName = fogControllerName;
            fogSampler.index = 0;
            fogSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            fogSampler.id = shaderVariableName;
            pPostProcessPass->GetSamplersData().AddSampler(fogSampler);

            fogSampler.index = 1;
            fogSampler.id = "fogDepthMap";
            pPostProcessPass->GetSamplersData().AddSampler(fogSampler);

            AddPostProcessPositionSampler(*pPostProcessPass, pMainViewIntegration->offscreenControllerName);
        }
        else {
            SR_ERROR("RenderTechniquePresetIntegrationVolumetricFog::Integrate() : failed to find post process pass!");
        }
    }

    void RenderTechniquePresetIntegrationBloom::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        const Quality quality = params.activeGraphicsSettings.bloom;
        if (quality == Quality::None || !params.activeGraphicsSettings.postProcess || !params.activeGraphicsSettings.hdr) {
            return;
        }

        auto&& pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationBloom::Integrate() : failed to find main view integration!");
            return;
        }

        auto&& data = technique.GetInternalData();
        auto&& pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass);

        const int32_t offscreenIndex = pMainGroup->IndexOfPass(pMainViewIntegration->offscreenControllerName);
        if (offscreenIndex < 0) {
            SRHalt("RenderTechniquePresetIntegrationBloom::Integrate() : failed to find offscreen controller pass! Controller name: {}", pMainViewIntegration->offscreenControllerName);
            return;
        }

        const BloomPreset preset = params.pRenderSettings->GetBloomPreset(quality);
        const uint8_t mipCount = SR_MATH_NS::Clamp<uint8_t>(preset.mipCount, 2, 10);
        const ImageFormat format = preset.highPrecision ? ImageFormat::RGBA16_SFLOAT : ImageFormat::B10G11R11_UFLOAT_PACK32;

        /// порог считается по экспонированной яркости, поэтому цепочка идет после расчета экспозиции
        const int32_t autoExposureIndex = pMainGroup->IndexOfPass(AutoExposurePass::GetClassStaticName());
        const bool useAutoExposure = autoExposureIndex >= 0;
        int32_t insertIndex = SR_MATH_NS::Max(offscreenIndex, autoExposureIndex) + 1;

        auto&& addLevel = [&](SR_UTILS_NS::StringAtom name, float_t scale, const BloomPass::Ptr& pPass) {
            FrameBufferController::Ptr pController = new FrameBufferController();
            pController->SetName(name);
            pController->SetColorFormats({ format });
            pController->SetDepthEnabled(false);
            pController->SetSamples(1);

            if (params.pCameraParams && params.pCameraParams->screenSize) {
                pController->SetSize(params.pCameraParams->screenSize.value());
                pController->SetDynamicResizing(false);
            }
            if (params.pCameraParams && params.pCameraParams->screenScale) {
                pController->SetPreScale(params.pCameraParams->screenScale.value() * scale);
            }
            else {
                pController->SetPreScale(SR_MATH_NS::FVector2(scale));
            }

            data.frameBuffers.emplace_back(pController);

            pPass->highQuality = preset.highQualityFilter;
            pPass->threshold = threshold;
            pPass->knee = knee;
            pPass->clampMax = clampMax;
            pPass->scatter = scatter;
            pPass->SetCustomName(SR_UTILS_NS::StringAtom(name.ToStringRef() + "Pass"));

            FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
            pFrameBufferPass->SetCustomName(name);
            pFrameBufferPass->SetFrameBufferName(name);
            pFrameBufferPass->GetFrameBufferPassData().SetClearDepth(SR_UTILS_NS::Optional<float_t>());
            pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 1.f));
            pFrameBufferPass->AddPass(pPass.StaticCast<BasePass>());

            pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), static_cast<uint32_t>(insertIndex));
            ++insertIndex;
        };

        auto&& addSampler = [](BloomPass& pass, SR_UTILS_NS::StringAtom fboName, SR_UTILS_NS::StringAtom id) {
            SamplerData sampler;
            sampler.fboName = fboName;
            sampler.index = 0;
            sampler.usageType = SamplerDataUsageType::FrameBufferColor;
            sampler.id = id;
            pass.GetSamplersData().AddSampler(sampler);
        };

        auto&& mipName = [this](uint32_t i) { return SR_UTILS_NS::StringAtom(SR_FORMAT("{}Mip{}", bloomControllerName.ToStringRef(), i)); };
        auto&& upName = [this](uint32_t i) { return SR_UTILS_NS::StringAtom(SR_FORMAT("{}Up{}", bloomControllerName.ToStringRef(), i)); };
        auto&& levelScale = [&preset](uint32_t i) { return preset.preScale / static_cast<float_t>(1u << i); };

        /// вниз: mip0 = prefilter(offscreen), mipN = downsample(mipN-1)
        for (uint32_t i = 0; i < mipCount; ++i) {
            BloomPass::Ptr pPass = new BloomPass();
            if (i == 0) {
                pPass->stage = BloomPassStage::Prefilter;
                addSampler(*pPass, pMainViewIntegration->offscreenControllerName, "bloomSource");
                if (useAutoExposure) {
                    pPass->AddSSBOUsageFromPass(AutoExposurePass::GetClassStaticName());
                }
            }
            else {
                pPass->stage = BloomPassStage::Downsample;
                addSampler(*pPass, mipName(i - 1), "bloomSource");
            }
            addLevel(mipName(i), levelScale(i), pPass);
        }

        /// вверх: upN = mix(mipN, upsample(upN+1), scatter), для предпоследнего уровня источник - самый нижний mip
        for (int32_t i = static_cast<int32_t>(mipCount) - 2; i >= 0; --i) {
            const auto level = static_cast<uint32_t>(i);

            BloomPass::Ptr pPass = new BloomPass();
            pPass->stage = BloomPassStage::Upsample;
            addSampler(*pPass, level + 2 == mipCount ? mipName(level + 1) : upName(level + 1), "bloomLow");
            addSampler(*pPass, mipName(level), "bloomHigh");
            addLevel(upName(level), levelScale(level), pPass);
        }

        if (auto&& pPostProcessPass = pMainGroup->FindPassAs<PostProcessPass>(PostProcessPass::GetClassStaticName())) {
            SamplerData bloomSampler;
            bloomSampler.fboName = upName(0);
            bloomSampler.index = 0;
            bloomSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            bloomSampler.id = shaderVariableName;
            pPostProcessPass->GetSamplersData().AddSampler(bloomSampler);
        }
        else {
            SR_ERROR("RenderTechniquePresetIntegrationBloom::Integrate() : failed to find post process pass!");
        }
    }

    void RenderTechniquePresetIntegrationSSR::Integrate(const Technique& technique, const Params& params) const {
        SR_TRACY_ZONE;

        const Quality quality = params.activeGraphicsSettings.SSR;
        if (quality == Quality::None || !params.activeGraphicsSettings.postProcess) {
            return;
        }

        auto&& pMainViewIntegration = technique.FindIntegration<RenderTechniquePresetIntegrationMainView>();
        if (!pMainViewIntegration) {
            SR_ERROR("RenderTechniquePresetIntegrationSSR::Integrate() : failed to find main view integration!");
            return;
        }

        if (pMainViewIntegration->mainRenderColorLayers < 4) {
            SR_ERROR("RenderTechniquePresetIntegrationSSR::Integrate() : main view has no normal layer!");
            return;
        }

        auto&& data = technique.GetInternalData();
        auto&& pMainGroup = SR_UTILS_NS::DynamicPointerCast<GroupPass>(data.pass);

        const int32_t offscreenIndex = pMainGroup->IndexOfPass(pMainViewIntegration->offscreenControllerName);
        if (offscreenIndex < 0) {
            SRHalt("RenderTechniquePresetIntegrationSSR::Integrate() : failed to find offscreen controller pass! Controller name: {}", pMainViewIntegration->offscreenControllerName);
            return;
        }

        const SSRPreset preset = params.pRenderSettings->GetSSRPreset(quality);
        const SR_UTILS_NS::StringAtom offscreenName = pMainViewIntegration->offscreenControllerName;
        int32_t insertIndex = EnsureSceneDistancePass(*pMainGroup, data, params, offscreenName) + 1;

        auto&& addSampler = [](SSRPass& pass, SR_UTILS_NS::StringAtom fboName, uint64_t index, SR_UTILS_NS::StringAtom id) {
            SamplerData sampler;
            sampler.fboName = fboName;
            sampler.index = index;
            sampler.usageType = SamplerDataUsageType::FrameBufferColor;
            sampler.id = id;
            pass.GetSamplersData().AddSampler(sampler);
        };

        auto&& addStage = [&](SR_UTILS_NS::StringAtom name, const SSRPass::Ptr& pPass) {
            FrameBufferController::Ptr pController = new FrameBufferController();
            pController->SetName(name);
            /// 0 - отражения, 1 - дистанция, для которой посчитан пиксель (для апсемпла с учетом глубины)
            pController->SetColorFormats({ ImageFormat::RGBA16_SFLOAT, ImageFormat::R32_SFLOAT });
            pController->SetDepthEnabled(false);
            pController->SetSamples(1);

            if (params.pCameraParams && params.pCameraParams->screenSize) {
                pController->SetSize(params.pCameraParams->screenSize.value());
                pController->SetDynamicResizing(false);
            }
            if (params.pCameraParams && params.pCameraParams->screenScale) {
                pController->SetPreScale(params.pCameraParams->screenScale.value() * preset.preScale);
            }
            else {
                pController->SetPreScale(SR_MATH_NS::FVector2(preset.preScale));
            }

            data.frameBuffers.emplace_back(pController);

            pPass->maxSteps = preset.maxSteps;
            pPass->refineSteps = preset.refineSteps;
            pPass->maxDistance = preset.maxDistance;
            pPass->thickness = thickness;
            pPass->maxRoughness = maxRoughness;
            pPass->edgeFade = edgeFade;
            pPass->blurRadius = blurRadius;
            pPass->SetCustomName(SR_UTILS_NS::StringAtom(name.ToStringRef() + "Pass"));

            AddGBufferSamplers(pPass->GetSamplersData(), offscreenName, true);

            FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
            pFrameBufferPass->SetCustomName(name);
            pFrameBufferPass->SetFrameBufferName(name);
            pFrameBufferPass->GetFrameBufferPassData().SetClearDepth(SR_UTILS_NS::Optional<float_t>());
            pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(0.f, 0.f, 0.f, 0.f));
            pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(SR_MATH_NS::FColor(1e6f, 0.f, 0.f, 1.f));
            pFrameBufferPass->AddPass(pPass.StaticCast<BasePass>());

            pMainGroup->InsertPass(SR_UTILS_NS::StaticPointerCast<BasePass>(pFrameBufferPass), static_cast<uint32_t>(insertIndex));
            ++insertIndex;
        };

        SSRPass::Ptr pTracePass = new SSRPass();
        pTracePass->stage = SSRPassStage::Trace;
        addSampler(*pTracePass, offscreenName, 0, "sceneColor");
        AddSceneDistanceSampler(pTracePass->GetSamplersData());
        addStage(traceControllerName, pTracePass);

        SR_UTILS_NS::StringAtom resultName = traceControllerName;
        if (preset.resolve) {
            SSRPass::Ptr pResolvePass = new SSRPass();
            pResolvePass->stage = SSRPassStage::Resolve;
            addSampler(*pResolvePass, traceControllerName, 0, "ssrTrace");
            addSampler(*pResolvePass, traceControllerName, 1, "ssrTraceDepth");
            addStage(resolveControllerName, pResolvePass);
            resultName = resolveControllerName;
        }

        if (auto&& pPostProcessPass = pMainGroup->FindPassAs<PostProcessPass>(PostProcessPass::GetClassStaticName())) {
            SamplerData ssrSampler;
            ssrSampler.fboName = resultName;
            ssrSampler.index = 0;
            ssrSampler.usageType = SamplerDataUsageType::FrameBufferColor;
            ssrSampler.id = shaderVariableName;
            pPostProcessPass->GetSamplersData().AddSampler(ssrSampler);

            ssrSampler.index = 1;
            ssrSampler.id = "ssrDepthMap";
            pPostProcessPass->GetSamplersData().AddSampler(ssrSampler);

            AddGBufferSamplers(pPostProcessPass->GetSamplersData(), offscreenName, true);
        }
        else {
            SR_ERROR("RenderTechniquePresetIntegrationSSR::Integrate() : failed to find post process pass!");
        }
    }
}
