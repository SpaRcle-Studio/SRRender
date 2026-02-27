//
// Created by Monika on 27.02.2026.
//

#include <Graphics/Pass/AutoExposurePass.h>
#include <Graphics/Pass/Data/SamplersPassData.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Memory/UBOManager.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Types/Time.h>
#include <Utils/Common/Features.h>

#include <Codegen/AutoExposurePass.generated.hpp>

namespace SR_GRAPH_NS {
    namespace Details {
        constexpr uint32_t EXPOSURE_SSBO_SIZE = 2u * sizeof(float);
        constexpr float INITIAL_EXPOSURE = 1.0f;
        static const SR_UTILS_NS::StringAtom ResolutionAtom = "resolution";
        static const SR_UTILS_NS::StringAtom ElementCountAtom = "elementCount";
        static const SR_UTILS_NS::StringAtom ReductionInAtom = "reductionIn";
        static const SR_UTILS_NS::StringAtom ReductionOutAtom = "reductionOut";
        static const SR_UTILS_NS::StringAtom KeyValueAtom = "keyValue";
        static const SR_UTILS_NS::StringAtom SpeedAtom = "speed";
        static const SR_UTILS_NS::StringAtom DtAtom = "dt";
        static const SR_UTILS_NS::StringAtom TotalPixelCountAtom = "totalPixelCount";
        static const SR_UTILS_NS::StringAtom LuminanceInAtom = "luminanceIn";
        static const SR_UTILS_NS::StringAtom ExposureOutAtom = "exposureOut";
        static const SR_UTILS_NS::StringAtom Exposure = "exposure";
    }

    bool AutoExposurePass::Init() {
        SR_TRACY_ZONE;

        if (!Super::Init()) {
            return false;
        }

        // Load compute shaders (reduction HDR->SSBO, reduction SSBO->SSBO, adaptation).
        auto loadShader = [](const SR_UTILS_NS::Path& path) -> SR_GTYPES_NS::Shader::Ptr {
            if (auto pShader = CoreResLoader::Load<SR_GTYPES_NS::Shader>(path)) {
                pShader->AddUsePoint();
                return pShader;
            }
            SR_ERROR("AutoExposurePass::Init() : failed to load shader: {}", path.ToStringRef());
            return nullptr;
        };

        m_pReductionShader = loadShader("Engine/Shaders/AutoExposure/reduction.srsl");
        m_pReductionLinearShader = loadShader("Engine/Shaders/AutoExposure/reduction_linear.srsl");
        m_pAdaptationShader = loadShader("Engine/Shaders/AutoExposure/adaptation.srsl");

        m_multiFrameSSBOResources = SR_UTILS_NS::Features::Instance().Enabled("MultiFrameSSBOResources", false);

        if (!m_pReductionShader || !m_pReductionLinearShader || !m_pAdaptationShader) {
            return false;
        }

        const auto maxFrames = m_multiFrameSSBOResources ? GetPipeline()->GetSwapchainImagesCount() : 1;
        m_exposureSSBO.resize(maxFrames, SR_ID_INVALID);

        for (uint32_t i = 0; i < maxFrames; ++i) {
            // Buffer creation: exposure SSBO (2 floats: current, previous for temporal).
            m_exposureSSBO[i] = GetPipeline()->AllocateSSBO(Details::EXPOSURE_SSBO_SIZE, SSBOUsage::CPUToGPU);
            if (m_exposureSSBO[i] == SR_ID_INVALID) {
                SR_ERROR("AutoExposurePass::Init() : failed to allocate exposure SSBO");
                return false;
            }

            // Initialization: exposure = 1.0 (for first frame and for prev in adaptation).
            float initExposure[2] = { Details::INITIAL_EXPOSURE, Details::INITIAL_EXPOSURE };
            GetPipeline()->UpdateSSBO(m_exposureSSBO[i], initExposure, Details::EXPOSURE_SSBO_SIZE);
        }

        return true;
    }

    void AutoExposurePass::DeInit() {
        SR_TRACY_ZONE;

        FreeBuffers();

        for (int32_t& SSBOId : m_exposureSSBO) {
            GetPipeline()->FreeSSBO(&SSBOId);
        }
        m_exposureSSBO.clear();

        DescriptorManager::Instance().TryFreeDescriptorSet(&m_reductionUBOFirst.descriptorSetId);
        Memory::UBOManager::Instance().TryFreeUBO(&m_reductionUBOFirst.uboId);

        for (auto&& info : m_reductionUBOLinear) {
            DescriptorManager::Instance().TryFreeDescriptorSet(&info.descriptorSetId);
            Memory::UBOManager::Instance().TryFreeUBO(&info.uboId);
        }

        DescriptorManager::Instance().TryFreeDescriptorSet(&m_adaptationUBO.descriptorSetId);
        Memory::UBOManager::Instance().TryFreeUBO(&m_adaptationUBO.uboId);

        if (m_pReductionShader) {
            m_pReductionShader->RemoveUsePoint();
            m_pReductionShader.Reset();
        }
        if (m_pReductionLinearShader) {
            m_pReductionLinearShader->RemoveUsePoint();
            m_pReductionLinearShader.Reset();
        }
        if (m_pAdaptationShader) {
            m_pAdaptationShader->RemoveUsePoint();
            m_pAdaptationShader.Reset();
        }

        Super::DeInit();
    }

    bool AutoExposurePass::Prepare() {
        return Super::Prepare() && m_samplers.PrepareSamplers();
    }

    void AutoExposurePass::Update() {
        SR_TRACY_ZONE;

        Super::Update();

        if (!m_pReductionShader || !m_pReductionLinearShader || !m_pAdaptationShader) {
            return;
        }

        //const auto maxFrames = m_multiFrameSSBOResources ? GetPipeline()->GetSwapchainImagesCount() : 1;
        //for (uint32_t i = 0; i < maxFrames; ++i) {
        //    GetPipeline()->UpdateSSBO(m_reductionSSBOA[i], m_emptyData.data(), static_cast<uint32_t>(m_bufferSize));
        //    GetPipeline()->UpdateSSBO(m_reductionSSBOB[i], m_emptyData.data(), static_cast<uint32_t>(m_bufferSize));
        //}

        if (m_pReductionShader) {
            GetPipeline()->SetCurrentShader(m_pReductionShader.Get());

            if (m_uboManager.BindUBO(m_reductionUBOFirst.uboId) == Memory::UBOManager::BindResult::Failed) {
                SR_ERROR("AutoExposurePass::Update() : failed to bind UBO!");
            }
            else {
                SR_MATH_NS::FVector2 resolution;
                if (auto&& pCamera = GetCamera()) {
                    resolution = pCamera->GetSize().Cast<float_t>();
                }
                else {
                    resolution = GetRenderScene()->GetSurfaceSize().Cast<float_t>();
                }
                m_pReductionShader->SetVec2(Details::ResolutionAtom, resolution);
                SR_UNUSED_VARIABLE(m_pReductionShader->Flush());
            }
        }

        if (m_pReductionLinearShader) {
            GetPipeline()->SetCurrentShader(m_pReductionLinearShader.Get());
            for (auto&& info : m_reductionUBOLinear) {
                if (m_uboManager.BindUBO(info.uboId) == Memory::UBOManager::BindResult::Failed) {
                    SR_ERROR("AutoExposurePass::Update() : failed to bind UBO!");
                }
                m_pReductionLinearShader->SetInt(Details::ElementCountAtom, static_cast<int32_t>(info.elementCount));
                SR_UNUSED_VARIABLE(m_pReductionLinearShader->Flush());
            }
        }

        if (m_pAdaptationShader) {
            GetPipeline()->SetCurrentShader(m_pAdaptationShader.Get());
            if (m_uboManager.BindUBO(m_adaptationUBO.uboId) == Memory::UBOManager::BindResult::Failed) {
                SR_ERROR("AutoExposurePass::Update() : failed to bind UBO!");
            }
            else {
                m_pAdaptationShader->SetFloat(Details::KeyValueAtom, m_keyValue);
                m_pAdaptationShader->SetFloat(Details::SpeedAtom, m_speed);
                m_pAdaptationShader->SetFloat(Details::DtAtom, SR_HTYPES_NS::Time::Instance().DeltaTime());
                m_pAdaptationShader->SetFloat(Details::TotalPixelCountAtom, static_cast<float>(m_totalPixelCount));
                SR_UNUSED_VARIABLE(m_pAdaptationShader->Flush());
            }
        }
    }

    void AutoExposurePass::OnResize(const SR_MATH_NS::UVector2& size) {
        Super::OnResize(size);
        m_samplers.MarkSamplersDirty();
        m_dirtyShader = true;
        m_width = size.x;
        m_height = size.y;
        if (m_width > 0u && m_height > 0u) {
            AllocateBuffers(m_width, m_height);
        }
    }

    void AutoExposurePass::SetRenderTechnique(IRenderTechnique* pRenderTechnique) {
        BasePass::SetRenderTechnique(pRenderTechnique);
        m_samplers.SetRenderTechnique(pRenderTechnique);
    }

    void AutoExposurePass::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        Super::UseSamplers(shader);
        m_samplers.UseSamplers(&shader);
    }

    bool AutoExposurePass::AllocateBuffers(uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;

        auto&& pPipeline = GetPipeline();
        if (!pPipeline) {
            return false;
        }

        uint32_t groupsX = (width + REDUCTION_FIRST_GROUP_X - 1u) / REDUCTION_FIRST_GROUP_X;
        uint32_t groupsY = (height + REDUCTION_FIRST_GROUP_Y - 1u) / REDUCTION_FIRST_GROUP_Y;
        uint32_t elementCount = groupsX * groupsY;

        if (elementCount == m_reductionBufferElementCount && !m_reductionSSBOA.empty()) {
            return true;
        }

        FreeBuffers();

        const auto maxFrames = m_multiFrameSSBOResources ? GetPipeline()->GetSwapchainImagesCount() : 1;
        m_reductionSSBOA.resize(maxFrames, SR_ID_INVALID);
        m_reductionSSBOB.resize(maxFrames, SR_ID_INVALID);

        for (uint32_t i = 0; i < maxFrames; ++i) {
            m_bufferSize = elementCount * sizeof(float);
            m_reductionSSBOA[i] = pPipeline->AllocateSSBO(static_cast<uint32_t>(m_bufferSize), SSBOUsage::CPUToGPU);
            m_reductionSSBOB[i] = pPipeline->AllocateSSBO(static_cast<uint32_t>(m_bufferSize), SSBOUsage::CPUToGPU);

            if (m_reductionSSBOA[i] == SR_ID_INVALID ||  m_reductionSSBOB[i] == SR_ID_INVALID) {
                SR_ERROR("AutoExposurePass::AllocateBuffers() : failed to allocate reduction SSBOs");
                FreeBuffers();
                return false;
            }

            m_emptyData.resize(m_bufferSize, 0);
            GetPipeline()->UpdateSSBO(m_reductionSSBOA[i], m_emptyData.data(), static_cast<uint32_t>(m_bufferSize));
            GetPipeline()->UpdateSSBO(m_reductionSSBOB[i], m_emptyData.data(), static_cast<uint32_t>(m_bufferSize));
        }

        m_reductionBufferElementCount = elementCount;
        return true;
    }

    void AutoExposurePass::FreeBuffers() {
        for (int32_t& SSBOId : m_reductionSSBOA) {
             GetPipeline()->FreeSSBO(&SSBOId);
        }
        for (int32_t& SSBOId : m_reductionSSBOB) {
             GetPipeline()->FreeSSBO(&SSBOId);
        }
        m_reductionSSBOA.clear();
        m_reductionSSBOB.clear();
        m_reductionBufferElementCount = 0u;
    }

    void AutoExposurePass::OnMultisampleChanged() {
        m_dirtyShader = true;
        m_samplers.MarkSamplersDirty();
        Super::OnMultisampleChanged();
    }

    bool AutoExposurePass::DispatchReductionAndAdaptation(uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;

        if (width == 0u || height == 0u || !m_pReductionShader || !m_pReductionLinearShader || !m_pAdaptationShader) {
            return false;
        }

        if (!AllocateBuffers(width, height)) {
            SR_ERROR("AutoExposurePass::DispatchReductionAndAdaptation() : failed to allocate buffers for reduction");
            return false;
        }

        // Pipeline barrier: ensure HDR texture is readable (handled by pipeline when using one cmd buffer).
        // Here we only record compute; barriers are pipeline responsibility.
        GetPipeline()->WriteMemoryBarrier(MemoryBarrierType::ReadAttachmentToCompute);

        if (!GetPipeline()->BeginCompute()) {
            return false;
        }

        const uint32_t groupsX = (width + REDUCTION_FIRST_GROUP_X - 1u) / REDUCTION_FIRST_GROUP_X;
        const uint32_t groupsY = (height + REDUCTION_FIRST_GROUP_Y - 1u) / REDUCTION_FIRST_GROUP_Y;
        m_totalPixelCount = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);

        if (!ReductionFirst(groupsX, groupsY)) {
            GetPipeline()->EndCompute();
            return false;
        }

        GetPipeline()->WriteMemoryBarrier(MemoryBarrierType::ComputeToReadAttachment);

        int32_t luminanceSSBO = ReductionLinear();
        if (luminanceSSBO == SR_ID_INVALID) {
            GetPipeline()->EndCompute();
            return false;
        }

        GetPipeline()->WriteMemoryBarrier(MemoryBarrierType::ComputeToReadAttachment);

        if (!Adaptation(luminanceSSBO)) {
            GetPipeline()->EndCompute();
            return false;
        }

        GetPipeline()->EndCompute();
        m_dirtyShader = false;

        // Barrier: exposure SSBO write -> shader read (for tone mapping) is pipeline responsibility.
        GetPipeline()->WriteMemoryBarrier(MemoryBarrierType::ComputeToReadAttachment);

        return true;
    }

    bool AutoExposurePass::Render() {
        SR_TRACY_ZONE;

        SR_MATH_NS::FVector2 resolution;
        if (auto&& pCamera = GetCamera()) {
            resolution = pCamera->GetSize().Cast<float_t>();
        }
        else {
            resolution = GetRenderScene()->GetSurfaceSize().Cast<float_t>();
        }

        //for (uint32_t i = 0; i < m_reductionSSBOA.size(); ++i) {
        //    GetPipeline()->UpdateSSBO(m_reductionSSBOA[i], m_emptyData.data(), static_cast<uint32_t>(m_bufferSize));
        //    GetPipeline()->UpdateSSBO(m_reductionSSBOB[i], m_emptyData.data(), static_cast<uint32_t>(m_bufferSize));
        //}

        const uint32_t w = SR_MAX(static_cast<uint32_t>(resolution.x), 1);
        const uint32_t h = SR_MAX(static_cast<uint32_t>(resolution.y), 1);
        return DispatchReductionAndAdaptation(w, h);
    }

    bool AutoExposurePass::ReductionFirst(uint32_t groupsX, uint32_t groupsY) {
        SR_TRACY_ZONE;

        // ----- First reduction pass: HDR -> SSBO (one float per workgroup) -----
        if (m_pReductionShader->Use() == SR_GRAPH_NS::ShaderBindResult::Failed) {
            return false;
        }

        if (m_dirtyShader) {
            m_reductionUBOFirst.uboId = m_uboManager.AllocateUBO(m_reductionUBOFirst.uboId);
            m_reductionUBOFirst.descriptorSetId = DescriptorManager::Instance().AllocateDescriptorSet(m_reductionUBOFirst.descriptorSetId);
        }

        if (m_reductionUBOFirst.uboId == SR_ID_INVALID || m_reductionUBOFirst.descriptorSetId == SR_ID_INVALID) {
            SR_ERROR("AutoExposurePass::ReductionFirst() : failed to allocate UBO or descriptor set for first reduction pass");
            return false;
        }

        m_uboManager.BindUBO(m_reductionUBOFirst.uboId);

        const auto frame = m_multiFrameSSBOResources ? GetPipeline()->GetCurrentImageIndex() : 0;
        const auto result = m_descriptorManager.Bind(m_reductionUBOFirst.descriptorSetId);
        if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            UseSamplers(*m_pReductionShader);
            m_pReductionShader->BindSSBO(Details::ReductionOutAtom, m_reductionSSBOA[frame]);
            m_descriptorManager.Flush();
        }

        GetPipeline()->Dispatch(groupsX, groupsY, 1u);

        m_pReductionShader->UnUse();

        return true;
    }

    int32_t AutoExposurePass::ReductionLinear() {
        SR_TRACY_ZONE;

        const auto frame = m_multiFrameSSBOResources ? GetPipeline()->GetCurrentImageIndex() : 0;

         // ----- Subsequent reduction passes: SSBO -> SSBO until 1 element -----
        uint32_t elementCount = m_reductionBufferElementCount;
        int32_t readSSBO = m_reductionSSBOA[frame];
        int32_t writeSSBO = m_reductionSSBOB[frame];

        if (m_pReductionLinearShader->Use() == SR_GRAPH_NS::ShaderBindResult::Failed) {
            return SR_ID_INVALID;
        }

        if (m_dirtyShader) {
            for (auto&& info : m_reductionUBOLinear) {
                info.uboId = m_uboManager.AllocateUBO(info.uboId);
                info.descriptorSetId = DescriptorManager::Instance().AllocateDescriptorSet(info.descriptorSetId);
            }
        }

        uint32_t index = 0u;
        while (elementCount > 1u) {
            if (index >= m_reductionUBOLinear.size()) {
                UBOInfo& info = m_reductionUBOLinear.emplace_back();
                info.uboId = m_uboManager.AllocateUBO(SR_ID_INVALID);
                info.descriptorSetId = DescriptorManager::Instance().AllocateDescriptorSet(SR_ID_INVALID);
            }

            UBOInfo& info = m_reductionUBOLinear[index];
            if (info.uboId == SR_ID_INVALID || info.descriptorSetId == SR_ID_INVALID) {
                SR_ERROR("AutoExposurePass::ReductionLinear() : failed to allocate UBO or descriptor set for linear reduction pass at index {}", index);
                return SR_ID_INVALID;
            }
            info.elementCount = elementCount;

            m_uboManager.BindUBO(info.uboId);

            const auto result = m_descriptorManager.Bind(info.descriptorSetId);
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
                m_pReductionLinearShader->BindSSBO(Details::ReductionInAtom, readSSBO);
                m_pReductionLinearShader->BindSSBO(Details::ReductionOutAtom, writeSSBO);
                m_descriptorManager.Flush();
            }

            uint32_t dispatchGroups = (elementCount + REDUCTION_LINEAR_GROUP_SIZE - 1u) / REDUCTION_LINEAR_GROUP_SIZE;

            GetPipeline()->Dispatch(dispatchGroups, 1u, 1u);

            elementCount = dispatchGroups;
            std::swap(readSSBO, writeSSBO);

            index++;

            GetPipeline()->WriteMemoryBarrier(MemoryBarrierType::ComputeToReadAttachment);
        }

        m_pReductionLinearShader->UnUse();

        // Final luminance sum is in readSSBO (after swap, the last write was to writeSSBO; we read from the other).
        return writeSSBO;
    }

    bool AutoExposurePass::Adaptation(int32_t luminanceSSBO) {
        SR_TRACY_ZONE;

        /// ----- Temporal adaptation: luminance sum + prev exposure -> exposure SSBO -----

        if (m_pAdaptationShader->Use() == SR_GRAPH_NS::ShaderBindResult::Failed) {
            return false;
        }

        if (m_dirtyShader) {
            m_adaptationUBO.uboId = m_uboManager.AllocateUBO(m_adaptationUBO.uboId);
            m_adaptationUBO.descriptorSetId = DescriptorManager::Instance().AllocateDescriptorSet(m_adaptationUBO.descriptorSetId);
        }

        if (m_adaptationUBO.uboId == SR_ID_INVALID || m_adaptationUBO.descriptorSetId == SR_ID_INVALID) {
            SR_ERROR("AutoExposurePass::Adaptation() : failed to allocate UBO or descriptor set for adaptation pass");
            return false;
        }

        m_uboManager.BindUBO(m_adaptationUBO.uboId);

        const auto frame = m_multiFrameSSBOResources ? GetPipeline()->GetCurrentImageIndex() : 0;
        const auto result = m_descriptorManager.Bind(m_adaptationUBO.descriptorSetId);

        if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            m_pAdaptationShader->BindSSBO(Details::LuminanceInAtom, luminanceSSBO);
            m_pAdaptationShader->BindSSBO(Details::ExposureOutAtom, m_exposureSSBO[frame]);
            m_descriptorManager.Flush();
        }

        GetPipeline()->Dispatch(1u, 1u, 1u);

        m_pAdaptationShader->UnUse();

        return true;
    }

    void AutoExposurePass::UseSSBOFromAnotherPass(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;
        Super::UseSSBOFromAnotherPass(shader);
        const auto frame = m_multiFrameSSBOResources ? GetPipeline()->GetCurrentImageIndex() : 0;
        shader.BindSSBO(Details::Exposure, m_exposureSSBO[frame]);
    }
}
