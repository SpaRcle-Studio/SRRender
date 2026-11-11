//
// Created by Monika on 12.11.2026.
//

#ifndef SR_ENGINE_WGSL_CODE_GENERATOR_H
#define SR_ENGINE_WGSL_CODE_GENERATOR_H

#include <Graphics/SRSL/ICodeGenerator.h>

namespace SR_SRSL_NS {
    class SR_GRAPHICS_DLL_API WGSLCodeGenerator : public ISRSLCodeGenerator, public SR_UTILS_NS::Singleton<WGSLCodeGenerator> {
        SR_REGISTER_SINGLETON(WGSLCodeGenerator)
    private:
        WGSLCodeGenerator() = default;
        ~WGSLCodeGenerator() override = default;

    public:
        SR_NODISCARD SRSLCodeGenRes GenerateStages(SR_UTILS_NS::IAllocator* allocator, const SRSLShader* shader) override;
    };
}
#endif
