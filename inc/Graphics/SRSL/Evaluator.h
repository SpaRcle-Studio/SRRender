//
// Created by Monika on 06.02.2023.
//

#ifndef SR_ENGINE_SRSL_EVALUATOR_H
#define SR_ENGINE_SRSL_EVALUATOR_H

#include <Graphics/SRSL/LexicalTree.h>
#include <Graphics/SRSL/ShaderType.h>

namespace SR_SRSL_NS {
    class SRSLEvaluator : public SR_UTILS_NS::Singleton<SRSLEvaluator> {
        SR_REGISTER_SINGLETON(SRSLEvaluator)
    public:
        SR_NODISCARD double_t Evaluate(const std::string& code);
        SR_NODISCARD double_t Evaluate(const SRSLExpr* pExpr);
        SR_NODISCARD bool MacroEvaluate(const SRSLExpr* pExpr, ShaderParams& params) const;

    private:
        SR_NODISCARD double_t ApplyOperator(const std::string& op, double_t left, double_t right) const;
        SR_NODISCARD bool MacroEvaluateInternal(const SRSLExpr* pExpr) const;
        SR_NODISCARD static bool MacroEvaluateIdentifier(const std::string_view& identifier) ;

    private:
        mutable ShaderParams* m_params = nullptr;

    };
}

#endif //SR_ENGINE_SRSL_EVALUATOR_H
