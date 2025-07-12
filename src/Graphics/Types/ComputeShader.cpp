//
// Created by Monika on 29.06.2025.
//

#include <Graphics/Types/ComputeShader.h>

namespace SR_GTYPES_NS {
    ComputeShader::ComputeShader() = default;

    ComputeShader::~ComputeShader() {
        SR_TRACY_ZONE;

        if (m_isComputeState) {
            SRHalt("ComputeShader::~ComputeShader() : compute shader is still in compute state! You must call EndCompute() before destroying the shader!");
        }

        if (m_isDispatched) {
            SRHalt("ComputeShader::~ComputeShader() : compute shader is still dispatched! You must call EndCompute() before destroying the shader!");
        }

        if (m_isComputeState && !m_isDispatched) {
            SRHalt("ComputeShader::~ComputeShader() : compute shader is in compute state, but not dispatched! You must call Dispatch() and EndCompute() before destroying the shader!");
        }

        if (m_pShader) {
            m_pShader->RemoveUsePoint();
            m_pShader = nullptr;
        }

        if (m_descriptorSet != SR_ID_INVALID) {
            SR_GRAPH_NS::DescriptorManager::Instance().FreeDescriptorSet(&m_descriptorSet);
            m_descriptorSet = SR_ID_INVALID;
        }
    }

    ComputeShader::Ptr ComputeShader::Load(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        if (auto&& pShader = SR_GTYPES_NS::Shader::Load(path)) {
            ComputeShader::Ptr pComputeShader = std::unique_ptr<ComputeShader>(new ComputeShader());
            pComputeShader->m_pShader = pShader;
            pComputeShader->m_pShader->AddUsePoint();
            if (pComputeShader->m_pShader->GetType() != SR_SRSL_NS::ShaderType::Compute) {
                SR_ERROR("ComputeShader::Load() : shader is not a compute shader!");
                return nullptr;
            }
            return pComputeShader;
        }
        SR_ERROR("ComputeShader::Load() : failed to load shader from path: {}", path);
        return nullptr;
    }

    const SR_GTYPES_NS::Shader::Ptr& ComputeShader::GetShader() const noexcept {
        SRAssert2(m_pShader, "ComputeShader::GetShader() : m_pShader is nullptr!");
        return m_pShader;
    }

    bool ComputeShader::BeginCompute() {
        SR_TRACY_ZONE;

        if (!m_pShader) {
            SRHalt("ComputeShader::BeginCompute() : m_pShader is nullptr!");
            return false;
        }

        if (m_isComputeState) {
            SRHalt("ComputeShader::BeginCompute() : already in compute state!");
            return false;
        }

        if (m_isDispatched) {
            SRHalt("ComputeShader::BeginCompute() : something went wrong! Shader is already dispatched, but not in compute state!");
            return false;
        }

        m_isComputeState = true;

        GetPipeline()->SetCurrentShader(m_pShader.Get());

        return GetPipeline()->BeginCompute();
    }

    const SR_GRAPH_NS::Pipeline::Ptr& ComputeShader::GetPipeline() const {
        if (m_pipeline) {
            return m_pipeline;
        }

        SR_TRACY_ZONE;

        SRAssert2(SR_THIS_THREAD, "ComputeShader::GetPipeline() : SR_THIS_THREAD is nullptr!");

        auto&& pRenderContext = SR_THIS_THREAD->GetContext()->GetValue<SR_GRAPH_NS::RenderContext::Ptr>();
        SRAssert2(pRenderContext, "ComputeShader::GetPipeline() : pRenderContext is nullptr!");

        m_pipeline = pRenderContext->GetPipeline();
        SRAssert2(m_pipeline, "ComputeShader::GetPipeline() : m_pipeline is nullptr!");

        return m_pipeline;
    }

    void ComputeShader::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        SR_TRACY_ZONE;

        if (m_isDispatched) {
            SRHalt("ComputeShader::Dispatch() : already dispatched!");
            return;
        }

        m_isDispatched = true;

        if (!m_pShader) {
            SRHalt("ComputeShader::Dispatch() : m_pShader is nullptr!");
            return;
        }

        if (!m_isComputeState) {
            SRHalt("ComputeShader::Dispatch() : not in compute state!");
            return;
        }

        if (m_pShader->Use() != SR_GRAPH_NS::ShaderBindResult::Failed) {
            m_virtualUBO = SR_GRAPH_NS::Memory::UBOManager::Instance().AllocateUBO(m_virtualUBO);

            if (m_descriptorSet == SR_ID_INVALID) {
                m_descriptorSet = SR_GRAPH_NS::DescriptorManager::Instance().AllocateDescriptorSet(SR_ID_INVALID);
            }

            if (m_virtualUBO != SR_ID_INVALID) {
                SR_GRAPH_NS::Memory::UBOManager::Instance().BindNoDublicateUBO(m_virtualUBO);
            }

            if (m_descriptorSet != SR_ID_INVALID) {
                SR_GRAPH_NS::DescriptorManager::Instance().Bind(m_descriptorSet);

                if (m_pShader->AttachDescriptorSets()) {
                    m_pShader->FlushConstants();
                    m_pShader->Dispatch();
                }
            }
            else {
                SR_ERROR("ComputeShader::Dispatch() : failed to allocate descriptor set!");
            }

            m_pShader->UnUse();
        }
        else {
            SR_ERROR("ComputeShader::Dispatch() : failed to use shader!");
        }
    }

    void ComputeShader::Dispatch() {
        SR_TRACY_ZONE;

        if (m_pShader) {
            auto&& workGroupSize = m_pShader->GetComputeWorkGroupSize();
            Dispatch(workGroupSize.x, workGroupSize.y, workGroupSize.z);
        }
        else {
            SRHalt("ComputeShader::Dispatch() : m_pShader is nullptr!");
            m_isDispatched = true;
        }
    }

    void ComputeShader::EndCompute() {
        SR_TRACY_ZONE;

        if (!m_isComputeState) {
            SRHalt("ComputeShader::EndCompute() : not in compute state!");
            return;
        }

        if (!m_isDispatched) {
            SRHalt("ComputeShader::EndCompute() : not dispatched!");
            return;
        }

        if (!m_pShader) {
            SRHalt("ComputeShader::EndCompute() : something went wrong! Shader lost!");
            return;
        }

        m_isDispatched = false;
        m_isComputeState = false;

        GetPipeline()->EndCompute();
        GetPipeline()->WaitComputeIdle();
    }
}
