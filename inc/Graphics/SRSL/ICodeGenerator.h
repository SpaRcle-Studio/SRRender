//
// Created by Monika on 30.01.2023.
//

#ifndef SR_ENGINE_ICODEGENERATOR_H
#define SR_ENGINE_ICODEGENERATOR_H

#include <Graphics/SRSL/LexicalTree.h>
#include <Graphics/Pipeline/ShaderUtils.h>

namespace SR_SRSL_NS {
    class SRSLShader;

    class ISRSLCodeGenerator {
    public:
        using SRSLCodeGenRes = std::pair<SRSLResult, SR_UTILS_NS::Map<ShaderStage, SR_UTILS_NS::String>>;

    protected:
        ISRSLCodeGenerator() = default;
        virtual ~ISRSLCodeGenerator() = default;

    protected:
        SR_NODISCARD virtual SRSLCodeGenRes GenerateStages(SR_UTILS_NS::IAllocator* pAllocator, const SRSLShader* pShader) = 0;
        SR_NODISCARD SR_UTILS_NS::StringView GenerateTab(int32_t deep) const;

    protected:
        void Clear();

    protected:
        SRSLResult m_result = SRSLResult();
        mutable SR_UTILS_NS::String m_tabs;

    };
}

#endif //SR_ENGINE_ICODEGENERATOR_H
