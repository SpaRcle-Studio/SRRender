//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/GLSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>

namespace SR_SRSL_NS {
    SR_UTILS_NS::String GLSLGenerator::GenerateVariable(const SRSLVariable* pVariable, int32_t deep) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(128);
        AppendVariable(code, pVariable, deep);
        return code;
    }

    void GLSLGenerator::AppendVariable(SR_UTILS_NS::String& code, const SRSLVariable* pVariable, int32_t deep) const {
        SR_TRACY_ZONE;
        code += GenerateTab(deep);

        if (pVariable->pType) {
            AppendType(code, pVariable->pType, 0);
            code += " ";
        }

        if (pVariable->pName) {
            AppendName(code, pVariable->pName, 0);
        }

        if (pVariable->pExpr) {
            code += " = ";
            AppendExpression(code, pVariable->pExpr);
        }

        return;
    }

    void GLSLGenerator::AppendFunction(SR_UTILS_NS::String& code, const SRSLFunction* pFunction, int32_t deep) const {
        SR_TRACY_ZONE;
        AppendFunction(code, pFunction, deep, SR_UTILS_NS::String(), SR_UTILS_NS::String());
        return;
    }

    void GLSLGenerator::AppendFunction(SR_UTILS_NS::String& code, const SRSLFunction* pFunction, int32_t deep,
        const SR_UTILS_NS::String& preCode, const SR_UTILS_NS::String& postCode
    ) const {
        SR_TRACY_ZONE;

        code += GenerateTab(deep);

        if (pFunction->pType) {
            AppendType(code, pFunction->pType, 0);
            code += " ";
        }

        if (pFunction->pName) {
            if (IsShaderEntryPoint(pFunction->GetName())) {
                code += "main";
            } else {
                AppendType(code, pFunction->pName, 0);
            }
        }

        code += "(";

        for (uint32_t i = 0; i < pFunction->args.size(); ++i) {
            AppendVariable(code, pFunction->args[i], 0);

            if (i + 1 < pFunction->args.size()) {
                code += ", ";
            }
        }

        code += ") ";

        if (pFunction->pLexicalTree) {
            AppendLexicalTree(code, pFunction->pLexicalTree, deep, preCode, postCode);
        }

        return;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateStructure(const SRSLStructureStatement* pStructure, int32_t deep) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(512);
        AppendStructure(code, pStructure, deep);
        return code;
    }

    void GLSLGenerator::AppendStructure(SR_UTILS_NS::String& code, const SRSLStructureStatement* pStructure, int32_t deep) const {
        SR_TRACY_ZONE;
        if (pStructure->HasDynamicArray()) {
            return;
        }

        code += GenerateTab(deep);
        code += "struct ";
        AppendName(code, pStructure->pName, 0);
        code += " ";

        if (pStructure->pLexicalTree) {
            AppendLexicalTree(code, pStructure->pLexicalTree, deep);
        }

        code += ";";

        return;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateType(const SRSLExpr* pExpr, int32_t deep) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(128);
        AppendType(code, pExpr, deep);
        return code;
    }

    void GLSLGenerator::AppendType(SR_UTILS_NS::String& code, const SRSLExpr* pExpr, int32_t deep) const {
        SR_TRACY_ZONE;
        AppendExpression(code, pExpr, deep);
        return;
    }

    void GLSLGenerator::AppendName(SR_UTILS_NS::String& code, const SRSLExpr* pExpr, int32_t deep) const {
        SR_TRACY_ZONE;
        AppendExpression(code, pExpr, deep);
        return;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateExpression(const SRSLExpr* expression, int32_t deep) const {
        SR_TRACY_ZONE;
        if (!expression) {
            return {};
        }
        SR_UTILS_NS::String code;
        code.reserve(128);
        AppendExpression(code, expression, deep);
        return code;
    }

    void GLSLGenerator::AppendExpression(SR_UTILS_NS::String& code, const SRSLExpr* expression, int32_t deep) const {
        SR_TRACY_ZONE;
        if (!expression) {
            return;
        }
        if ((expression->token == "++" || expression->token == "--") && !expression->args.empty()) {
            SRHalt0();
            return;
        }
        code += GenerateTab(deep);
        auto emit = [&](const SRSLExpr* child) { AppendExpression(code, child); };
        if (CodeGenDetail::AppendCommonExpression(code, expression, emit)) {
            return;
        }
        const auto& token = expression->token;
        const auto& args = expression->args;
        if (expression->isCall) {
            code += token;
            code += '(';
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) {
                    code += ", ";
                }
                emit(args[i]);
            }
            code += ')';
        } else if (expression->isArray) {
            if (args.size() == 1) {
                emit(args[0]);
                code += "[]";
            } else {
                SRHalt("GLSLGenerator: invalid array expression in {}", m_shader->GetPath());
            }
        } else if (expression->isList) {
            code += "{ ";
            for (size_t i = 0; i < args.size(); ++i) {
                code += '\n';
                AppendExpression(code, args[i], deep + 2);
                if (i + 1 < args.size()) {
                    code += ',';
                }
            }
            code += '\n';
            code += GenerateTab(deep + 1);
            code += '}';
        } else if (args.empty()) {
            code += token;
        } else if (args.size() == 2 && (token == "=" || token == "." || token == "?")) {
            emit(args[0]);
            if (token != ".") {
                code += ' ';
            }
            code += token;
            if (token != ".") {
                code += ' ';
            }
            emit(args[1]);
        } else if (args.size() == 2 && token.empty()) {
            emit(args[0]);
            emit(args[1]);
        } else if (args.size() == 2) {
            code += '(';
            emit(args[0]);
            code += ' ';
            code += token;
            code += ' ';
            emit(args[1]);
            code += ')';
        }
    }

    void GLSLGenerator::AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep) const {
        SR_TRACY_ZONE;
        AppendLexicalTree(code, pLexicalTree, deep, SR_UTILS_NS::String(), SR_UTILS_NS::String());
        return;
    }

    void GLSLGenerator::AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep,
        const SR_UTILS_NS::String& preCode,
        const SR_UTILS_NS::String& postCode
    ) const {
        SR_TRACY_ZONE;

        if (deep >= 0) {
            code += "{\n";
        }

        if (!preCode.empty()) {
            CodeGenDetail::Append(code, preCode, "\n");
        }

        for (uint32_t i = 0; i < pLexicalTree->lexicalTree.size(); ++i) {
            CodeGenDetail::VisitStatement(pLexicalTree->lexicalTree[i], [&](auto* node) {
                using T = std::remove_cv_t<std::remove_pointer_t<decltype(node)>>;
                if constexpr (std::is_same_v<T, SRSLVariable>) {
                    AppendVariable(code, node, deep + 1);
                    code += ";";
                } else if constexpr (std::is_same_v<T, SRSLFunction>) {
                    AppendFunction(code, node, deep + 1);
                    code += "\n";
                } else if constexpr (std::is_same_v<T, SRSLLexicalTree>) {
                    AppendLexicalTree(code, node, deep + 1);
                } else if constexpr (std::is_same_v<T, SRSLExpr>) {
                    AppendExpression(code, node, deep + 1);
                    code += ';';
                } else if constexpr (std::is_same_v<T, SRSLIfStatement>) {
                    AppendIfStatement(code, node, deep + 1);
                } else if constexpr (std::is_same_v<T, SRSLReturn>) {
                    code += GenerateTab(deep + 1);
                    code += "return ";
                    AppendExpression(code, node->pExpr);
                    code += ";\n";
                } else if constexpr (std::is_same_v<T, SRSLForStatement>) {
                    AppendForStatement(code, node, deep + 1);
                } else if constexpr (std::is_same_v<T, SRSLWhileStatement>) {
                    AppendWhileStatement(code, node, deep + 1);
                }
            });

            if (i + 1 < pLexicalTree->lexicalTree.size()) {
                code += "\n";
            }
        }

        if (deep >= 0) {
            code += "\n";
        }

        if (!postCode.empty()) {
            CodeGenDetail::Append(code, "\n", postCode);
        }

        if (deep >= 0) {
            CodeGenDetail::Append(code, GenerateTab(deep), "}");
        }

        return;
    }

    void GLSLGenerator::AppendIfStatement(SR_UTILS_NS::String& code, const SRSLIfStatement* pIfStatement, int32_t deep) const {
        SR_TRACY_ZONE;

        code += GenerateTab(deep);

        if (!pIfStatement->isElse && pIfStatement->pExpr) {
            code += "if";
        } else if (pIfStatement->isElse && pIfStatement->pExpr) {
            code += "else if";
        } else if (pIfStatement->isElse && !pIfStatement->pExpr) {
            code += "else";
        } else {
            SRHalt0();
        }

        if (pIfStatement->pExpr) {
            code += " (";
            AppendExpression(code, pIfStatement->pExpr, 0);
            code += ")";
        }

        if (pIfStatement->pLexicalTree) {
            code += " ";
            AppendLexicalTree(code, pIfStatement->pLexicalTree, deep);
        }

        return;
    }

    void GLSLGenerator::AppendWhileStatement(SR_UTILS_NS::String& code, const SRSLWhileStatement* pWhileStatement, int32_t deep) const {
        SR_TRACY_ZONE;

        CodeGenDetail::Append(code, GenerateTab(deep), "while (");

        if (pWhileStatement->pCondition) {
            AppendExpression(code, pWhileStatement->pCondition, 0);
            code += ") ";
        }

        if (pWhileStatement->pLexicalTree) {
            AppendLexicalTree(code, pWhileStatement->pLexicalTree, deep);
        }

        return;
    }

    void GLSLGenerator::AppendForStatement(SR_UTILS_NS::String& code, const SRSLForStatement* pForStatement, int32_t deep) const {
        SR_TRACY_ZONE;

        CodeGenDetail::Append(code, GenerateTab(deep), "for (");

        if (pForStatement->pVar) {
            AppendVariable(code, pForStatement->pVar, 0);
            code += "; ";
        }

        if (pForStatement->pCondition) {
            AppendExpression(code, pForStatement->pCondition, 0);
            code += "; ";
        }

        if (pForStatement->pExpr) {
            AppendExpression(code, pForStatement->pExpr, 0);
            code += ") ";
        }

        if (pForStatement->pLexicalTree) {
            AppendLexicalTree(code, pForStatement->pLexicalTree, deep);
        }

        return;
    }

    SR_UTILS_NS::StringView GLSLGenerator::ReplaceToken(SR_UTILS_NS::StringView token, bool isUniform) const {
        SR_TRACY_ZONE;
        if (token == "bool" && isUniform) {
            return "int";
        }
        return token;
    }
} // namespace SR_SRSL_NS
