//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    void WGSLGenerator::AppendStatement(SR_UTILS_NS::String& code, const SRSLLexicalUnit* unit, int32_t deep, SR_UTILS_NS::StringView returnCode, bool linewise) const {
        SR_TRACY_ZONE;
        CodeGenDetail::VisitStatement(unit, [&](auto* node) {
            using T = std::remove_cv_t<std::remove_pointer_t<decltype(node)>>;
            if constexpr (std::is_same_v<T, SRSLVariable>) {
                AppendVariable(code, node, deep);
                code += linewise ? ";\n" : ";";
            } else if constexpr (std::is_same_v<T, SRSLFunction>) {
                AppendFunctionBody(code, node, deep);
                code += '\n';
            } else if constexpr (std::is_same_v<T, SRSLLexicalTree>) {
                AppendLexicalTree(code, node, deep, returnCode);
            } else if constexpr (std::is_same_v<T, SRSLExpr>) {
                const size_t start = code.size();
                AppendExpression(code, node, deep);
                size_t begin = start;
                while (begin < code.size() && (code[begin] == ' ' || code[begin] == '\t')) {
                    ++begin;
                }
                // The parser may place an assignment under a binary node.
                if (begin < code.size() && code[begin] == '(' && code.back() == ')' &&
                    code.find(" = ", begin + 1) != SR_UTILS_NS::String::npos) {
                    code.erase(code.size() - 1, 1);
                    code.erase(begin, 1);
                }
                code += linewise ? ";\n" : ";";
            } else if constexpr (std::is_same_v<T, SRSLIfStatement>) {
                AppendIfStatement(code, node, deep, returnCode);
                if (linewise) {
                    code += '\n';
                }
            } else if constexpr (std::is_same_v<T, SRSLForStatement>) {
                AppendForStatement(code, node, deep, returnCode);
                if (linewise) {
                    code += '\n';
                }
            } else if constexpr (std::is_same_v<T, SRSLWhileStatement>) {
                AppendWhileStatement(code, node, deep, returnCode);
            } else if constexpr (std::is_same_v<T, SRSLReturn>) {
                if (!node->pExpr && !returnCode.empty()) {
                    code += returnCode;
                    if (returnCode.back() != '\n') {
                        code += '\n';
                    }
                } else {
                    code += GenerateTab(deep);
                    code += "return";
                    if (node->pExpr) {
                        code += ' ';
                        AppendExpression(code, node->pExpr);
                    }
                    code += ";\n";
                }
            }
        });
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateVariable(const SRSLVariable* pVariable, int32_t deep) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(128);
        AppendVariable(code, pVariable, deep);
        return code;
    }

    void WGSLGenerator::AppendVariable(SR_UTILS_NS::String& code, const SRSLVariable* pVariable, int32_t deep) const {
        SR_TRACY_ZONE;
        code += GenerateTab(deep);
        SR_UTILS_NS::String variableName(&m_scratch);
        SR_UTILS_NS::String arrayType;
        SR_UTILS_NS::StringView variableType;

        if (pVariable->pName) {
            m_tmpBuffer.clear();
            const auto* name = pVariable->pName->isArray && !pVariable->pName->args.empty()
                                   ? pVariable->pName->args[0]
                                   : pVariable->pName;
            const auto rawName = name->ToString(0, m_tmpBuffer);
            variableName.reserve(rawName.size() + 1);
            WGSLDetail::AppendIdentifier(variableName, rawName);

            // The parser may keep the declarator dimension in the first name
            // token (for example, "uv[4]"). WGSL puts the dimension into the
            // type, so it must not remain part of the identifier.
            if (const auto arrayStart = variableName.find('['); arrayStart != SR_UTILS_NS::String::npos) {
                variableName.resize(arrayStart);
            }
        }

        if (pVariable->pType) {
            m_tmpBuffer.clear();
            const SR_UTILS_NS::StringView rawType = pVariable->pType->ToString(0, m_tmpBuffer);
            const auto* tree = m_pCurrentShader ? m_pCurrentShader->GetAnalyzedTree() : nullptr;
            const auto baseType = WGSLDetail::GenerateType(DescribeType(rawType, tree).base);
            const auto dimensions =
                pVariable->pName && pVariable->pName->isArray
                    ? SRSLTypeInfo::Instance().GetDimension(
                          pVariable->pName, m_pCurrentShader ? m_pCurrentShader->GetAnalyzedTree() : nullptr)
                    : SR_UTILS_NS::Vector<uint64_t>();
            if (dimensions.empty()) {
                variableType = baseType;
            } else {
                arrayType = WGSLDetail::ToWGSLArrayType(baseType, dimensions);
                variableType = arrayType;
            }
        }

        // WGSL variable declaration: var name : type = expr;
        code += "var ";

        code += variableName;
        if (!variableType.empty()) {
            CodeGenDetail::Append(code, " : ", variableType);
            if (!variableName.empty()) {
                RegisterVariableType(variableName, variableType);
            }
        }

        if (pVariable->pExpr) {
            if (!variableType.empty() && pVariable->pName && pVariable->pName->isArray && pVariable->pExpr->isList) {
                CodeGenDetail::Append(code, " = ", variableType, "(");
                for (uint32_t i = 0; i < pVariable->pExpr->args.size(); ++i) {
                    if (i > 0) {
                        code += ", ";
                    }
                    AppendExpression(code, pVariable->pExpr->args[i], 0);
                }
                code += ")";
            } else {
                code += " = ";
                AppendExpression(code, pVariable->pExpr, 0);
            }
        }

        return;
    }

    void WGSLGenerator::AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep,
        SR_UTILS_NS::StringView returnCode
    ) const {
        SR_TRACY_ZONE;
        const Scope scope(*this);
        AppendLexicalTree(code, pLexicalTree, deep, SR_UTILS_NS::String(), SR_UTILS_NS::String(), returnCode);
        return;
    }

    void WGSLGenerator::AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep,
        const SR_UTILS_NS::String& preCode, const SR_UTILS_NS::String& postCode,
        SR_UTILS_NS::StringView returnCode
    ) const {
        SR_TRACY_ZONE;
        const Scope scope(*this);

        if (deep >= 0) {
            code += "{\n";
        }

        if (!preCode.empty()) {
            CodeGenDetail::Append(code, preCode, "\n");
        }

        for (uint32_t i = 0; i < pLexicalTree->lexicalTree.size(); ++i) {
            AppendStatement(code, pLexicalTree->lexicalTree[i], deep + 1, returnCode, false);

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

    void WGSLGenerator::AppendIfStatement(SR_UTILS_NS::String& code, const SRSLIfStatement* pIfStatement, int32_t deep,
                                          SR_UTILS_NS::StringView returnCode) const {
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
            const bool previousForceExplicitLod = m_forceExplicitTextureLod;
            m_forceExplicitTextureLod = true;
            code += " ";
            AppendLexicalTree(code, pIfStatement->pLexicalTree, deep, returnCode);
            m_forceExplicitTextureLod = previousForceExplicitLod;
        }

        return;
    }

    void WGSLGenerator::AppendForStatement(SR_UTILS_NS::String& code, const SRSLForStatement* pForStatement,
                                           int32_t deep, SR_UTILS_NS::StringView returnCode) const {
        SR_TRACY_ZONE;
        const Scope scope(*this);

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
            const bool previousForceExplicitLod = m_forceExplicitTextureLod;
            m_forceExplicitTextureLod = true;
            AppendLexicalTree(code, pForStatement->pLexicalTree, deep, returnCode);
            m_forceExplicitTextureLod = previousForceExplicitLod;
        }

        return;
    }

    void WGSLGenerator::AppendWhileStatement(SR_UTILS_NS::String& code, const SRSLWhileStatement* pWhileStatement,
                                             int32_t deep, SR_UTILS_NS::StringView returnCode) const {
        SR_TRACY_ZONE;
        const Scope scope(*this);

        CodeGenDetail::Append(code, GenerateTab(deep), "loop {\n");

        if (pWhileStatement->pCondition) {
            code += GenerateTab(deep + 1);
            code += "if (!(";
            AppendExpression(code, pWhileStatement->pCondition, 0);
            code += ")) { break; }\n";
        }

        if (pWhileStatement->pLexicalTree) {
            const bool previousForceExplicitLod = m_forceExplicitTextureLod;
            m_forceExplicitTextureLod = true;
            // Emit body without braces since we already have the outer loop block
            for (const auto* unit : pWhileStatement->pLexicalTree->lexicalTree) {
                AppendStatement(code, unit, deep + 1, returnCode, true);
            }
            m_forceExplicitTextureLod = previousForceExplicitLod;
        }

        CodeGenDetail::Append(code, GenerateTab(deep), "}\n");
        return;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateStructure(const SRSLStructureStatement* pStructure, int32_t deep) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(512);
        AppendStructure(code, pStructure, deep);
        return code;
    }

    void WGSLGenerator::AppendStructure(SR_UTILS_NS::String& code, const SRSLStructureStatement* pStructure,
                                        int32_t deep) const {
        SR_TRACY_ZONE;
        if (pStructure->HasDynamicArray()) {
            return;
        }

        code += GenerateTab(deep);
        code += "struct ";

        if (pStructure->pName) {
            code += pStructure->pName->token;
        }

        code += " {\n";

        if (pStructure->pLexicalTree) {
            for (auto&& pUnit : pStructure->pLexicalTree->lexicalTree) {
                if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                    SR_UTILS_NS::String fieldName;
                    SR_UTILS_NS::String fieldType;
                    if (pVariable->pName) {
                        m_tmpBuffer.clear();
                        fieldName = pVariable->pName->ToString(0, m_tmpBuffer);
                    }
                    if (pVariable->pType) {
                        m_tmpBuffer.clear();
                        fieldType = WGSLDetail::GenerateType(pVariable->pType->ToString(0, m_tmpBuffer));
                    }
                    if (!fieldType.empty() && !fieldName.empty()) {
                        CodeGenDetail::Append(code, GenerateTab(deep + 1), fieldName, " : ", fieldType, ",\n");
                    }
                }
            }
        }

        CodeGenDetail::Append(code, GenerateTab(deep), "}");
        return;
    }

    void WGSLGenerator::AppendFunctionBody(SR_UTILS_NS::String& code, const SRSLFunction* pFunction,
                                           int32_t deep) const {
        SR_TRACY_ZONE;
        const Scope scope(*this);
        SR_UTILS_NS::String parameterAliases;
        code += GenerateTab(deep);
        code += "fn "; // WGSL requires "fn" keyword for every function definition

        if (pFunction->pName) {
            if (IsShaderEntryPoint(pFunction->GetName())) {
                code += pFunction->GetName().data();
            } else {
                code += WGSLDetail::SafeIdentifier(pFunction->pName->token);
            }
        }

        code += "(";

        for (uint32_t i = 0; i < pFunction->args.size(); ++i) {
            auto&& pArg = pFunction->args[i];
            SR_UTILS_NS::String argName, argType;
            if (pArg->pName) {
                m_tmpBuffer.clear();
                argName = WGSLDetail::SafeIdentifier(SR_UTILS_NS::String(pArg->pName->ToString(0, m_tmpBuffer)));
            }
            if (pArg->pType) {
                m_tmpBuffer.clear();
                const SR_UTILS_NS::StringView rawType = pArg->pType->ToString(0, m_tmpBuffer);
                argType = WGSLDetail::GenerateType(rawType);
                if (rawType == "sampler2D")
                    argType = "texture_2d<f32>";
                else if (rawType == "sampler2DArray")
                    argType = "texture_2d_array<f32>";
                else if (rawType == "sampler2DArrayDepth")
                    argType = "texture_depth_2d_array";
                else if (rawType == "sampler3D")
                    argType = "texture_3d<f32>";
                else if (rawType == "samplerCube")
                    argType = "texture_cube<f32>";
                else if (rawType == "sampler2DShadow")
                    argType = "texture_depth_2d";
                if (rawType == "sampler2D" || rawType == "sampler2DArray" || rawType == "sampler2DArrayDepth" ||
                    rawType == "sampler3D" || rawType == "samplerCube" || rawType == "sampler2DShadow") {
                    m_samplerParameterNames.emplace_back(argName);
                }
            }
            RegisterVariableType(argName, argType);
            const bool isTexture = argType.find("texture_") == 0;
            if (isTexture) {
                CodeGenDetail::Append(code, argName, " : ", argType);
                CodeGenDetail::Append(code, ", ", argName, "_sampler : sampler");
            } else {
                SR_UTILS_NS::String immutableArgName;
                immutableArgName.reserve(8 + argName.size());
                CodeGenDetail::Append(immutableArgName, "_sr_arg_", argName);
                CodeGenDetail::Append(code, immutableArgName, " : ", argType);
                CodeGenDetail::Append(parameterAliases, GenerateTab(deep + 1), "var ", argName, " : ", argType, " = ",
                                      immutableArgName, ";\n");
            }
            if (i + 1 < pFunction->args.size()) {
                code += ", ";
            }
        }

        code += ")";

        if (pFunction->pType) {
            m_tmpBuffer.clear();
            SR_UTILS_NS::String retType = WGSLDetail::GenerateType(pFunction->pType->ToString(0, m_tmpBuffer));
            if (!retType.empty() && retType != "void") {
                CodeGenDetail::Append(code, " -> ", retType);
            }
        }

        if (pFunction->pLexicalTree) {
            SR_UTILS_NS::String preCode = parameterAliases;
            if (m_pCurrentShader && !IsShaderEntryPoint(pFunction->GetName())) {
                if (auto&& pUseStackFunction = m_pCurrentShader->GetUseStack()->FindFunction(pFunction->GetName())) {
                    auto isDeclared = [&](auto&& self, const SRSLLexicalTree* pTree,
                                          SR_UTILS_NS::StringView name) -> bool {
                        if (!pTree) {
                            return false;
                        }
                        for (auto&& pUnit : pTree->lexicalTree) {
                            if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                                if (pVariable->pName && pVariable->pName->GetAsName() == name) {
                                    return true;
                                }
                            } else if (auto&& pNestedTree = dynamic_cast<SRSLLexicalTree*>(pUnit)) {
                                if (self(self, pNestedTree, name)) {
                                    return true;
                                }
                            } else if (auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(pUnit)) {
                                if (self(self, pIfStatement->pLexicalTree, name)) {
                                    return true;
                                }
                            } else if (auto&& pForStatement = dynamic_cast<SRSLForStatement*>(pUnit)) {
                                if ((pForStatement->pVar && pForStatement->pVar->pName &&
                                     pForStatement->pVar->pName->GetAsName() == name) ||
                                    self(self, pForStatement->pLexicalTree, name)) {
                                    return true;
                                }
                            } else if (auto&& pWhileStatement = dynamic_cast<SRSLWhileStatement*>(pUnit)) {
                                if (self(self, pWhileStatement->pLexicalTree, name)) {
                                    return true;
                                }
                            }
                        }
                        return false;
                    };

                    for (auto&& [blockName, uniformBlock] : m_pCurrentShader->GetUniformBlocks()) {
                        for (auto&& field : uniformBlock.fields) {
                            bool isUsed = false;
                            for (auto&& variable : pUseStackFunction->variables) {
                                isUsed = isUsed || variable == field.name;
                            }
                            if (!isUsed) {
                                continue;
                            }

                            bool isShadowed = false;
                            for (auto&& arg : pFunction->args) {
                                isShadowed = isShadowed || (arg->pName && arg->pName->GetAsName() == field.name);
                            }
                            if (isShadowed || isDeclared(isDeclared, pFunction->pLexicalTree, field.name)) {
                                continue;
                            }

                            const SR_UTILS_NS::StringView typeName = GetResourceType(field.type);
                            if (!typeName.empty()) {
                                SR_UTILS_NS::FormatTo(
                                    preCode, "{}let {} : {} = {}.{};\n", GenerateTab(deep + 1),
                                    field.name.ToStringView(), typeName, blockName.ToStringView(),
                                    WGSLDetail::SafeIdentifier(field.name.ToStringView()));
                            }
                        }
                    }
                }
            }
            code += " ";
            AppendLexicalTree(code, pFunction->pLexicalTree, deep, preCode, SR_UTILS_NS::String());
        }

        return;
    }

    void WGSLGenerator::AppendFunction(SR_UTILS_NS::String& result, SRSLFunction* pFunction, const int32_t deep,
                                       const SR_UTILS_NS::StringView& preArgs, const SR_UTILS_NS::StringView& preCode,
                                       const SR_UTILS_NS::StringView& postCode,
                                       const SR_UTILS_NS::StringView& returnType) {
        SR_TRACY_ZONE;
        const Scope scope(*this);

        if (!pFunction) {
            return;
        }

        result += GenerateTab(deep);
        result += "fn ";
        result += pFunction->GetName();
        result += "(";

        result += preArgs;
        if (!preArgs.empty() && !pFunction->args.empty()) {
            result += ", ";
        }

        for (size_t i = 0; i < pFunction->args.size(); ++i) {
            auto&& parameter = pFunction->args[i];
            SR_UTILS_NS::String argName, argType;
            if (parameter->pName) {
                m_tmpBuffer.clear();
                argName = parameter->pName->ToString(0, m_tmpBuffer);
            }
            if (parameter->pType) {
                m_tmpBuffer.clear();
                argType = WGSLDetail::GenerateType(parameter->pType->ToString(0, m_tmpBuffer));
            }
            CodeGenDetail::Append(result, argName, " : ", argType);
            if (i + 1 < pFunction->args.size()) {
                result += ", ";
            }
        }

        m_tmpBuffer.clear();
        SR_UTILS_NS::String retTypeStr;
        if (!returnType.empty()) {
            retTypeStr = SR_UTILS_NS::String(returnType);
        } else if (pFunction->pType) {
            retTypeStr = WGSLDetail::GenerateType(pFunction->pType->ToString(0, m_tmpBuffer));
        }

        if (!retTypeStr.empty() && retTypeStr != "void") {
            result += ") -> ";
            result += retTypeStr;
        } else {
            result += ") ";
        }
        result += " {\n";

        if (!preCode.empty()) {
            result += preCode;
        }

        const SR_UTILS_NS::StringView stageReturnCode = returnType.empty() ? SR_UTILS_NS::StringView() : postCode;

        // Emit function body from AST
        if (pFunction->pLexicalTree) {
            for (const auto* unit : pFunction->pLexicalTree->lexicalTree) {
                AppendStatement(result, unit, deep + 1, stageReturnCode, true);
            }
        }

        if (!postCode.empty()) {
            result += postCode;
        }

        result += GenerateTab(deep);
        result += "}\n";

        return;
    }
} // namespace SR_SRSL_NS
