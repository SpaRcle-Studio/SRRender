//
// Created by Monika on 22.01.2023.
//

#include <Graphics/SRSL/LexicalTree.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <Enum/ShaderStage.hpp>

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

    SR_UTILS_NS::StringView SRSLExpr::GetAsName() {
        SR_TRACY_ZONE;
        if (!isArray) {
            return token;
        }

        if (args.size() >= 1) {
            return args[0]->GetAsName();
        }
        return SR_UTILS_NS::StringView();
    }

    SRSLExpr* SRSLExpr::CreateStringExpression(SR_UTILS_NS::String token) {
        SR_TRACY_ZONE;
        auto&& pExpr = new SRSLExpr(std::move(token));
        pExpr->isString = true;
        return pExpr;
    }

    SRSLExpr* SRSLExpr::CreateStringExpression(std::string_view token) {
        SR_TRACY_ZONE;
        auto&& pExpr = new SRSLExpr(token);
        pExpr->isString = true;
        return pExpr;
    }

    SRSLExpr::SRSLExpr(std::string_view token)
        : SRSLExpr(SR_UTILS_NS::String(token))
    { }

    SRSLExpr::SRSLExpr(SR_UTILS_NS::String token)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(token)
    {
        SR_TRACY_ZONE;
        SRAssert(this->token != "(" && this->token != ")");
        SRAssert(this->token != "[" && this->token != "]");
        SRAssert(this->token != "}");

        if (this->token == "{") {
            isList = true;
        }
    }

    SRSLExpr::SRSLExpr(std::string_view token, SRSLExpr* pAExpr)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(token)
    {
        SR_TRACY_ZONE;
        SRAssert(pAExpr);
        SRAssert(this->token != ")" && this->token != "(");
        SRAssert(this->token != "[" && this->token != "]");
        args.emplace_back(pAExpr);
    }

    SRSLExpr::SRSLExpr(std::string_view token, SRSLExpr *pAExpr, SRSLExpr *pBExpr)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(token)
    {
        SR_TRACY_ZONE;
        SRAssert(pAExpr);
        SRAssert(this->token != ")" && this->token != "(");
        SRAssert(this->token != "]");

        if (this->token == "[") {
            isArray = true;
        }

        args.emplace_back(pAExpr);

        if (pBExpr) {
            args.emplace_back(pBExpr);
        }
        else {
            SRAssert(isArray);
            /// массив без явного размера (динамический массив вида float[] arr;)
            SR_NOOP;
        }
    }

    SRSLExpr::SRSLExpr(SRSLExpr* pAExpr, SRSLExpr* pBExpr)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
    {
        SR_TRACY_ZONE;
        SRAssert(pAExpr && pBExpr);
        args.emplace_back(pAExpr);
        args.emplace_back(pBExpr);
    }

    SRSLExpr::~SRSLExpr() {
        for (auto&& pExpr : args) {
            delete pExpr;
        }
    }

    SRSLExpr::SRSLExpr(SRSLExpr&& other) noexcept
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(SR_UTILS_NS::Exchange(other.token, { }))
        , args(SR_UTILS_NS::Exchange(other.args, { }))
        , isCall(SR_UTILS_NS::Exchange(other.isCall, { }))
        , isArray(SR_UTILS_NS::Exchange(other.isArray, { }))
    { }

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

    SRSLFunction* SRSLLexicalTree::FindFunction(SR_UTILS_NS::StringView name) const {
        for (auto&& pUnit : lexicalTree) {
            if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                if (SR_UTILS_NS::StringView(pFunction->pName->token) == name) {
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

    void SRSLLexicalTree::Clear() {
        for (auto&& pUnit : lexicalTree) {
            delete pUnit;
        }
        lexicalTree.clear();
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

    SR_UTILS_NS::StringView SRSLVariable::GetType() const {
        if (pType) {
            return pType->token;
        }

        return SR_UTILS_NS::StringView();
    }

    SR_UTILS_NS::StringView SRSLVariable::GetName() const {
        if (!pName) {
            return SR_UTILS_NS::StringView();
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
        : SRSLLexicalUnit(LexicalUnitType::IfStatement)
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

    void SRSLAnalyzedTree::PostProcess(const ShaderParams& params) {
        SR_TRACY_ZONE;

        const bool isColorPassDefined = params.IsDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = params.IsDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);

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
