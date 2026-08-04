//
// Created by Monika on 11.04.2026.
//

#include <Graphics/SRSL/WGSLCodeGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/TypeInfo.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <unordered_set>

namespace SR_SRSL_NS {
    namespace WGSLDetail {
        /// Map a SRSL/GLSL type name to its WGSL equivalent.
        /// Returns the input unchanged if no mapping is found (handles user-defined struct types).
        std::string GenerateType(std::string_view type) {
            // ---- scalars ----
            if (type == "float")  return "f32";
            if (type == "double") return "f32"; // WGSL has no f64 in shaders
            if (type == "int")    return "i32";
            if (type == "uint")   return "u32";
            if (type == "bool")   return "bool";

            // ---- float vectors ----
            if (type == "vec2" || type == "vec2f") return "vec2<f32>";
            if (type == "vec3" || type == "vec3f") return "vec3<f32>";
            if (type == "vec4" || type == "vec4f") return "vec4<f32>";

            // ---- int vectors ----
            if (type == "ivec2") return "vec2<i32>";
            if (type == "ivec3") return "vec3<i32>";
            if (type == "ivec4") return "vec4<i32>";

            // ---- uint vectors ----
            if (type == "uvec2") return "vec2<u32>";
            if (type == "uvec3") return "vec3<u32>";
            if (type == "uvec4") return "vec4<u32>";

            // ---- bool vectors ----
            if (type == "bvec2") return "vec2<bool>";
            if (type == "bvec3") return "vec3<bool>";
            if (type == "bvec4") return "vec4<bool>";

            // ---- matrices (GLSL column-major → WGSL matCxR) ----
            if (type == "mat2" || type == "mat2x2") return "mat2x2<f32>";
            if (type == "mat3" || type == "mat3x3") return "mat3x3<f32>";
            if (type == "mat4" || type == "mat4x4") return "mat4x4<f32>";
            if (type == "mat2x3") return "mat2x3<f32>";
            if (type == "mat2x4") return "mat2x4<f32>";
            if (type == "mat3x2") return "mat3x2<f32>";
            if (type == "mat3x4") return "mat3x4<f32>";
            if (type == "mat4x2") return "mat4x2<f32>";
            if (type == "mat4x3") return "mat4x3<f32>";

            // ---- void ----
            if (type == "void") return "void";

            // ---- samplers — not a direct type in WGSL, but we return empty so callers skip ----
            if (type == "sampler2D")       return std::string();
            if (type == "sampler3D")       return std::string();
            if (type == "samplerCube")     return std::string();
            if (type == "sampler2DShadow") return std::string();

            // ---- unknown / user struct → keep as-is ----
            return std::string(type);
        }

        std::string_view VertexAttributeFormatToString(SR_UTILS_NS::VertexAttributeFormat format, uint8_t count) {
            static std::string result;
            result.clear();

            switch (format) {
                case SR_UTILS_NS::VertexAttributeFormat::Float32:
                case SR_UTILS_NS::VertexAttributeFormat::R11G11B10_Float:
                    result = "f32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Int32:   result = "i32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UInt32:  result = "u32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Float16: result = "f16"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Int16:   result = "i32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UInt16:  result = "u32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Int8:    result = "i32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UInt8:   result = "u32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UNorm8:
                case SR_UTILS_NS::VertexAttributeFormat::SNorm8:  result = "f32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UNorm16:
                case SR_UTILS_NS::VertexAttributeFormat::SNorm16: result = "f32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::R10G10B10A2_UNorm: result = "u32"; break;
                default:
                    SRHalt0();
                    break;
            }

            if (count > 1) {
                switch (count) {
                    case 2: result = "vec2<" + result + ">"; break;
                    case 3: result = "vec3<" + result + ">"; break;
                    case 4: result = "vec4<" + result + ">"; break;
                    default:
                        SRHalt0();
                        break;
                }
            }

            return result;
        }

        /// WGSL reserved keywords that must be renamed
        bool IsWGSLReservedKeyword(const std::string& token) {
            // https://www.w3.org/TR/WGSL/#reserved-words
            static const std::unordered_set<std::string> reserved = {
                "target", "result", "output", "input", "type", "module",
                "abstract", "active", "alignas", "alignof", "as", "asm",
                "asm_fragment", "async", "attribute", "auto", "await", "become",
                "binding_array", "cast", "catch", "class", "co_await", "co_return",
                "co_yield", "coherent", "column_major", "common", "compile",
                "compile_fragment", "concept", "const_cast", "consteval",
                "constexpr", "constinit", "crate", "debugger", "decltype",
                "delete", "demote", "demote_to_helper", "do", "dynamic_cast",
                "enum", "explicit", "export", "extends", "extern", "external",
                "fallthrough", "filter", "final", "finally", "friend", "from",
                "fxgroup", "get", "goto", "groupshared", "highp", "impl",
                "implements", "import", "inline", "instanceof", "interface",
                "layout", "lowp", "macro", "macro_rules", "match", "mediump",
                "meta", "mod", "modf", "move", "mut", "mutable", "namespace",
                "new", "nil", "noexcept", "noinline", "nointerpolation",
                "noperspective", "null", "nullptr", "of", "operator", "package",
                "packoffset", "partition", "pass", "patch", "pixelfragment",
                "precise", "precision", "premerge", "priv", "protected",
                "pub", "public", "readonly", "ref", "regardless", "register",
                "reinterpret_cast", "require", "resource", "restrict", "self",
                "set", "shared", "sizeof", "smooth", "snorm", "static",
                "static_assert", "static_cast", "std", "subroutine", "super",
                "target", "template", "this", "thread_local", "throw", "trait",
                "try", "typedef", "typeid", "typename", "typeof", "union",
                "unless", "unorm", "unsafe", "unsized", "use", "using",
                "varying", "virtual", "volatile", "wgsl", "where", "with",
                "writeonly", "yield"
            };
            return reserved.count(token) > 0;
        }

        std::string SafeIdentifier(const std::string& token) {
            if (IsWGSLReservedKeyword(token)) {
                return "_" + token;
            }
            return token;
        }

        /// Whether an operator token is a compound assignment in WGSL
        bool IsCompoundAssignment(const std::string& token) {
            return token == "+=" || token == "-=" || token == "*=" || token == "/="
                || token == "%=" || token == "&=" || token == "|=" || token == "^="
                || token == "<<=" || token == ">>=" || token == "||=" || token == "&&=";
        }

        /// Convert C-style array dimension string to WGSL array<type, N> syntax.
        /// Input: typeName e.g. "f32", dimensions e.g. {256} or {0} for runtime-sized
        std::string ToWGSLArrayType(const std::string& baseType, const std::vector<uint64_t>& dims) {
            if (dims.empty()) {
                return baseType;
            }
            // Build inside-out for multi-dimensional arrays
            std::string result = baseType;
            for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
                if (*it == 0) {
                    result = "array<" + result + ">";
                }
                else {
                    result = "array<" + result + ", " + std::to_string(*it) + ">";
                }
            }
            return result;
        }

        /// Replace a GLSL token with its WGSL equivalent for expressions.
        std::string ReplaceToken(const std::string& token) {
            // Type replacements inside expressions (constructors, casts)
            if (token == "float")  return "f32";
            if (token == "int")    return "i32";
            if (token == "uint")   return "u32";
            if (token == "vec2")   return "vec2<f32>";
            if (token == "vec3")   return "vec3<f32>";
            if (token == "vec4")   return "vec4<f32>";
            if (token == "ivec2")  return "vec2<i32>";
            if (token == "ivec3")  return "vec3<i32>";
            if (token == "ivec4")  return "vec4<i32>";
            if (token == "uvec2")  return "vec2<u32>";
            if (token == "uvec3")  return "vec3<u32>";
            if (token == "uvec4")  return "vec4<u32>";
            if (token == "bvec2")  return "vec2<bool>";
            if (token == "bvec3")  return "vec3<bool>";
            if (token == "bvec4")  return "vec4<bool>";
            if (token == "mat2")   return "mat2x2<f32>";
            if (token == "mat3")   return "mat3x3<f32>";
            if (token == "mat4")   return "mat4x4<f32>";

            // Builtin functions that differ
            if (token == "mix")         return "mix";
            if (token == "fract")       return "fract";
            if (token == "dFdx")        return "dpdx";
            if (token == "dFdy")        return "dpdy";
            if (token == "inversesqrt") return "inverseSqrt";
            if (token == "fma")         return "fma";
            if (token == "mod")         return ""; // handled in GenerateExpression as %
            if (token == "atan")        return "atan2";
            if (token == "texture")     return "textureSample";
            if (token == "textureLod")  return "textureSampleLevel";

            // GLSL builtins → WGSL equivalents
            if (token == "gl_Position")        return "vsOut.position";
            if (token == "gl_FragCoord")       return "fsIn.position";
            if (token == "gl_VertexIndex")     return "vertexIndex";
            if (token == "gl_GlobalInvocationID") return "global_id";
            if (token == "gl_LocalInvocationID")  return "local_id";
            if (token == "gl_WorkGroupID")     return "workgroup_id";
            if (token == "gl_NumWorkGroups")   return "num_workgroups";
            if (token == "gl_LocalInvocationIndex") return "local_index";

            return SafeIdentifier(token);
        }
    } // namespace WGSLDetail

    // ----------------------------------------------------------------------------------------------------------------
    // Expression / statement code generation
    // ----------------------------------------------------------------------------------------------------------------

    std::string WGSLCodeGenerator::GenerateExpression(const SRSLExpr* pExpr, int32_t deep) const {
        if (!pExpr) {
            return std::string();
        }

        std::string code = std::string(GenerateTab(deep));

        // WGSL does not support prefix ++ / --. Convert to += 1 / -= 1.
        if (pExpr->token == "++" || pExpr->token == "--") {
            const std::string op = (pExpr->token == "++") ? " += 1" : " -= 1";
            if (!pExpr->args.empty()) {
                // Post/prefix increment used as statement: var += 1
                code += GenerateExpression(pExpr->args[0], 0) + op;
            }
            else {
                // Standalone ++ / -- token (part of empty-token concatenation pair).
                // The parent expression with token=="" will combine this with the variable.
                code += (pExpr->token == "++") ? "__INC__" : "__DEC__";
            }
        }
        else if (pExpr->isCall) {
            const std::string funcName = WGSLDetail::ReplaceToken(pExpr->token);

            code += funcName + "(";
            for (uint32_t i = 0; i < pExpr->args.size(); ++i) {
                code += GenerateExpression(pExpr->args[i], 0);
                if (i + 1 < pExpr->args.size()) {
                    code += ", ";
                }
            }
            code += ")";
        }
        else if (pExpr->isArray) {
            if (pExpr->args.size() == 1) {
                // array type declaration `type[]` → handled elsewhere; here it's indexing with no index (shouldn't happen)
                code += GenerateExpression(pExpr->args[0], 0) + "[]";
            }
            else if (pExpr->args.size() == 2) {
                // indexing: a[i]
                code += GenerateExpression(pExpr->args[0], 0) + "[" + GenerateExpression(pExpr->args[1], 0) + "]";
            }
            else {
                SRHalt("WGSLCodeGenerator::GenerateExpression() : invalid array expression!");
            }
        }
        else if (pExpr->isList) {
            // Struct/array literal: use the type constructor style if we know the type,
            // otherwise emit as WGSL array<> initializer via array()
            code += "array(";
            for (uint32_t i = 0; i < pExpr->args.size(); ++i) {
                code += GenerateExpression(pExpr->args.at(i), 0);
                if (i + 1 < pExpr->args.size()) {
                    code += ", ";
                }
            }
            code += ")";
        }
        else if (pExpr->args.empty()) {
            code += WGSLDetail::ReplaceToken(pExpr->token);
        }
        else if (pExpr->args.size() == 1) {
            // Unary: e.g. !x, -x
            code += "(" + WGSLDetail::ReplaceToken(pExpr->token) + GenerateExpression(pExpr->args[0], 0) + ")";
        }
        else if (pExpr->args.size() == 2 && pExpr->token == ".") {
            // Member access: a.b
            code += GenerateExpression(pExpr->args[0], 0) + "." + GenerateExpression(pExpr->args[1], 0);
        }
        else if (pExpr->args.size() == 2 && (pExpr->token == "=" || WGSLDetail::IsCompoundAssignment(pExpr->token))) {
            // Assignment / compound-assignment: emit without wrapping parens (WGSL disallows assignment in expression)
            code += GenerateExpression(pExpr->args[0], 0) + " " + pExpr->token + " " + GenerateExpression(pExpr->args[1], 0);
        }
        else if (pExpr->args.size() == 2 && pExpr->token.empty()) {
            // Concatenation: used for prefix/postfix ++/-- patterns like { "++", i } or { i, "++" }
            // WGSL doesn't support ++ / --, so we detect and convert to += 1 / -= 1
            const std::string left  = GenerateExpression(pExpr->args[0], 0);
            const std::string right = GenerateExpression(pExpr->args[1], 0);

            if (left == "__INC__") {
                code += right + " += 1";
            }
            else if (left == "__DEC__") {
                code += right + " -= 1";
            }
            else if (right == "__INC__") {
                code += left + " += 1";
            }
            else if (right == "__DEC__") {
                code += left + " -= 1";
            }
            else {
                code += left + right;
            }
        }
        else if (pExpr->args.size() == 2 && (pExpr->token == "==" || pExpr->token == "!=")) {
            code += "((" + GenerateExpression(pExpr->args[0], 0) + ") " + pExpr->token + " (" + GenerateExpression(pExpr->args[1], 0) + "))";
        }
        else if (pExpr->args.size() == 2 && pExpr->token == "?") {
            // WGSL: select(false_val, true_val, condition)
            // SRSL AST: '?' node has args=[condition, ':' node]
            // ':' node has args=[true_branch, false_branch]
            const std::string cond = GenerateExpression(pExpr->args[0], 0);
            const SRSLExpr* colonExpr = pExpr->args[1];
            if (colonExpr && colonExpr->token == ":" && colonExpr->args.size() == 2) {
                const std::string trueVal  = GenerateExpression(colonExpr->args[0], 0);
                const std::string falseVal = GenerateExpression(colonExpr->args[1], 0);
                code += "select(" + falseVal + ", " + trueVal + ", " + cond + ")";
            }
            else {
                // Fallback: treat args[1] as true branch with unknown false
                const std::string trueVal = GenerateExpression(pExpr->args[1], 0);
                code += "select(" + trueVal + ", " + trueVal + ", " + cond + ")";
            }
        }
        else if (pExpr->args.size() == 2 && pExpr->token == ":") {
            // ':' node appears as the right child of a '?' node (ternary operator).
            // If it somehow reaches here standalone (shouldn't happen in well-formed AST),
            // emit just the true branch (args[0]) as a safe fallback.
            code += GenerateExpression(pExpr->args[0], 0);
        }
        else if (pExpr->args.size() == 2) {
            code += "(" + GenerateExpression(pExpr->args[0], 0) + " " + WGSLDetail::ReplaceToken(pExpr->token) + " " + GenerateExpression(pExpr->args[1], 0) + ")";
        }

        return code;
    }

    std::string WGSLCodeGenerator::GenerateVariable(const SRSLVariable* pVariable, int32_t deep) const {
        std::string code = std::string(GenerateTab(deep));

        // WGSL variable declaration: var name : type = expr;
        code += "var ";

        if (pVariable->pName) {
            m_tmpBuffer.clear();
            code += WGSLDetail::SafeIdentifier(std::string(pVariable->pName->ToString(0, m_tmpBuffer)));
        }

        if (pVariable->pType) {
            m_tmpBuffer.clear();
            std::string typeName = WGSLDetail::GenerateType(pVariable->pType->ToString(0, m_tmpBuffer));
            if (!typeName.empty()) {
                code += " : " + typeName;
            }
        }

        if (pVariable->pExpr) {
            code += " = " + GenerateExpression(pVariable->pExpr, 0);
        }

        return code;
    }

    std::string WGSLCodeGenerator::GenerateLexicalTree(const SRSLLexicalTree* pLexicalTree, int32_t deep) const {
        return GenerateLexicalTree(pLexicalTree, deep, std::string(), std::string());
    }

    std::string WGSLCodeGenerator::GenerateLexicalTree(const SRSLLexicalTree* pLexicalTree, int32_t deep, const std::string& preCode, const std::string& postCode) const {
        std::string code;

        if (deep >= 0) {
            code += "{\n";
        }

        if (!preCode.empty()) {
            code += preCode + "\n";
        }

        for (uint32_t i = 0; i < pLexicalTree->lexicalTree.size(); ++i) {
            auto&& pUnit = pLexicalTree->lexicalTree[i];

            if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                code += GenerateVariable(pVariable, deep + 1) + ";";
            }
            else if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                code += GenerateFunctionBody(pFunction, deep + 1) + "\n";
            }
            else if (auto&& pTree = dynamic_cast<SRSLLexicalTree*>(pUnit)) {
                code += GenerateLexicalTree(pTree, deep + 1);
            }
            else if (auto&& pExpression = dynamic_cast<SRSLExpr*>(pUnit)) {
                code += GenerateExpression(pExpression, deep + 1) + ";";
            }
            else if (auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(pUnit)) {
                code += GenerateIfStatement(pIfStatement, deep + 1);
            }
            else if (auto&& pReturn = dynamic_cast<SRSLReturn*>(pUnit)) {
                code += std::string(GenerateTab(deep + 1)) + "return " + GenerateExpression(pReturn->pExpr, 0) + ";\n";
            }
            else if (auto&& pForStatement = dynamic_cast<SRSLForStatement*>(pUnit)) {
                code += GenerateForStatement(pForStatement, deep + 1);
            }
            else if (auto&& pWhileStatement = dynamic_cast<SRSLWhileStatement*>(pUnit)) {
                code += GenerateWhileStatement(pWhileStatement, deep + 1);
            }

            if (i + 1 < pLexicalTree->lexicalTree.size()) {
                code += "\n";
            }
        }

        if (deep >= 0) {
            code += "\n";
        }

        if (!postCode.empty()) {
            code += "\n" + postCode;
        }

        if (deep >= 0) {
            code += std::string(GenerateTab(deep)) + "}";
        }

        return code;
    }

    std::string WGSLCodeGenerator::GenerateIfStatement(const SRSLIfStatement* pIfStatement, int32_t deep) const {
        std::string code;

        code += std::string(GenerateTab(deep));

        if (!pIfStatement->isElse && pIfStatement->pExpr) {
            code += "if";
        }
        else if (pIfStatement->isElse && pIfStatement->pExpr) {
            code += "else if";
        }
        else if (pIfStatement->isElse && !pIfStatement->pExpr) {
            code += "else";
        }
        else {
            SRHalt0();
        }

        if (pIfStatement->pExpr) {
            code += " (" + GenerateExpression(pIfStatement->pExpr, 0) + ")";
        }

        if (pIfStatement->pLexicalTree) {
            code += " " + GenerateLexicalTree(pIfStatement->pLexicalTree, deep);
        }

        return code;
    }

    std::string WGSLCodeGenerator::GenerateForStatement(const SRSLForStatement* pForStatement, int32_t deep) const {
        std::string code;

        code += std::string(GenerateTab(deep)) + "for (";

        if (pForStatement->pVar) {
            code += GenerateVariable(pForStatement->pVar, 0) + "; ";
        }

        if (pForStatement->pCondition) {
            code += GenerateExpression(pForStatement->pCondition, 0) + "; ";
        }

        if (pForStatement->pExpr) {
            code += GenerateExpression(pForStatement->pExpr, 0) + ") ";
        }

        if (pForStatement->pLexicalTree) {
            code += GenerateLexicalTree(pForStatement->pLexicalTree, deep);
        }

        return code;
    }

    std::string WGSLCodeGenerator::GenerateWhileStatement(const SRSLWhileStatement* pWhileStatement, int32_t deep) const {
        std::string code;

        code += std::string(GenerateTab(deep)) + "loop {\n";

        if (pWhileStatement->pCondition) {
            code += std::string(GenerateTab(deep + 1)) + "if (!(" + GenerateExpression(pWhileStatement->pCondition, 0) + ")) { break; }\n";
        }

        if (pWhileStatement->pLexicalTree) {
            // Emit body without braces since we already have the outer loop block
            for (auto&& pUnit : pWhileStatement->pLexicalTree->lexicalTree) {
                if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                    code += GenerateVariable(pVariable, deep + 1) + ";\n";
                }
                else if (auto&& pExpression = dynamic_cast<SRSLExpr*>(pUnit)) {
                    code += GenerateExpression(pExpression, deep + 1) + ";\n";
                }
                else if (auto&& pReturn = dynamic_cast<SRSLReturn*>(pUnit)) {
                    code += std::string(GenerateTab(deep + 1)) + "return " + GenerateExpression(pReturn->pExpr, 0) + ";\n";
                }
                else if (auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(pUnit)) {
                    code += GenerateIfStatement(pIfStatement, deep + 1) + "\n";
                }
                else if (auto&& pForStatement = dynamic_cast<SRSLForStatement*>(pUnit)) {
                    code += GenerateForStatement(pForStatement, deep + 1) + "\n";
                }
                else if (auto&& pSubTree = dynamic_cast<SRSLLexicalTree*>(pUnit)) {
                    code += GenerateLexicalTree(pSubTree, deep + 1);
                }
            }
        }

        code += std::string(GenerateTab(deep)) + "}\n";
        return code;
    }

    std::string WGSLCodeGenerator::GenerateStructure(const SRSLStructureStatement* pStructure, int32_t deep) const {
        if (pStructure->HasDynamicArray()) {
            return std::string();
        }

        std::string code;
        code += std::string(GenerateTab(deep));
        code += "struct ";

        if (pStructure->pName) {
            code += pStructure->pName->token.c_str();
        }

        code += " {\n";

        if (pStructure->pLexicalTree) {
            for (auto&& pUnit : pStructure->pLexicalTree->lexicalTree) {
                if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                    std::string fieldName;
                    std::string fieldType;
                    if (pVariable->pName) {
                        m_tmpBuffer.clear();
                        fieldName = pVariable->pName->ToString(0, m_tmpBuffer);
                    }
                    if (pVariable->pType) {
                        m_tmpBuffer.clear();
                        fieldType = WGSLDetail::GenerateType(pVariable->pType->ToString(0, m_tmpBuffer));
                    }
                    if (!fieldType.empty() && !fieldName.empty()) {
                        code += std::string(GenerateTab(deep + 1)) + fieldName + " : " + fieldType + ",\n";
                    }
                }
            }
        }

        code += std::string(GenerateTab(deep)) + "}";
        return code;
    }

    std::string WGSLCodeGenerator::GenerateFunctionBody(const SRSLFunction* pFunction, int32_t deep) const {
        std::string code;
        code += std::string(GenerateTab(deep));
        code += "fn ";  // WGSL requires "fn" keyword for every function definition

        if (pFunction->pName) {
            if (IsShaderEntryPoint(pFunction->GetName())) {
                code += pFunction->GetName().data();
            }
            else {
                code += WGSLDetail::SafeIdentifier(std::string(pFunction->pName->token));
            }
        }

        code += "(";

        for (uint32_t i = 0; i < pFunction->args.size(); ++i) {
            auto&& pArg = pFunction->args[i];
            std::string argName, argType;
            if (pArg->pName) {
                m_tmpBuffer.clear();
                argName = WGSLDetail::SafeIdentifier(std::string(pArg->pName->ToString(0, m_tmpBuffer)));
            }
            if (pArg->pType) {
                m_tmpBuffer.clear();
                argType = WGSLDetail::GenerateType(pArg->pType->ToString(0, m_tmpBuffer));
            }
            code += argName + " : " + argType;
            if (i + 1 < pFunction->args.size()) {
                code += ", ";
            }
        }

        code += ")";

        if (pFunction->pType) {
            m_tmpBuffer.clear();
            std::string retType = WGSLDetail::GenerateType(pFunction->pType->ToString(0, m_tmpBuffer));
            if (!retType.empty() && retType != "void") {
                code += " -> " + retType;
            }
        }

        if (pFunction->pLexicalTree) {
            code += " " + GenerateLexicalTree(pFunction->pLexicalTree, deep);
        }

        return code;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Uniforms / SSBO / Samplers / Push constants
    // ----------------------------------------------------------------------------------------------------------------

    std::string WGSLCodeGenerator::GenerateUniforms(const SRSLShader* pShader) const {
        // This function generates the combined uniforms block for the whole shader module.
        // It should be called ONCE per shader, not per stage.
        std::string code;

        // ---- Uniform Blocks → @group(0) @binding(N) var<uniform> Name : StructName; ----
        for (auto&& [name, uniformBlock] : pShader->GetUniformBlocks()) {
            // Emit an anonymous struct for the UBO fields
            std::string structName = std::string(name.ToStringView()) + "_t";
            code += "struct " + structName + " {\n";
            for (auto&& field : uniformBlock.fields) {
                auto&& baseType = WGSLDetail::GenerateType(
                    SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), field.type)
                );
                if (baseType.empty()) { continue; }

                auto&& dimensions = SRSLTypeInfo::Instance().GetDimension(pShader->GetAllocator(), field.type, pShader->GetAnalyzedTree());
                std::string fieldType = WGSLDetail::ToWGSLArrayType(baseType, dimensions);
                code += SR_FORMAT("\t// ({} bytes) {}\n", field.size, field.isPublic ? "public" : "private");
                code += SR_FORMAT("\t{} : {},\n", field.name.ToStringView(), fieldType);
            }
            code += "};\n";
            code += SR_FORMAT("@group(0) @binding({}) var<uniform> {} : {};\n\n",
                uniformBlock.binding, name.ToStringView(), structName);
        }

        // ---- SSBO Blocks → @group(0) @binding(N) var<storage, read_write/read> Name ----
        for (auto&& [name, ssboBlock] : pShader->GetSSBOBlocks()) {
            // Determine access mode
            std::string_view accessMode = "read_write";
            if (ssboBlock.isReadOnly.has_value()) {
                accessMode = ssboBlock.isReadOnly.value() ? "read" : "read_write";
            }

            // Special case: single dynamic-array field → emit as var<storage> name : array<T>
            // This allows shaders to use name[i] directly instead of name.field[i]
            if (ssboBlock.fields.size() == 1) {
                auto&& field = ssboBlock.fields[0];
                auto&& dims = SRSLTypeInfo::Instance().GetDimension(pShader->GetAllocator(), field.type, pShader->GetAnalyzedTree());
                if (!dims.empty() && dims.back() == 0) { // last dim == 0 means runtime-sized []
                    auto&& baseType = WGSLDetail::GenerateType(
                        SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), field.type)
                    );
                    if (!baseType.empty()) {
                        // Build array<baseType> (dropping the last runtime dim, since it is implicit)
                        std::vector<uint64_t> staticDims(dims.begin(), dims.end() - 1);
                        std::string elemType = WGSLDetail::ToWGSLArrayType(baseType, staticDims);
                        code += SR_FORMAT("@group(0) @binding({}) var<storage, {}> {} : array<{}>;\n\n",
                            ssboBlock.binding, accessMode, name.ToStringView(), elemType);
                        continue;
                    }
                }
            }

            // General case: emit as struct
            std::string structName = std::string(name.ToStringView()) + "_t";
            code += "struct " + structName + " {\n";
            for (auto&& field : ssboBlock.fields) {
                auto&& baseType = WGSLDetail::GenerateType(
                    SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), field.type)
                );
                if (baseType.empty()) { continue; }

                auto&& dimensions = SRSLTypeInfo::Instance().GetDimension(pShader->GetAllocator(), field.type, pShader->GetAnalyzedTree());
                std::string fieldType = WGSLDetail::ToWGSLArrayType(baseType, dimensions);
                code += SR_FORMAT("\t{} : {},\n", field.name.ToStringView(), fieldType);
            }
            code += "};\n";
            code += SR_FORMAT("@group(0) @binding({}) var<storage, {}> {} : {};\n\n",
                ssboBlock.binding, accessMode, name.ToStringView(), structName);
        }

        // ---- Samplers → @group(1) @binding(N) var tex : texture_2d<f32> + sampler ----
        uint32_t samplerBinding = 0;
        for (auto&& [name, sampler] : pShader->GetSamplers()) {
            std::string_view wgslTextureType = "texture_2d<f32>";
            if (sampler.type == "sampler3D") {
                wgslTextureType = "texture_3d<f32>";
            }
            else if (sampler.type == "samplerCube") {
                wgslTextureType = "texture_cube<f32>";
            }
            else if (sampler.type == "sampler2DShadow") {
                wgslTextureType = "texture_depth_2d";
            }
            else if (sampler.type == "sampler2DArray") {
                wgslTextureType = "texture_2d_array<f32>";
            }
            else if (sampler.type == "subpassInput" || sampler.attachment >= 0) {
                wgslTextureType = "texture_2d<f32>";
            }

            code += SR_FORMAT("@group(1) @binding({}) var {} : {};\n", samplerBinding * 2, name.ToStringView(), wgslTextureType);
            code += SR_FORMAT("@group(1) @binding({}) var {}_sampler : sampler;\n\n", samplerBinding * 2 + 1, name.ToStringView());
            ++samplerBinding;
        }

        // ---- Push constants → emulated as @group(2) @binding(0) var<uniform> PushConstants : PushConstants_t ----
        // WGSL does NOT allow module-scope variable initializers that reference uniform buffers.
        // We emit only the struct and binding declaration here; the field aliases are emitted
        // as function-local `let` statements in preCode of each stage function.
        if (!pShader->GetPushConstants().fields.empty()) {
            code += "struct PushConstants_t {\n";
            for (auto&& field : pShader->GetPushConstants().fields) {
                auto&& typeName = WGSLDetail::GenerateType(
                    SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), field.type)
                );
                if (typeName.empty()) { continue; }
                code += SR_FORMAT("\t{} : {},\n", WGSLDetail::SafeIdentifier(std::string(field.name.ToStringView())), typeName);
            }
            code += "};\n";
            code += "@group(2) @binding(0) var<uniform> _pushConstants : PushConstants_t;\n\n";
        }

        return code;
    }

    std::string WGSLCodeGenerator::GeneratePushConstantAliases(const SRSLShader* pShader, ShaderStage stage) const {
        std::string code;

        if (pShader->GetPushConstants().fields.empty()) {
            return code;
        }

        auto&& pFunction = pShader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));
        if (!pFunction) {
            return code;
        }

        // Emit function-local let-bindings for each push constant field used in this stage.
        // WGSL disallows initializing module-scope vars from uniform buffers, but let-bindings
        // inside functions are fine.
        for (auto&& field : pShader->GetPushConstants().fields) {
            if (!pFunction->IsVariableUsed(field.name)) {
                continue;
            }
            auto&& typeName = WGSLDetail::GenerateType(
                SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), field.type)
            );
            if (typeName.empty()) { continue; }
            const std::string safeFieldName = WGSLDetail::SafeIdentifier(std::string(field.name.ToStringView()));
            code += SR_FORMAT("{}let {} : {} = _pushConstants.{};\n",
                GenerateTab(1), field.name.ToStringView(), typeName, safeFieldName);
        }

        return code;
    }

    std::string WGSLCodeGenerator::GenerateUniformBlockAliases(const SRSLShader* pShader, ShaderStage stage) const {
        std::string code;

        auto&& pFunction = pShader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));
        if (!pFunction) {
            return code;
        }

        // For each uniform block, emit local let-bindings for fields used in this stage.
        // This makes UBO fields directly accessible by name inside the function body,
        // matching GLSL behaviour where UBO fields are implicitly in scope.
        for (auto&& [blockName, uniformBlock] : pShader->GetUniformBlocks()) {
            for (auto&& field : uniformBlock.fields) {
                if (!pFunction->IsVariableUsed(field.name)) {
                    continue;
                }
                auto&& typeName = WGSLDetail::GenerateType(
                    SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), field.type)
                );
                if (typeName.empty()) { continue; }
                const std::string safeField = WGSLDetail::SafeIdentifier(std::string(field.name.ToStringView()));
                code += SR_FORMAT("{}let {} : {} = {}.{};\n",
                    GenerateTab(1), field.name.ToStringView(), typeName,
                    blockName.ToStringView(), safeField);
            }
        }

        return code;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // GenerateFunction (entry-point wrapper with decorators)
    // ----------------------------------------------------------------------------------------------------------------

    std::string_view WGSLCodeGenerator::GenerateFunction(
        SRSLFunction* pFunction,
        const int32_t deep,
        const std::string_view& preArgs,
        const std::string_view& preCode,
        const std::string_view& postCode,
        const std::string_view& returnType
    ) {
        static std::string result;
        result.clear();

        if (!pFunction) {
            return result;
        }

        result += std::string(GenerateTab(deep));
        result += "fn ";
        result += pFunction->GetName();
        result += "(";

        result += preArgs;
        if (!preArgs.empty() && !pFunction->args.empty()) {
            result += ", ";
        }

        for (size_t i = 0; i < pFunction->args.size(); ++i) {
            auto&& parameter = pFunction->args[i];
            std::string argName, argType;
            if (parameter->pName) {
                m_tmpBuffer.clear();
                argName = parameter->pName->ToString(0, m_tmpBuffer);
            }
            if (parameter->pType) {
                m_tmpBuffer.clear();
                argType = WGSLDetail::GenerateType(parameter->pType->ToString(0, m_tmpBuffer));
            }
            result += argName + " : " + argType;
            if (i + 1 < pFunction->args.size()) {
                result += ", ";
            }
        }

        m_tmpBuffer.clear();
        std::string retTypeStr;
        if (!returnType.empty()) {
            retTypeStr = std::string(returnType);
        }
        else if (pFunction->pType) {
            retTypeStr = WGSLDetail::GenerateType(pFunction->pType->ToString(0, m_tmpBuffer));
        }

        if (!retTypeStr.empty() && retTypeStr != "void") {
            result += ") -> ";
            result += retTypeStr;
        }
        else {
            result += ") ";
        }
        result += " {\n";

        if (!preCode.empty()) {
            result += preCode;
        }

        // Emit function body from AST
        if (pFunction->pLexicalTree) {
            for (auto&& pUnit : pFunction->pLexicalTree->lexicalTree) {
                if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                    result += GenerateVariable(pVariable, deep + 1) + ";\n";
                }
                else if (auto&& pNestedFn = dynamic_cast<SRSLFunction*>(pUnit)) {
                    result += GenerateFunctionBody(pNestedFn, deep + 1) + "\n";
                }
                else if (auto&& pTree = dynamic_cast<SRSLLexicalTree*>(pUnit)) {
                    result += GenerateLexicalTree(pTree, deep + 1);
                }
                else if (auto&& pExpression = dynamic_cast<SRSLExpr*>(pUnit)) {
                    result += GenerateExpression(pExpression, deep + 1) + ";\n";
                }
                else if (auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(pUnit)) {
                    result += GenerateIfStatement(pIfStatement, deep + 1) + "\n";
                }
                else if (auto&& pReturn = dynamic_cast<SRSLReturn*>(pUnit)) {
                    result += std::string(GenerateTab(deep + 1)) + "return " + GenerateExpression(pReturn->pExpr, 0) + ";\n";
                }
                else if (auto&& pForStatement = dynamic_cast<SRSLForStatement*>(pUnit)) {
                    result += GenerateForStatement(pForStatement, deep + 1) + "\n";
                }
                else if (auto&& pWhileStatement = dynamic_cast<SRSLWhileStatement*>(pUnit)) {
                    result += GenerateWhileStatement(pWhileStatement, deep + 1);
                }
            }
        }

        if (!postCode.empty()) {
            result += postCode;
        }

        result += std::string(GenerateTab(deep));
        result += "}\n";

        return result;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // GenerateStage
    // ----------------------------------------------------------------------------------------------------------------

    std::string_view WGSLCodeGenerator::GenerateStage(const SRSLShader* pShader, SRSLResult& result, ShaderStage stage, const std::string& preCode) {
        static std::string code;
        code.clear();

        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(stage);

        // Emit shared workgroup variables (only for compute stage)
        if (stage == ShaderStage::Compute) {
            if (auto&& pFunctionCallStack = pShader->GetUseStack()->FindFunction(entryPoint)) {
                for (auto&& [name, pVariable] : pShader->GetSharedWorkgroup()) {
                    if (!pFunctionCallStack->IsVariableUsed(name)) {
                        continue;
                    }
                    if (pVariable->pType) {
                        // Get the raw SRSL type string (may include e.g. "float[256]")
                        m_tmpBuffer.clear();
                        std::string rawType = std::string(pVariable->pType->ToString(0, m_tmpBuffer));
                        // GetTypeName strips array brackets → returns base type ("float")
                        std::string strippedType = SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), rawType);
                        std::string baseType = WGSLDetail::GenerateType(strippedType.empty() ? rawType : strippedType);
                        // Get dimensions using the raw SRSL type string
                        auto dims = SRSLTypeInfo::Instance().GetDimension(
                            pShader->GetAllocator(), rawType, pShader->GetAnalyzedTree());
                        std::string varType = WGSLDetail::ToWGSLArrayType(baseType, dims);
                        if (!varType.empty()) {
                            code += SR_FORMAT("var<workgroup> {} : {};\n", name.ToStringView(), varType);
                        }
                    }
                }
            }
        }

        // Note: uniforms/SSBOs/samplers are emitted once at module level in GenerateStages.
        // preCode contains function-local aliases (let-bindings) that MUST stay inside the
        // function body — GenerateFunction/GenerateComputeStage inject them via its preCode param.
        // Do NOT emit preCode here at module scope.

        if (auto&& pFunctionCallStack = pShader->GetUseStack()->FindFunction(entryPoint)) {
            for (auto&& pUnit : pShader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                    if (!pFunctionCallStack->IsFunctionUsed(pFunction->pName->token)) {
                        continue;
                    }
                    // Skip the entry point itself — it's emitted by GenerateVertex/Fragment/ComputeStage
                    if (pFunction->GetName() == entryPoint) {
                        continue;
                    }

                    code += GenerateFunctionBody(pFunction, 0);
                    code += "\n\n";
                }
            }
        }

        return code;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Vertex stage
    // ----------------------------------------------------------------------------------------------------------------

    std::optional<std::string_view> WGSLCodeGenerator::GenerateVertexStage(const SRSLShader* pShader, SRSLResult& result) {
        static std::string resultCode;
        static std::string preArgs;
        static std::string preCode;
        static std::string postCode;

        resultCode.clear();
        preArgs.clear();
        preCode.clear();
        postCode.clear();

        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Vertex);
        auto&& pStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = pShader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction) {
            return std::optional<std::string_view>();
        }

        resultCode = GenerateStage(pShader, result, ShaderStage::Vertex);

        const bool isOutPositionUsed = pUseStackFunction->IsVariableUsed("OUT_POSITION");

        // Inject UBO field aliases and push constant aliases (must be inside function, not module scope)
        preCode += GenerateUniformBlockAliases(pShader, ShaderStage::Vertex);
        preCode += GeneratePushConstantAliases(pShader, ShaderStage::Vertex);

        preCode += std::string(GenerateTab(1));
        preCode += "var vsOut : VertexOutput;\n";
        if (isOutPositionUsed) {
            preCode += std::string(GenerateTab(1));
            preCode += "var OUT_POSITION : vec4<f32>;\n";
        }

        auto&& vertexLayoutDescriptions = pShader->GetCreateInfo().vertexLayoutDescriptions;
        if (vertexLayoutDescriptions.GetAttributesCount() > 0) {
            preArgs += "input : VertexInput";
        }

        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            preCode += SR_FORMAT("{}{} = input.{}_INPUT;\n", GenerateTab(1), attributeName, attributeName);
        });

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("VERTEX_INDEX")) {
            preCode += std::string(GenerateTab(1));
            preCode += "var VERTEX_INDEX : u32 = vertexIndex;\n";
            if (!preArgs.empty()) {
                preArgs += ", ";
            }
            preArgs += "@builtin(vertex_index) vertexIndex : u32";
        }

        if (isOutPositionUsed) {
            postCode += std::string(GenerateTab(1));
            postCode += "vsOut.position = OUT_POSITION;\n";
        }
        else if (vertexLayoutDescriptions.Find(SR_UTILS_NS::VertexAttribute::Position)) {
            postCode += std::string(GenerateTab(1));
            postCode += "vsOut.position = vec4<f32>(VERTEX, 1.0);\n";
        }

        // Copy vertex attributes into vsOut
        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            postCode += SR_FORMAT("{}vsOut.{} = {};\n", GenerateTab(1), attributeName, attributeName);
        });

        // Copy shared vars into vsOut for interpolation to fragment (using _s_ prefix to avoid name clash)
        for (auto&& [name, pVariable] : pShader->GetShared()) {
            postCode += SR_FORMAT("{}vsOut._s_{} = {};\n", GenerateTab(1), name.ToStringView(), name.ToStringView());
        }

        postCode += std::string(GenerateTab(1));
        postCode += "return vsOut;\n";

        resultCode += "@vertex\n";

        static const std::string vertexOutputId = "VertexOutput";
        resultCode += GenerateFunction(pStageFunction, 0, preArgs, preCode, postCode, vertexOutputId);

        return resultCode;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Fragment stage
    // ----------------------------------------------------------------------------------------------------------------

    std::optional<std::string_view> WGSLCodeGenerator::GenerateFragmentStage(const SRSLShader* pShader, SRSLResult& result) {
        static std::string code;
        static std::string variablesCode;
        static std::string preCode;
        static std::string postCode;

        code.clear();
        variablesCode.clear();
        preCode.clear();
        postCode.clear();

        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment);
        auto&& pStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = pShader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction || !pUseStackFunction) {
            return std::optional<std::string>();
        }

        const bool isColorPassDefined = pShader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = pShader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);
        const bool isColorUsed = isColorPassDefined || pUseStackFunction->IsVariableUsed("COLOR");
        const bool isFragCoordUsed = pUseStackFunction->IsVariableUsed("FRAG_COORD");

        code = GenerateStage(pShader, result, ShaderStage::Fragment, variablesCode);

        // Inject UBO field aliases and push constant aliases
        preCode += GenerateUniformBlockAliases(pShader, ShaderStage::Fragment);
        preCode += GeneratePushConstantAliases(pShader, ShaderStage::Fragment);

        // Fragment input: vertex output interpolants
        static std::string fragArgs;
        fragArgs.clear();
        auto&& vertexLayoutDescriptions = pShader->GetCreateInfo().vertexLayoutDescriptions;
        auto&& sharedVarsF = pShader->GetShared();
        const bool needsFsIn = vertexLayoutDescriptions.GetAttributesCount() > 0
                            || isFragCoordUsed
                            || !sharedVarsF.empty();
        if (needsFsIn) {
            fragArgs = "fsIn : VertexOutput";
        }

        if (isFragCoordUsed) {
            preCode += std::string(GenerateTab(1));
            preCode += "var FRAG_COORD : vec4<f32> = fsIn.position;\n\n";
        }

        // Copy vertex interpolants into private vars
        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            preCode += SR_FORMAT("{}{} = fsIn.{};\n", GenerateTab(1), attributeName, attributeName);
        });

        // Copy shared (inter-stage) interpolants into private vars (using _s_ prefix from VertexOutput)
        for (auto&& [name, pVariable] : sharedVarsF) {
            preCode += SR_FORMAT("{}{} = fsIn._s_{};\n", GenerateTab(1), name.ToStringView(), name.ToStringView());
        }

        uint64_t outLocation = 0;
        for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
            const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
            if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                preCode += std::string(GenerateTab(1));
                preCode += SR_FORMAT("var {} : vec4<f32>; /// location {}\n", layer.c_str(), outLocation);
            }
            ++outLocation;
        }

        preCode += std::string(GenerateTab(1));
        preCode += "var fsOut : FragmentOutput;\n";

        if (isColorUsed) {
            postCode += std::string(GenerateTab(1));
            postCode += SR_SRSL_MAIN_OUT_LAYER;
            postCode += " = COLOR;\n";
        }

        // Color buffer pass code
        if (isColorPassDefined) {
            const bool discardExists = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_color_buffer_discard") != nullptr;
            if (discardExists) {
                postCode += std::string(GenerateTab(1));
                postCode += "fragment_color_buffer_discard();\n";
            }
            preCode += std::string(GenerateTab(1));
            preCode += "COLOR = vec4<f32>({}, 1.0);\n"_format(SHADER_PC_COLOR_BUFFER_VALUE);
        }
        else if (isCascadedMapPassDefined) {
            const bool discardExists = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_depth_buffer_discard") != nullptr;
            if (discardExists) {
                postCode += std::string(GenerateTab(1));
                postCode += "fragment_depth_buffer_discard();\n";
            }
        }

        outLocation = 0;
        for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
            const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
            if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                if (pShader->IsMacroDefined(SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[outLocation])) {
                    postCode += std::string(GenerateTab(1));
                    postCode += SR_FORMAT("fsOut.{} = {};\n", layer.c_str(), layer.c_str());
                }
            }
            ++outLocation;
        }

        postCode += std::string(GenerateTab(1));
        postCode += "return fsOut;\n";

        code += "@fragment\n";
        code += GenerateFunction(pStageFunction, 0, fragArgs, preCode, postCode);

        return code;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Compute stage
    // ----------------------------------------------------------------------------------------------------------------

    std::optional<std::string_view> WGSLCodeGenerator::GenerateComputeStage(const SRSLShader* pShader, SRSLResult& result) {
        static std::string code;
        static std::string preCode;
        static std::string preArgs;

        code.clear();
        preCode.clear();
        preArgs.clear();

        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Compute);
        auto&& pStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = pShader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction || !pUseStackFunction) {
            return std::optional<std::string_view>();
        }

        // Inject UBO field aliases and push constant aliases
        preCode += GenerateUniformBlockAliases(pShader, ShaderStage::Compute);
        preCode += GeneratePushConstantAliases(pShader, ShaderStage::Compute);

        // Always declare ALL compute builtins as parameters.
        // The shader body may use gl_GlobalInvocationID, gl_LocalInvocationIndex etc. directly
        // (which ReplaceToken maps to the WGSL builtin names), so they must be in scope regardless
        // of whether the SRSL alias (GLOBAL_INVOCATION_ID etc.) was used.
        preArgs = "@builtin(global_invocation_id) global_id : vec3<u32>, "
                  "@builtin(workgroup_id) workgroup_id : vec3<u32>, "
                  "@builtin(num_workgroups) num_workgroups : vec3<u32>, "
                  "@builtin(local_invocation_id) local_id : vec3<u32>, "
                  "@builtin(local_invocation_index) local_index : u32";

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("GLOBAL_INVOCATION_ID")) {
            preCode += std::string(GenerateTab(1));
            preCode += "var GLOBAL_INVOCATION_ID : vec3<u32> = global_id;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("WORK_GROUP_ID")) {
            preCode += std::string(GenerateTab(1));
            preCode += "var WORK_GROUP_ID : vec3<u32> = workgroup_id;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("NUM_WORK_GROUPS")) {
            preCode += std::string(GenerateTab(1));
            preCode += "var NUM_WORK_GROUPS : vec3<u32> = num_workgroups;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_ID")) {
            preCode += std::string(GenerateTab(1));
            preCode += "var LOCAL_INVOCATION_ID : vec3<u32> = local_id;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_INDEX")) {
            preCode += std::string(GenerateTab(1));
            preCode += "var LOCAL_INVOCATION_INDEX : u32 = local_index;\n";
        }

        // Emit @workgroup_size decorator
        auto&& computeGroupSize = pShader->GetComputeWorkGroupSize();
        std::string workgroupDecorator = SR_FORMAT("@compute @workgroup_size({}, {}, {})\n",
            computeGroupSize.x, computeGroupSize.y, computeGroupSize.z);

        // Check for THREADS decorator on the compute function
        if (pStageFunction->pDecorators) {
            auto&& threadsDecorator = pStageFunction->pDecorators->Find("THREADS");
            if (threadsDecorator && threadsDecorator->args.size() == 3) {
                workgroupDecorator = SR_FORMAT("@compute @workgroup_size({}, {}, {})\n",
                    threadsDecorator->args[0]->token.c_str(),
                    threadsDecorator->args[1]->token.c_str(),
                    threadsDecorator->args[2]->token.c_str()
                );
            }
        }

        code = GenerateStage(pShader, result, ShaderStage::Compute, preCode);
        code += workgroupDecorator;
        code += GenerateFunction(pStageFunction, 0, preArgs, preCode);

        return code;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // GenerateStages (top-level entry point)
    // ----------------------------------------------------------------------------------------------------------------

    ISRSLCodeGenerator::SRSLCodeGenRes WGSLCodeGenerator::GenerateStages(SR_UTILS_NS::IAllocator* pAllocator, const SRSLShader* pShader) {
        SR_GLOBAL_LOCK

        Clear();

        ISRSLCodeGenerator::SRSLCodeGenRes codeGenRes;

        auto&& [result, stages] = codeGenRes;

        if (!pShader->GetAnalyzedTree()) {
            result = SRSLResult(SRSLReturnCode::InvalidLexicalTree);
            return codeGenRes;
        }

        auto& code = stages[ShaderStage::All];
        code.reserve(4096);

        code += "/// [WARNING: THIS FILE WAS CREATED BY SRSL CODE GENERATION]\n\n";
        if (pShader->IsGLayerUsed()) {
            code += "/// WARNING: GLayer (viewport layer) is not supported in WGSL. Remove GLayer usage or use a different shader target.\n\n";
        }
        code += "/// Shader type: " + SR_UTILS_NS::EnumReflector::ToStringAtom(pShader->GetType()).ToStringRef() + "\n\n";

        // ---- Uniforms / SSBOs / Samplers (emitted once for the whole module) ----
        {
            auto&& uniformsCode = GenerateUniforms(pShader);
            if (!uniformsCode.empty()) {
                code += uniformsCode;
                code += "\n";
            }
        }

        // ---- Shared (inter-stage) variables ----
        // These are [[shared]] variables that must be passed from vertex to fragment via
        // VertexOutput interpolants. Collect them for inclusion in VertexOutput struct.
        auto&& sharedVars = pShader->GetShared();

        // ---- VertexInput struct ----
        auto&& vertexLayoutDescriptions = pShader->GetCreateInfo().vertexLayoutDescriptions;
        if (vertexLayoutDescriptions.GetAttributesCount() > 0) {
            code += "struct VertexInput {\n";
            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t i) {
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "\t@location({}) {}_INPUT : {},\n"_format(i, attributeName, attributeType);
            });
            code += "};\n\n";
        }

        // ---- VertexOutput struct (always emitted) ----
        // Shared variables are added with a "_s_" prefix to avoid clashing with
        // the @builtin(position) field named "position".
        {
            code += "struct VertexOutput {\n";
            code += "\t@builtin(position) position : vec4<f32>,\n";
            uint32_t locationIdx = 0;
            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "\t@location({}) {} : {},\n"_format(locationIdx++, attributeName, attributeType);
            });
            // Add shared (inter-stage) variables as interpolants with "_s_" prefix
            for (auto&& [name, pVariable] : sharedVars) {
                if (pVariable->pType) {
                    m_tmpBuffer.clear();
                    std::string strippedType = SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), std::string(pVariable->pType->ToString(0, m_tmpBuffer)));
                    std::string typeName = WGSLDetail::GenerateType(strippedType.empty() ? std::string(pVariable->pType->ToString(0, m_tmpBuffer)) : strippedType);
                    if (!typeName.empty()) {
                        code += "\t@location({}) _s_{} : {},\n"_format(locationIdx++, name.ToStringView(), typeName);
                    }
                }
            }
            code += "};\n\n";
        }

        // Private module-scope vars for vertex attributes (writable from vertex/fragment functions)
        if (vertexLayoutDescriptions.GetAttributesCount() > 0) {
            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "var<private> {} : {};\n"_format(attributeName, attributeType);
            });
            code += "\n";
        }

        // Private module-scope vars for shared (inter-stage) variables
        for (auto&& [name, pVariable] : sharedVars) {
            if (pVariable->pType) {
                m_tmpBuffer.clear();
                std::string rawType = std::string(pVariable->pType->ToString(0, m_tmpBuffer));
                std::string strippedType = SRSLTypeInfo::Instance().GetTypeName(pShader->GetAllocator(), rawType);
                std::string typeName = WGSLDetail::GenerateType(strippedType.empty() ? rawType : strippedType);
                if (!typeName.empty()) {
                    code += "var<private> {} : {};\n"_format(name.ToStringView(), typeName);
                }
            }
        }
        if (!sharedVars.empty()) {
            code += "\n";
        }

        // ---- FragmentOutput struct ----
        auto&& fragmentEntryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment);
        auto&& pFragmentStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(fragmentEntryPoint);

        if (pFragmentStageFunction) {
            code += "struct FragmentOutput {\n";
            uint64_t outLocation = 0;
            for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
                if (pShader->IsMacroDefined(SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[outLocation])) {
                    code += "\t@location({}) {} : vec4<f32>,\n"_format(outLocation, layer.c_str());
                }
                ++outLocation;
            }
            code += "};\n\n";
        }

        // ---- Module-scope private vars for COLOR / FRAG_COORD ----
        auto&& pFragmentUseStackFunction = pShader->GetUseStack()->FindFunction(fragmentEntryPoint);
        if (pFragmentStageFunction && pFragmentUseStackFunction) {
            const bool isColorPassDefined = pShader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
            const bool isColorUsed = isColorPassDefined || pFragmentUseStackFunction->IsVariableUsed("COLOR");
            const bool isFragCoordUsed = pFragmentUseStackFunction->IsVariableUsed("FRAG_COORD");

            if (isColorUsed) {
                code += "var<private> COLOR : vec4<f32>;\n\n";
            }
            if (isFragCoordUsed) {
                code += "var<private> FRAG_COORD : vec4<f32>;\n\n";
            }
        }

        // ---- Generate each stage ----
        bool hasVertex = false, hasFragment = false, hasCompute = false;

        if (auto&& stageCode = GenerateVertexStage(pShader, result)) {
            code += stageCode.value();
            code += "\n";
            hasVertex = true;
        }

        if (auto&& stageCode = GenerateFragmentStage(pShader, result)) {
            code += stageCode.value();
            code += "\n";
            hasFragment = true;
        }

        if (auto&& stageCode = GenerateComputeStage(pShader, result)) {
            code += stageCode.value();
            hasCompute = true;
        }

        // WGSL uses a single combined source file for all entry points.
        // Keep ShaderStage::All populated so that tests and any code reading result.second[All]
        // still get the full shader source.
        // Also store the same combined code under each individual stage key so Export()
        // can write it to the per-stage cache files that AllocateShaderProgram() reads later.
        if (hasVertex) {
            stages[ShaderStage::Vertex] = code;
        }
        if (hasFragment) {
            stages[ShaderStage::Fragment] = code;
        }
        if (hasCompute) {
            stages[ShaderStage::Compute] = code;
        }
        // ShaderStage::All was already filled as 'code' above — keep it for consumers that
        // read stages[ShaderStage::All] directly (e.g. SRSLTest).

        result = SR_UTILS_NS::Exchange(m_result, { });

        return codeGenRes;
    }
} // namespace SR_SRSL_NS
