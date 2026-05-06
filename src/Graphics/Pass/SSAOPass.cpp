//
// Created by Monika on 09.02.2023.
//

#include <Graphics/Pass/SSAOPass.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Shader.h>
#include <Codegen/SSAOPass.generated.hpp>

#include "Utils/Common/Numeric.h"
#include "Utils/Resources/ResourceManager.h"

namespace SR_GRAPH_NS {


    bool SSAOPass::Init() {
        SetShader("Engine/Shaders/SSAO/ssao.srsl");

        auto mat = GetMaterial();
        mat->SetTexture("Noise", CoreResLoader::Load<SR_GTYPES_NS::Texture>("Engine/Textures/4x4noise.png"));

        m_kernel = CreateKernel();

        m_kernelSSBO = SSBOInstance::Create(sizeof(SR_MATH_NS::Vector4<float_t>) * m_kernel.size(),
            SSBOUsage::CPUToGPU, "SSAOKernel");

        m_kernelSSBO->UpdateSSBO(m_kernel.data(), sizeof(SR_MATH_NS::Vector4<float_t>) * m_kernel.size());

        return PostProcessPass::Init();
    }

    void SSAOPass::UseSSBO() {
        if (m_kernelSSBO){
            if (!m_kernelSSBO->Bind("SSAOKernel")){
                SR_ERROR("SSAOPass::UseSSBO() : Cant Bind SSAOKernel)");
            };
        }
    }

    SSAOPass::SSAOKernel SSAOPass::CreateKernel() const {
        std::vector<SR_MATH_NS::Vector4<float_t>> kernel;
        kernel.resize(64);

        for (uint8_t i = 0; i < kernel.size(); ++i)
        {
            SR_MATH_NS::Vector4<float_t> sample(
                    SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                    SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                    SR_UTILS_NS::Random::Instance().Float(0.0, 1.0),
                    0.f
            );

            sample = sample.Normalize() * SR_UTILS_NS::Random::Instance().Float(0.0, 1.0);

            float_t scale = float_t(i) / static_cast<float_t>(kernel.size());
            scale = SR_MATH_NS::Lerp(0.1, 1.0, scale * scale);

            kernel[i] = sample * scale;
        }

        return kernel;
    }

    bool SSAOPass::Prepare(){

        return Super::Prepare();
    }

    void SSAOPass::Update() {

        Super::Update();
    }
}
