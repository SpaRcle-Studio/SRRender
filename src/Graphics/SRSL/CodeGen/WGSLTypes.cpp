//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    SR_UTILS_NS::StringView WGSLGenerator::GetSharedVariableType(const SRSLShader* pShader, const SRSLVariable* pVariable) const {
        SR_TRACY_ZONE;
        if (!pVariable || !pVariable->pType) {
            return { };
        }
        m_tmpBuffer.clear();
        const auto rawType = pVariable->pType->ToString(0, m_tmpBuffer);
        const auto& strippedType = DescribeType(rawType, pShader->GetAnalyzedTree()).base;
        return WGSLDetail::GenerateType(strippedType.empty() ? KeepText(rawType) : strippedType.ToStringView());
    }

    SR_UTILS_NS::StringView WGSLGenerator::GetResourceType(SR_UTILS_NS::StringView type) const {
        SR_TRACY_ZONE;
        const auto& description = DescribeType(type, m_pCurrentShader->GetAnalyzedTree());
        const auto baseType = WGSLDetail::GenerateType(description.base);
        if (description.dimensions.empty()) { return baseType; }
        const auto found = m_arrayTypes.find(&description);
        if (found != m_arrayTypes.end()) { return found->second; }
        return m_arrayTypes.emplace(&description, WGSLDetail::ToWGSLArrayType(baseType, description.dimensions)).first->second;
    }

    void WGSLGenerator::RegisterVariableType(SR_UTILS_NS::StringView name, SR_UTILS_NS::StringView wgslType) const {
        SR_TRACY_ZONE;
        if (name.empty() || wgslType.empty()) {
            return;
        }
        auto it = m_variableTypes.find(name);
        if (m_scopeDepth > 0) {
            m_typeBindings.emplace_back(TypeBinding {
                SR_UTILS_NS::String(name, &m_scratch),
                it != m_variableTypes.end() ? std::move(it->second) : SR_UTILS_NS::String(&m_scratch),
                it != m_variableTypes.end()
            });
        }
        if (it != m_variableTypes.end()) {
            it->second = SR_UTILS_NS::String(wgslType, &m_scratch);
        }
        else {
            m_variableTypes.emplace(KeepText(name), SR_UTILS_NS::String(wgslType, &m_scratch));
        }
    }

    void WGSLGenerator::CollectDeclaredTypes(const SRSLShader* pShader) const {
        SR_TRACY_ZONE;
        auto&& registerField = [this, pShader](const auto& field) {
            const SR_UTILS_NS::String typeName = WGSLDetail::GenerateType(
                DescribeType(field.type, pShader->GetAnalyzedTree()).base
            );
            RegisterVariableType(field.name.ToStringView(), typeName);
        };

        for (auto&& [blockName, uniformBlock] : pShader->GetUniformBlocks()) {
            for (auto&& field : uniformBlock.fields) {
                registerField(field);
            }
        }

        for (auto&& [blockName, ssboBlock] : pShader->GetSSBOBlocks()) {
            for (auto&& field : ssboBlock.fields) {
                registerField(field);
            }
        }

        for (auto&& field : pShader->GetPushConstants().fields) {
            registerField(field);
        }

        for (auto&& [name, pVariable] : pShader->GetShared()) {
            RegisterVariableType(name.ToStringView(), GetSharedVariableType(pShader, pVariable));
        }

        for (auto&& [name, pVariable] : pShader->GetSharedWorkgroup()) {
            RegisterVariableType(name.ToStringView(), GetSharedVariableType(pShader, pVariable));
        }

        for (auto&& [name, pVariable] : pShader->GetConstants()) {
            RegisterVariableType(name.ToStringView(), GetSharedVariableType(pShader, pVariable));
        }

        for (auto&& [name, sampler] : pShader->GetSamplers()) {
            if (sampler.type.ToStringView() == "sampler2DArray") {
                RegisterVariableType(name.ToStringView(), "texture_2d_array<f32>");
            }
            else if (sampler.type.ToStringView() == "sampler2DArrayDepth") {
                RegisterVariableType(name.ToStringView(), "texture_depth_2d_array");
            }
        }

        pShader->GetCreateInfo().vertexLayoutDescriptions.ForEachAttribute(
            [this](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                RegisterVariableType(
                    SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute),
                    WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count)
                );
            });

        RegisterVariableType("OUT_POSITION", "vec4<f32>");
        RegisterVariableType("COLOR", "vec4<f32>");
        RegisterVariableType("FRAG_COORD", "vec4<f32>");

        if (pShader->GetAnalyzedTree() && pShader->GetAnalyzedTree()->pLexicalTree) {
            for (auto&& pUnit : pShader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit);
                if (!pFunction || !pFunction->pName || !pFunction->pType) {
                    continue;
                }
                m_tmpBuffer.clear();
                const SR_UTILS_NS::StringView returnType = WGSLDetail::GenerateType(pFunction->pType->ToString(0, m_tmpBuffer));
                if (!returnType.empty() && returnType != "void") {
                    m_functionReturnTypes[pFunction->pName->token.ToStringView()] = SR_UTILS_NS::String(returnType, &m_scratch);
                }
            }
        }
    }

    SR_UTILS_NS::StringView WGSLGenerator::InferExprType(const SRSLExpr* pExpr) const {
        SR_TRACY_ZONE;
        if (!pExpr) {
            return { };
        }

        if (pExpr->isCall) {
            const SR_UTILS_NS::String& token = pExpr->token;

            /// конструктор типа: GenerateType() меняет имя только для известных типов
            const SR_UTILS_NS::StringView asType = WGSLDetail::GenerateType(token);
            if (asType != token && !asType.empty()) {
                return asType;
            }

            /// функции, не меняющие тип аргумента
            static const SR_HTYPES_NS::FlatHashSet<SR_UTILS_NS::StringView> passThrough = {
                "transpose", "inverse", "normalize", "abs", "min", "max", "clamp", "mix",
                "floor", "ceil", "round", "fract", "sign", "saturate", "pow", "sqrt", "exp", "log"
            };
            if (passThrough.count(token) > 0 && !pExpr->args.empty()) {
                return InferExprType(pExpr->args[0]);
            }

            auto&& it = m_functionReturnTypes.find(token);
            if (it != m_functionReturnTypes.end()) {
                return it->second;
            }

            return { };
        }

        if (pExpr->isArray && pExpr->args.size() == 2) {
            const SR_UTILS_NS::StringView baseType = InferExprType(pExpr->args[0]);
            /// индексация матрицы даёт её столбец
            if (WGSLDetail::MatrixColumns(baseType) > 0) {
                return WGSLDetail::MatrixColumnType(baseType);
            }
            return { };
        }

        if (pExpr->args.empty()) {
            if (WGSLDetail::IsNumericLiteral(pExpr->token)) {
                return pExpr->token.find('.') != SR_UTILS_NS::String::npos ? "f32" : "i32";
            }
            auto&& it = m_variableTypes.find(pExpr->token);
            if (it != m_variableTypes.end()) {
                return it->second;
            }
            return { };
        }

        if (pExpr->args.size() == 1) {
            return InferExprType(pExpr->args[0]);
        }

        if (pExpr->args.size() == 2) {
            const SR_UTILS_NS::String& token = pExpr->token;
            if (token == "+" || token == "-" || token == "*" || token == "/") {
                const SR_UTILS_NS::StringView left = InferExprType(pExpr->args[0]);
                const SR_UTILS_NS::StringView right = InferExprType(pExpr->args[1]);
                if (left == right) {
                    return left;
                }
                if (WGSLDetail::IsScalarType(left)) {
                    return right;
                }
                if (WGSLDetail::IsScalarType(right)) {
                    return left;
                }
                /// матрица * вектор даёт вектор
                if (token == "*") {
                    if (WGSLDetail::MatrixColumns(left) > 0 && WGSLDetail::MatrixColumns(right) == 0) {
                        return right;
                    }
                    if (WGSLDetail::MatrixColumns(right) > 0 && WGSLDetail::MatrixColumns(left) == 0) {
                        return left;
                    }
                }
            }
        }

        return { };
    }
}
