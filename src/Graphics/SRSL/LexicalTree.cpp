//
// Created by Monika on 22.01.2023.
//

#include <Graphics/SRSL/LexicalTree.h>
#include <Graphics/SRSL/ShaderVariables.h>

namespace SR_SRSL_NS {
    std::string SRSLExpr::ToString(uint32_t deep) const {
        if ((token == "++" || token == "--") && !args.empty()) {
            SRHalt0();
        }

        if (isArray) {
            if (args.size() == 2) {
                return args[0]->ToString(deep + 1) + "[" + args[1]->ToString(deep + 1) + "]";
            }
            else {
                return args[0]->ToString(deep + 1) + "[]";
            }
        }
        else if (args.empty()) {
            return token;
        }
        else if (args.size() == 1) {
            return "(" + token + args[0]->ToString(deep + 1) + ")";
        }
        else if (args.size() == 2) {
            return "(" + args[0]->ToString(deep + 1) + token + args[1]->ToString(deep + 1) + ")";
        }

        return std::string();
    }

    std::string SRSLExpr::GetAsName() {
        if (!isArray) {
            return token;
        }

        if (args.size() >= 1) {
            return args[0]->GetAsName();
        }
        return std::string();
    }

    std::string SRSLDecorator::ToString(uint32_t deep) const {
        std::string code = "[" + name;

        if (!args.empty()) {
            code += "(";

            for (uint32_t i = 0; i < args.size(); ++i) {
                code += args[i]->ToString(deep + 1);
                if (i + 1 < args.size()) {
                    code += ", ";
                }
            }

            code += ")";
        }

        return code + "]";
    }

    std::string SRSLDecorators::ToString(uint32_t deep) const {
        std::string code = "[";

        for (uint32_t i = 0; i < decorators.size(); ++i) {
            code += decorators[i].ToString(deep + 1);

            if (i + 1 < decorators.size()) {
                code += ", ";
            }
        }

        return code + "]";
    }

    SRSLDecorator* SRSLDecorators::Find(const std::string &name) {
        for (auto&& decorator : decorators) {
            if (decorator.name == name) {
                return &decorator;
            }
        }

        return nullptr;
    }

    std::string SRSLLexicalTree::ToString(uint32_t deep) const {
        std::string code;

        if (deep > 0) {
            code += "{\n";
        }

        for (auto&& pUnit : lexicalTree) {
            code += std::string(deep, '\t') + pUnit->ToString(deep + 1) + "\n";
        }

        if (deep > 0) {
            code += "}\n";
        }

        return code;
    }

    SRSLFunction *SRSLLexicalTree::FindFunction(const std::string &name) const {
        for (auto&& pUnit : lexicalTree) {
            if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                if (pFunction->pName->token == name) {
                    return pFunction;
                }
            }
        }

        return nullptr;
    }

    SRSLExpr *SRSLLexicalTree::AsExpression() const {
        if (lexicalTree.size() != 1) {
            return nullptr;
        }
        return dynamic_cast<SRSLExpr*>(lexicalTree.back());
    }

    std::string SRSLVariable::ToString(uint32_t deep) const {
        std::string code;

        if (pDecorators) {
            code += pDecorators->ToString(deep + 1) + " ";
        }

        code += pType->ToString(deep + 1) + " " + pName->ToString(deep + 1);

        if (pExpr) {
            code += " = " + pExpr->ToString(deep + 1);
        }

        return code;
    }

    std::string SRSLVariable::GetType() const {
        if (pType) {
            return pType->token;
        }

        return std::string();
    }

    std::string SRSLVariable::GetName() const {
        if (!pName) {
            return std::string();
        }

        return pName->GetAsName();
    }

    std::string SRSLFunction::ToString(uint32_t deep) const {
        std::string code;

        if (pDecorators) {
            code += pDecorators->ToString(deep + 1) + " ";
        }

        code += pType->ToString(deep + 1) + " " + pName->ToString(deep + 1) + "(";

        for (uint32_t i = 0; i < args.size(); ++i) {
            code += args[i]->ToString(deep + 1);

            if (i + 1 < args.size()) {
                code += ", ";
            }
        }

        code += ")\n";

        if (pLexicalTree) {
            code += pLexicalTree->ToString(deep + 1);
        }

        return code;
    }

    SRSLFunction::~SRSLFunction() {
        SR_SAFE_DELETE_PTR(pDecorators);
        SR_SAFE_DELETE_PTR(pType);
        SR_SAFE_DELETE_PTR(pName);
        SR_SAFE_DELETE_PTR(pLexicalTree);

        for (auto&& pArg : args) {
            delete pArg;
        }
    }

    SRSLIfStatement::~SRSLIfStatement() {
        SR_SAFE_DELETE_PTR(pExpr);
        SR_SAFE_DELETE_PTR(pLexicalTree);
    }

    SRSLIfStatement::SRSLIfStatement(bool isElse)
        : SRSLLexicalUnit()
        , isElse(isElse)
    { }

    SRSLForStatement::~SRSLForStatement() {
        SR_SAFE_DELETE_PTR(pExpr);
        SR_SAFE_DELETE_PTR(pCondition);
        SR_SAFE_DELETE_PTR(pVar);
        SR_SAFE_DELETE_PTR(pLexicalTree);
    }

    SRSLStructureStatement::~SRSLStructureStatement() {
        SR_SAFE_DELETE_PTR(pName);
        SR_SAFE_DELETE_PTR(pLexicalTree);
    }

    bool SRSLStructureStatement::HasDynamicArray() const {
        for (auto&& pUnit : pLexicalTree->lexicalTree) {
            if (auto&& pVar = dynamic_cast<SRSLVariable*>(pUnit)) {
                if (pVar->pName && pVar->pName->isArray && pVar->pName->args.size() == 1) {
                    return true;
                }
            }
        }
        return false;
    }

    SRSLWhileStatement::~SRSLWhileStatement() {
        SR_SAFE_DELETE_PTR(pCondition);
        SR_SAFE_DELETE_PTR(pLexicalTree);
    }

    void SRSLAnalyzedTree::PostProcess(const ShaderMacrosParams& macros) {
        SR_TRACY_ZONE;

        const bool isColorPassDefined = macros.IsDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = macros.IsDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);

        const bool isNeedRemoveFragmentEntryPoint = isColorPassDefined || isCascadedMapPassDefined;

        for (auto&& pUnit : pLexicalTree->lexicalTree) {
            if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                if (isNeedRemoveFragmentEntryPoint && pFunction->pName->GetAsName() == SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment)) {
                    if (pFunction->pLexicalTree) {
                        pFunction->pLexicalTree->Clear();
                    }
                }
            }
        }
    }
}
