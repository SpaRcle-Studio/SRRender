//
// Created by Monika on 09.02.2023.
//

#ifndef SR_ENGINE_GRAPHICS_SSAOPASS_H
#define SR_ENGINE_GRAPHICS_SSAOPASS_H


#include <Graphics/Pass/PostProcessPass.h>

#include "Graphics/Memory/SSBO.h"


namespace SR_GRAPH_NS {
    class SSAOPass: public PostProcessPass{
        SR_CLASS()
        using Super = PostProcessPass;
        using SSAOKernel = std::vector<SR_MATH_NS::Vector4<float_t>>;

    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<SSAOPass>;
        SSAOKernel m_kernel;
        SR_GRAPH_NS::SSBOInstance::Ptr m_kernelSSBO;


    public:
        ~SSAOPass() override = default;

        bool Init() override;
        bool Prepare() override;
        void Update() override;
        void UseSSBO() override;
        SSAOKernel CreateKernel() const;
        void UseConstants(SR_GTYPES_NS::Shader& shader) override;
    };
}


#endif //SR_ENGINE_SSAOPASS_H
