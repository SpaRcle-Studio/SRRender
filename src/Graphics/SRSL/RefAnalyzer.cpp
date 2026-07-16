//
// Created by Monika on 03.02.2023.
//

#include <Graphics/SRSL/RefAnalyzer.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <Enum/ShaderStage.hpp>

namespace SR_SRSL_NS {
    void SRSLUseStack::Concat(const SRSLUseStack::Ptr& pOther) {
        for (auto&& name : pOther->variables) {
            variables.insert(name);
        }

        for (auto&& [name, function] : pOther->functions) {
            if (functions.count(name) == 1) {
                continue;
            }

            functions[name] = function;
        }

        for (auto&& name : pOther->forceUsedVariables) {
            forceUsedVariables.insert(name);
        }

        for (auto&& name : pOther->forceUsedFunctions) {
            forceUsedFunctions.insert(name);
        }
    }

    std::string SRSLUseStack::ToString(int32_t deep) const {
        std::string str;

        for (auto&& name : variables) {
            str += std::string(SR_MAX(0, deep * 4), ' ') + "var is \"" + name + "\"\n";
        }

        for (auto&& name : forceUsedVariables) {
            str += std::string(SR_MAX(0, deep * 4), ' ') + "force use var is \"" + name + "\"\n";
        }

        for (auto&& [name, function] : functions) {
            if (function) {
                str += std::string(SR_MAX(0, deep * 4), ' ') + "call \"" + name + "\" function:\n" + function->ToString(deep + 1);
            }
            else {
                str += std::string(SR_MAX(0, deep * 4), ' ') + "call \"" + name + "\" function\n";
            }
        }

        for (auto&& name : forceUsedFunctions) {
            str += std::string(SR_MAX(0, deep * 4), ' ') + "force use function is \"" + name + "\"\n";
        }

        return str;
    }

    bool SRSLUseStack::IsVariableUsed(const std::string_view& name, uint8_t depth) const {
        SR_TRACY_ZONE;

        depth++;

        if (depth > 128) {
            SR_TRACY_ZONE_COLOR(0xFF0000);
            return false;
        }

        for (auto&& nameInForce : forceUsedVariables) {
            if (nameInForce == name) {
                return true;
            }
        }

        for (auto&& variable : variables) {
            if (variable == name) {
                return true;
            }
        }

        for (auto&& function : functions) {
            if (function.second && function.second->IsVariableUsed(name, depth)) {
                return true;
            }
        }

        if (SRVerify(pRoot)) {
            for (auto&& function : forceUsedFunctions) {
                if (auto&& pFunction = pRoot->FindFunction(function); pFunction && pFunction->IsVariableUsed(name, depth)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool SRSLUseStack::IsFunctionUsed(const std::string_view& name, uint8_t depth) const {
        SR_TRACY_ZONE;

        depth++;

        if (depth > 128) {
            SR_TRACY_ZONE_COLOR(0xFF0000);
            return false;
        }

        for (auto&& nameInForce : forceUsedFunctions) {
            if (nameInForce == name) {
                return true;
            }
            if (SRVerify(pRoot)) {
                if (auto&& pFunction = pRoot->FindFunction(nameInForce); pFunction && pFunction->IsFunctionUsed(name, depth)) {
                    return true;
                }
            }
        }

        for (auto&& function : functions) {
            if (function.first == name) {
                return true;
            }

            if (!function.second) {
                continue;
            }

            if (function.second->IsFunctionUsed(name, depth)) {
                return true;
            }
        }

        return false;
    }

    bool SRSLUseStack::IsStructUsed(const std::string_view& name) const {
        // TODO: Monika will handle this <3
        return true;
    }

    SRSLUseStack::Ptr SRSLUseStack::FindFunction(const std::string_view &name) const {
        SR_TRACY_ZONE;

        for (auto&& function : functions) {
            if (function.first == name) {
                return function.second;
            }
        }

        return nullptr;
    }

    bool SRSLUseStack::IsVariableUsedInEntryPoint(SR_GRAPH_NS::ShaderStage stage, const std::string_view& name) const {
        SR_TRACY_ZONE;

        if (auto&& it = SR_SRSL_ENTRY_POINTS.find(stage); it != SR_SRSL_ENTRY_POINTS.end()) {
            if (auto&& pFunction = FindFunction(it->second); pFunction && pFunction->IsVariableUsed(name)) {
                return true;
            }
        }
        return false;
    }

    bool SRSLUseStack::IsVariableUsedInEntryPoints(const std::string_view& name) const {
        SR_TRACY_ZONE;

        for (auto&& [stage, entryPoint] : SR_SRSL_ENTRY_POINTS) {
            if (auto&& pFunction = FindFunction(entryPoint); pFunction && pFunction->IsVariableUsed(name)) {
                return true;
            }
        }

        return false;
    }

    std::set<ShaderStage> SRSLUseStack::IsVariableUsedInEntryPointsExt(const std::string_view& name) const {
        std::set<ShaderStage> stages;

        for (auto&& [stage, entryPoint] : SR_SRSL_ENTRY_POINTS) {
            if (auto&& pFunction = FindFunction(entryPoint); pFunction && pFunction->IsVariableUsed(name)) {
                stages.insert(stage);
            }
        }

        return stages;
    }

    void SRSLUseStack::SetRoot(SRSLUseStack* pRootStack) {
        pRoot = pRootStack;
        for (auto&& [name, function] : functions) {
            if (function) {
                function->SetRoot(pRootStack);
            }
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

    SRSLUseStack::Ptr SRSLRefAnalyzer::Analyze(const SRSLAnalyzedTree::Ptr& pAnalyzedTree, const SR_SRSL_NS::ShaderParams& params) {
        SR_TRACY_ZONE;
        SR_GLOBAL_LOCK
        m_analyzedTree = pAnalyzedTree;
        std::list<std::string> stack;
        auto&& pUseStack = AnalyzeTree(stack, pAnalyzedTree->pLexicalTree);
        if (pUseStack) {
            PreprocessUseStack(pUseStack, params);
        }

        pUseStack->SetRoot(pUseStack.get());

        /// SR_LOG("Analyzed pUseStack:\n\t{}"_format(pUseStack->ToString(1)));

        return pUseStack;
    }

    void SRSLRefAnalyzer::PreprocessUseStack(SRSLUseStack::Ptr& pUseStack, const SR_SRSL_NS::ShaderParams& params) {
        const bool isColorPassDefined = params.IsDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = params.IsDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);

        if (isColorPassDefined) {
            auto&& pFragmentEntryPoint = pUseStack->FindFunction(SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment));

            pFragmentEntryPoint->forceUsedVariables.insert(SHADER_PC_COLOR_BUFFER_VALUE);

            if (pUseStack->FindFunction("fragment_color_buffer_discard")) {
                pFragmentEntryPoint->forceUsedFunctions.insert("fragment_color_buffer_discard");
            }
        }
        else if (isCascadedMapPassDefined) {
            auto&& pFragmentEntryPoint = pUseStack->FindFunction(SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment));

            if (pUseStack->FindFunction("fragment_depth_buffer_discard")) {
                pFragmentEntryPoint->forceUsedFunctions.insert("fragment_depth_buffer_discard");
            }
        }
    }

    SRSLUseStack::Ptr SRSLRefAnalyzer::AnalyzeTree(std::list<std::string>& stack, SRSLLexicalTree* pTree) {
        auto&& pUseStack = SRSLUseStack::Ptr(new SRSLUseStack());
        if (!pTree) {
            return pUseStack;
        }

        for (auto&& pUnit : pTree->lexicalTree) {
            /// Выражения в декораторах не учитываем, так как они не могут использовать переменные
            /// Однако стоит на будущее подумать использование в них макросов
            if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit); pVariable) {
                AnalyzeVariable(pUseStack, stack, pVariable);
            }
            else if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                AnalyzeFunction(pUseStack, stack, pFunction);
            }
            else if (auto&& pSubTree = dynamic_cast<SRSLLexicalTree*>(pUnit)) {
                pUseStack->Concat(AnalyzeTree(stack, pSubTree));
            }
            else if (auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(pUnit)) {
                AnalyzeIfStatement(pUseStack, stack, pIfStatement);
            }
            else if (auto&& pExpr = dynamic_cast<SRSLExpr*>(pUnit)) {
                AnalyzeExpression(pUseStack, stack, pExpr);
            }
            else if (auto&& pForStatement = dynamic_cast<SRSLForStatement*>(pUnit)) {
                AnalyzeForStatement(pUseStack, stack, pForStatement);
            }
            else if (auto&& pWhileStatement = dynamic_cast<SRSLWhileStatement*>(pUnit)) {
                AnalyzeWhileStatement(pUseStack, stack, pWhileStatement);
            }
            else if (auto&& pReturn = dynamic_cast<SRSLReturn*>(pUnit)) {
                AnalyzeExpression(pUseStack, stack, pReturn->pExpr);
            }
        }

        return pUseStack;
    }

    void SRSLRefAnalyzer::AnalyzeExpression(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLExpr* pExpr) {
        if (!pExpr) {
            return;
        }

        if (pExpr->token == ".") {
            AnalyzeExpression(pUseStack, stack, pExpr->args[0]);
            return;
        }

        if (pExpr->token == "=") {
            if (pExpr->args[0]->isArray) {
                AnalyzeArrayExpression(pUseStack, stack, pExpr->args[0]);
            }
            else {
                SRAssert(!pExpr->args[0]->token.empty());
                if (pExpr->args[0]->token == ".") {
                    AnalyzeExpression(pUseStack, stack, pExpr->args[0]);
                }
                else {
                    pUseStack->variables.insert(pExpr->args[0]->token);
                }
            }
            return AnalyzeExpression(pUseStack, stack, pExpr->args[1]);
        }

        if (pExpr->isArray) {
            AnalyzeArrayExpression(pUseStack, stack, pExpr);
            return;
        }

        if (pExpr->isCall) {
            if (auto&& pFunction = FindFunction(pExpr->token)) {
                pUseStack->forceUsedFunctions.insert(pExpr->token);
            }

            for (auto&& pSubExpr : pExpr->args) {
                AnalyzeExpression(pUseStack, stack, pSubExpr);
            }

            return;
        }

        if (IsIdentifier(pExpr->token) && !pExpr->token.empty()) {
            pUseStack->variables.insert(pExpr->token);
        }

        for (auto&& pSubExpr : pExpr->args) {
            AnalyzeExpression(pUseStack, stack, pSubExpr);
        }
    }

    void SRSLRefAnalyzer::AnalyzeIfStatement(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLIfStatement* pIfStatement) {
        if (pIfStatement->pExpr) {
            AnalyzeExpression(pUseStack, stack, pIfStatement->pExpr);
        }

        if (pIfStatement->pLexicalTree) {
            pUseStack->Concat(AnalyzeTree(stack, pIfStatement->pLexicalTree));
        }
    }

    SRSLFunction *SRSLRefAnalyzer::FindFunction(const std::string &name) const {
        return FindFunction(m_analyzedTree->pLexicalTree, name);
    }

    SRSLFunction *SRSLRefAnalyzer::FindFunction(SRSLLexicalTree* pTree, const std::string &name) const {
        if (!pTree) {
            SRHalt("SRSLRefAnalyzer::FindFunction() : pTree is nullptr!");
            return nullptr;
        }

        for (auto&& pUnit : pTree->lexicalTree) {
            if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                if (pFunction->pName->token == name) {
                    return pFunction;
                }
            }
            else if (auto&& pSubTree = dynamic_cast<SRSLLexicalTree*>(pUnit)) {
                if (pSubTree == pTree) {
                    SRHalt("SRSLRefAnalyzer::FindFunction() : pSubTree is equal to pTree! This is a bug!");
                    return nullptr;
                }

                if (auto&& pFoundedFunction = FindFunction(pSubTree, name)) {
                    return pFoundedFunction;
                }
            }
        }

        return nullptr;
    }

    void SRSLRefAnalyzer::AnalyzeArrayExpression(SRSLUseStack::Ptr& pUseStack, std::list<std::string> &stack, SRSLExpr* pExpr) {
        AnalyzeExpression(pUseStack, stack, pExpr->args[0]);
        if (pExpr->args.size() == 2) {
            AnalyzeExpression(pUseStack, stack, pExpr->args[1]);
        }
    }

    void SRSLRefAnalyzer::AnalyzeFunction(SRSLUseStack::Ptr &pUseStack, std::list<std::string> &stack, SRSLFunction *pFunction) {
        if (pFunction->pLexicalTree) {
            pUseStack->functions[pFunction->GetName()] = AnalyzeTree(stack, pFunction->pLexicalTree);
        }
        else {
            SRHalt("EntryPoint function must have a body!");
        }
    }

    void SRSLRefAnalyzer::AnalyzeWhileStatement(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLWhileStatement* pWhileStatement) {
        if (pWhileStatement->pCondition) {
            AnalyzeExpression(pUseStack, stack, pWhileStatement->pCondition);
        }
        if (pWhileStatement->pLexicalTree) {
            pUseStack->Concat(AnalyzeTree(stack, pWhileStatement->pLexicalTree));
        }
    }

    void SRSLRefAnalyzer::AnalyzeForStatement(SRSLUseStack::Ptr &pUseStack, std::list<std::string> &stack, SRSLForStatement *pForStatement) {
        if (pForStatement->pVar) {
            AnalyzeVariable(pUseStack, stack, pForStatement->pVar);
        }

        if (pForStatement->pCondition) {
            AnalyzeExpression(pUseStack, stack, pForStatement->pCondition);
        }

        if (pForStatement->pExpr) {
            AnalyzeExpression(pUseStack, stack, pForStatement->pExpr);
        }

        if (pForStatement->pLexicalTree) {
            pUseStack->Concat(AnalyzeTree(stack, pForStatement->pLexicalTree));
        }
    }

    void SRSLRefAnalyzer::AnalyzeVariable(SRSLUseStack::Ptr &pUseStack, std::list<std::string> &stack, SRSLVariable *pVariable) {
        if (pVariable->pType) {
            AnalyzeExpression(pUseStack, stack, pVariable->pType);
        }

        if (pVariable->pName) {
            AnalyzeExpression(pUseStack, stack, pVariable->pName);
        }

        if (pVariable->pExpr) {
            AnalyzeExpression(pUseStack, stack, pVariable->pExpr);
        }
    }
}
