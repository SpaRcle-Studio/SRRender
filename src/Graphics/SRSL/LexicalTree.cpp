//
// Created by Monika on 22.01.2023.
//

#include <Graphics/SRSL/LexicalTree.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <Enum/ShaderStage.hpp>

namespace SR_SRSL_NS {
    SR_UTILS_NS::StringView SRSLExpr::ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const {
        SR_TRACY_ZONE;

        if ((token == "++" || token == "--") && !args.empty()) {
            SRHalt0();
        }

        if (isArray) {
            if (args.size() == 2) {
                args[0]->ToString(deep + 1, buffer);
                buffer += "[";
                args[1]->ToString(deep + 1, buffer);
                buffer += "]";
                return buffer;
            }
            else {
                args[0]->ToString(deep + 1, buffer);
                buffer += "[]";
                return buffer;
            }
        }
        else if (args.empty()) {
            buffer += token;
            return buffer;
        }
        else if (args.size() == 1) {
            buffer += "(";
            buffer += token;
            args[0]->ToString(deep + 1, buffer);
            buffer += ")";
            return buffer;
        }
        else if (args.size() == 2) {
            buffer += "(";
            args[0]->ToString(deep + 1, buffer);
            buffer += token;
            args[1]->ToString(deep + 1, buffer);
            buffer += ")";
            return buffer;
        }

        return SR_UTILS_NS::StringView();
    }

    SR_UTILS_NS::StringView SRSLExpr::GetAsName() {
        if (!isArray) {
            return token;
        }

        if (args.size() >= 1) {
            return args[0]->GetAsName();
        }
        return SR_UTILS_NS::StringView();
    }

    SRSLExpr* SRSLExpr::CreateStringExpression(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView token) {
        auto&& pExpr = AllocateLexicalUnit<SRSLExpr>(allocator, token);
        pExpr->isString = true;
        return pExpr;
    }

    SRSLExpr::SRSLExpr(SR_UTILS_NS::IAllocator& allocator)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(&allocator)
        , args(&allocator)
    { }

    SRSLExpr::SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView nToken)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(nToken, &allocator)
        , args(&allocator)
    {
        SRAssert(token != "(" && token != ")");
        SRAssert(token != "[" && token != "]");
        SRAssert(token != "}");
        if (token == "{") {
            isList = true;
        }
    }

    SRSLExpr::SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView nToken, SRSLExpr* pAExpr)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(nToken, &allocator)
        , args(&allocator)
    {
        SRAssert(pAExpr);
        SRAssert(token != ")" && token != "(");
        SRAssert(token != "[" && token != "]");
        args.emplace_back(pAExpr);
    }

    SRSLExpr::SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SR_UTILS_NS::StringView nToken, SRSLExpr* pAExpr, SRSLExpr* pBExpr)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(nToken, &allocator)
        , args(&allocator)
    {
        SRAssert(pAExpr);
        SRAssert(token != ")" && token != "(");
        SRAssert(token != "]");

        if (token == "[") {
            isArray = true;
        }

        args.reserve(2);
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

    SRSLExpr::SRSLExpr(SR_UTILS_NS::IAllocator& allocator, SRSLExpr* pAExpr, SRSLExpr* pBExpr)
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(&allocator)
        , args(&allocator)
    {
        SRAssert(pAExpr && pBExpr);
        args.reserve(2);
        args.emplace_back(pAExpr);
        args.emplace_back(pBExpr);
    }

    SRSLExpr::SRSLExpr(SRSLExpr&& other) noexcept
        : SRSLLexicalUnit(LexicalUnitType::Expr)
        , token(SR_UTILS_NS::Exchange(other.token, { }))
        , args(SR_UTILS_NS::Exchange(other.args, { }))
        , isCall(SR_UTILS_NS::Exchange(other.isCall, { }))
        , isArray(SR_UTILS_NS::Exchange(other.isArray, { }))
        , isList(SR_UTILS_NS::Exchange(other.isList, { }))
        , isString(SR_UTILS_NS::Exchange(other.isString, { }))
    { }

    SR_UTILS_NS::StringView SRSLDecorator::ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const {
        buffer += "[";
        buffer += name;

        if (!args.empty()) {
            buffer += "(";

            for (uint32_t i = 0; i < args.size(); ++i) {
                args[i]->ToString(deep + 1, buffer);
                if (i + 1 < args.size()) {
                    buffer += ", ";
                }
            }

            buffer += ")";
        }

        buffer += "]";
        return buffer;
    }

    SRSLDecorator::SRSLDecorator(SR_UTILS_NS::IAllocator& allocator)
        : SRSLLexicalUnit(LexicalUnitType::Decorator)
        , name(&allocator)
        , args(&allocator)
    { }

    SR_UTILS_NS::StringView SRSLDecorators::ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const {
        buffer += "[";

        for (uint32_t i = 0; i < decorators.size(); ++i) {
            decorators[i].ToString(deep + 1, buffer);

            if (i + 1 < decorators.size()) {
                buffer += ", ";
            }
        }

        buffer += "]";
        return buffer;
    }

    SRSLDecorator* SRSLDecorators::Find(SR_UTILS_NS::StringView name) {
        for (auto&& decorator : decorators) {
            if (decorator.name == name) {
                return &decorator;
            }
        }
        return nullptr;
    }

    SRSLDecorators::SRSLDecorators(SR_UTILS_NS::IAllocator& allocator)
        : SRSLLexicalUnit(LexicalUnitType::Decorators)
        , decorators(&allocator)
    { }

    SRSLLexicalTree::SRSLLexicalTree(SR_UTILS_NS::IAllocator& allocator)
        : SRSLLexicalUnit(LexicalUnitType::LexcialTree)
        , lexicalTree(&allocator)
    { }

    SR_UTILS_NS::StringView SRSLLexicalTree::ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const {
        if (deep > 0) {
            buffer += "{\n";
        }

        for (auto&& pUnit : lexicalTree) {
            for (uint32_t i = 0; i < deep; ++i) {
                buffer += '\t';
            }
            pUnit->ToString(deep + 1, buffer);
            buffer += '\n';
        }

        if (deep > 0) {
            buffer += "}\n";
        }

        return buffer;
    }

    SRSLFunction* SRSLLexicalTree::FindFunction(SR_UTILS_NS::StringView name) const {
        for (auto&& pUnit : lexicalTree) {
            if (pUnit->GetLexicalUnitType() != LexicalUnitType::Function) {
                continue;
            }
            auto&& pFunction = static_cast<SRSLFunction*>(pUnit);
            if (SR_UTILS_NS::StringView(pFunction->pName->token) == name) {
                return pFunction;
            }
        }
        return nullptr;
    }

    SRSLExpr* SRSLLexicalTree::AsExpression() const {
        if (lexicalTree.size() != 1 || lexicalTree.back()->GetLexicalUnitType() != LexicalUnitType::Expr) {
            return nullptr;
        }
        return static_cast<SRSLExpr*>(lexicalTree.back());
    }

    void SRSLLexicalTree::Clear() {
        lexicalTree.clear();
    }

    SR_UTILS_NS::StringView SRSLVariable::ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const {
        if (pDecorators) {
            pDecorators->ToString(deep + 1, buffer);
            buffer += " ";
        }

        pType->ToString(deep + 1, buffer);
        buffer += " ";
        pName->ToString(deep + 1, buffer);

        if (pExpr) {
            buffer += " = ";
            pExpr->ToString(deep + 1, buffer);
        }

        return buffer;
    }

    SR_UTILS_NS::StringView SRSLVariable::GetType() const {
        return pType ? pType->token : SR_UTILS_NS::StringView();
    }

    SR_UTILS_NS::StringView SRSLVariable::GetName() const {
        return pName ? pName->GetAsName() : SR_UTILS_NS::StringView();
    }

    SR_UTILS_NS::StringView SRSLFunction::ToString(uint32_t deep, SR_UTILS_NS::String& buffer) const {
        if (pDecorators) {
            pDecorators->ToString(deep + 1, buffer);
            buffer += " ";
        }

        pType->ToString(deep + 1, buffer);
        buffer += " ";
        pName->ToString(deep + 1, buffer);
        buffer += "(";

        for (uint32_t i = 0; i < args.size(); ++i) {
            args[i]->ToString(deep + 1, buffer);
            if (i + 1 < args.size()) {
                buffer += ", ";
            }
        }

        buffer += ")\n";

        if (pLexicalTree) {
            pLexicalTree->ToString(deep + 1, buffer);
        }

        return buffer;
    }


    SRSLIfStatement::SRSLIfStatement(SR_UTILS_NS::IAllocator& allocator, bool isElse)
        : SRSLLexicalUnit(LexicalUnitType::IfStatement)
        , isElse(isElse)
    { }

    bool SRSLStructureStatement::HasDynamicArray() const {
        for (auto&& pUnit : pLexicalTree->lexicalTree) {
            if (pUnit->GetLexicalUnitType() != LexicalUnitType::Variable) {
                continue;
            }
            auto&& pVar = static_cast<SRSLVariable*>(pUnit);
            if (pVar->pName && pVar->pName->isArray && pVar->pName->args.size() == 1) {
                return true;
            }
        }
        return false;
    }

    void SRSLAnalyzedTree::PostProcess(const ShaderParams& params) {
        SR_TRACY_ZONE;

        const bool isColorPassDefined = params.IsDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = params.IsDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);

        const bool isNeedRemoveFragmentEntryPoint = isColorPassDefined || isCascadedMapPassDefined;

        for (auto&& pUnit : pLexicalTree->lexicalTree) {
            if (pUnit->GetLexicalUnitType() != LexicalUnitType::Function) {
                continue;
            }
            auto&& pFunction = static_cast<SRSLFunction*>(pUnit);
            if (isNeedRemoveFragmentEntryPoint && pFunction->pName->GetAsName() == SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment)) {
                if (pFunction->pLexicalTree) {
                    pFunction->pLexicalTree->Clear();
                }
            }
        }
    }

    SRSLAnalyzedTree::SRSLAnalyzedTree(SR_UTILS_NS::IAllocator& allocator)
        : allocator(allocator)
    { }
}
