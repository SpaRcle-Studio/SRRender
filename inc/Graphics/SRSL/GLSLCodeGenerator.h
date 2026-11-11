//
// Created by Monika on 12.11.2026.
//

#ifndef SR_ENGINE_GLSL_CODE_GENERATOR_H
#define SR_ENGINE_GLSL_CODE_GENERATOR_H

#include <Graphics/SRSL/ICodeGenerator.h>

namespace SR_SRSL_NS {
    class SR_GRAPHICS_DLL_API GLSLCodeGenerator : public ISRSLCodeGenerator, public SR_UTILS_NS::Singleton<GLSLCodeGenerator> {
        SR_REGISTER_SINGLETON(GLSLCodeGenerator)
    private:
        GLSLCodeGenerator() = default;
        ~GLSLCodeGenerator() override = default;

    public:
        SR_NODISCARD SRSLCodeGenRes GenerateStages(SR_UTILS_NS::IAllocator* allocator, const SRSLShader* shader) override;
    };
}
#endif
