//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    SR_UTILS_NS::String WGSLGenerator::GenerateExpression(const SRSLExpr* expression, int32_t deep) const {
        SR_TRACY_ZONE;
        if (!expression) {
            return {};
        }
        SR_UTILS_NS::String code;
        code.reserve(128);
        AppendExpression(code, expression, deep);
        return code;
    }

    void WGSLGenerator::AppendExpression(SR_UTILS_NS::String& code, const SRSLExpr* expression, int32_t deep) const {
        SR_TRACY_ZONE;
        if (!expression) {
            return;
        }
        code += GenerateTab(deep);
        auto emit = [&](const SRSLExpr* child) { AppendExpression(code, child); };
        if (CodeGenDetail::AppendCommonExpression(code, expression, emit)) {
            return;
        }
        if (expression->token == "." && expression->args.size() == 2) {
            emit(expression->args[0]);
            code += '.';
            const auto* member = expression->args[1];
            if (member->args.empty()) {
                WGSLDetail::AppendIdentifier(code, member->token);
            } else {
                emit(member);
            }
            return;
        }
        if (!expression->isCall && !expression->isArray && !expression->isList && expression->args.empty() &&
            expression->token != "++" && expression->token != "--") {
            const auto field = m_ssboFieldToQualified.find(expression->token);
            if (field != m_ssboFieldToQualified.end() && !IsLocalVariable(expression->token)) {
                code += field->second;
            } else {
                WGSLDetail::AppendToken(code, expression->token);
            }
        } else {
            AppendSpecialExpression(code, expression);
        }
    }

    void WGSLGenerator::AppendSpecialExpression(SR_UTILS_NS::String& code, const SRSLExpr* pExpr) const {
        SR_TRACY_ZONE;

        // WGSL does not support prefix ++ / --. Convert to += 1 / -= 1.
        if (pExpr->token == "++" || pExpr->token == "--") {
            const SR_UTILS_NS::StringView op = (pExpr->token == "++") ? " += 1" : " -= 1";
            if (!pExpr->args.empty()) {
                // Post/prefix increment used as statement: var += 1
                AppendExpression(code, pExpr->args[0], 0);
                code += op;
            } else {
                // Standalone ++ / -- token (part of empty-token concatenation pair).
                // The parent expression with token=="" will combine this with the variable.
                code += (pExpr->token == "++") ? "__INC__" : "__DEC__";
            }
        } else if (pExpr->isCall) {
            const SR_UTILS_NS::String& rawToken = pExpr->token;
            const SR_UTILS_NS::StringView funcName = WGSLDetail::ReplaceToken(rawToken);

            if (rawToken == "mod" && pExpr->args.size() == 2) {
                code += "((";
                const auto leftBegin = code.size();
                AppendExpression(code, pExpr->args[0]);
                const auto leftSize = code.size() - leftBegin;
                code += ") - (";
                const auto rightBegin = code.size();
                AppendExpression(code, pExpr->args[1]);
                const auto rightSize = code.size() - rightBegin;
                code += ") * floor((";
                CodeGenDetail::AppendRange(code, leftBegin, leftSize);
                code += ") / (";
                CodeGenDetail::AppendRange(code, rightBegin, rightSize);
                code += ")))";
            }
            // ---- texture() / textureLod() → inject sampler argument ----
            // WGSL requires an explicit sampler: textureSample(tex, tex_sampler, uv)
            // SRSL/GLSL calls: texture(tex, uv)  or  textureLod(tex, uv, lod)
            else if ((rawToken == "texture" || rawToken == "textureLod") && pExpr->args.size() >= 2) {
                // Extract the texture variable name from the first argument
                SR_UTILS_NS::String texName = GenerateExpression(pExpr->args[0], 0);
                // Strip any trailing whitespace / tab indentation from the recursive call
                // (GenerateTab(0) returns empty string, so this should be clean already)

                const SR_UTILS_NS::StringView textureType = InferExprType(pExpr->args[0]);
                if (textureType == "texture_depth_2d_array") {
                    const SR_UTILS_NS::String coords = GenerateExpression(pExpr->args[1], 0);
                    const SR_UTILS_NS::String level = rawToken == "textureLod" && pExpr->args.size() >= 3
                                                          ? "i32(" + GenerateExpression(pExpr->args[2], 0) + ")"
                                                          : "0";
                    CodeGenDetail::Append(code, "vec4<f32>(textureLoad(", texName, ", vec2<i32>(min(vec2<f32>((",
                                          coords, ").x, 1.0 - (", coords, ").y) * vec2<f32>(textureDimensions(",
                                          texName, ")), vec2<f32>(textureDimensions(", texName, ")) - vec2<f32>(1.0)))",
                                          ", i32((", coords, ").z), ", level, "))");
                    return;
                }

                if (rawToken == "textureLod" &&
                    (IsSamplerParameter(texName) || m_attachmentSamplerNames.count(texName) > 0)) {
                    const SR_UTILS_NS::String coords = GenerateExpression(pExpr->args[1], 0);
                    const SR_UTILS_NS::String level =
                        pExpr->args.size() >= 3 ? "i32(" + GenerateExpression(pExpr->args[2], 0) + ")" : "0";
                    CodeGenDetail::Append(code, "textureLoad(", texName, ", clamp(vec2<i32>((", coords,
                                          ") * vec2<f32>(textureDimensions(", texName, ", ", level,
                                          "))), vec2<i32>(0), vec2<i32>(textureDimensions(", texName, ", ", level,
                                          ")) - vec2<i32>(1))", ", ", level, ")");
                    return;
                }

                const bool forceExplicitLod = rawToken == "texture" && m_forceExplicitTextureLod;
                const SR_UTILS_NS::StringView sampleFunction =
                    forceExplicitLod ? SR_UTILS_NS::StringView("textureSampleLevel") : funcName;

                if (textureType.find("texture_2d_array") != SR_UTILS_NS::String::npos) {
                    const SR_UTILS_NS::String coords = GenerateExpression(pExpr->args[1], 0);
                    CodeGenDetail::Append(code, sampleFunction, "(", texName, ", ", texName, "_sampler", ", (", coords,
                                          ").xy, i32((", coords, ").z)");
                    if (rawToken == "textureLod" && pExpr->args.size() >= 3) {
                        code += ", ";
                        AppendExpression(code, pExpr->args[2]);
                    } else if (forceExplicitLod) {
                        code += ", 0.0";
                    }
                    code += ")";
                    return;
                }

                // textureSample(tex, tex_sampler, uv [, lod])
                CodeGenDetail::Append(code, sampleFunction, "(", texName, ", ", texName, "_sampler");
                for (uint32_t i = 1; i < pExpr->args.size(); ++i) {
                    code += ", ";
                    AppendExpression(code, pExpr->args[i]);
                }
                if (forceExplicitLod) {
                    code += ", 0.0";
                }
                code += ")";
            }
            // ---- textureSize(tex, lod) → textureDimensions(tex) ----
            // WGSL's textureDimensions() does not take a mip level for non-mipmapped textures.
            // We drop the second argument (lod) here; for shaders that need mip-aware
            // dimensions, users should use textureDimensions(tex, lod) directly in a future version.
            else if (rawToken == "textureSize" && !pExpr->args.empty()) {
                SR_UTILS_NS::String texName = GenerateExpression(pExpr->args[0], 0);
                CodeGenDetail::Append(code, "textureDimensions(", texName);
                // Pass lod argument if provided (WGSL supports textureDimensions(t, level))
                if (pExpr->args.size() >= 2) {
                    code += ", ";
                    AppendExpression(code, pExpr->args[1]);
                }
                code += ")";
            }
            // ---- inverse(m) → sr_inverseN(m) ----
            // В WGSL нет встроенной inverse(), поэтому подставляем свою реализацию,
            // выбирая её по выведенному типу аргумента.
            else if (rawToken == "inverse" && pExpr->args.size() == 1) {
                uint32_t dimension = WGSLDetail::SquareMatrixDimension(InferExprType(pExpr->args[0]));
                if (dimension == 0) {
                    dimension = 4; /// самый частый случай, если тип вывести не удалось
                }
                const SR_UTILS_NS::StringView helper = WGSLDetail::InverseHelperName(dimension);
                m_inverseHelpers[dimension - 2] = true;
                code += helper;
                code += "(";
                AppendExpression(code, pExpr->args[0], 0);
                code += ")";
            }
            // ---- matN(x) с одним аргументом ----
            // В WGSL нет ни конструктора матрицы из матрицы другого размера, ни из скаляра.
            else if (pExpr->args.size() == 1 &&
                     (rawToken == "mat2" || rawToken == "mat3" || rawToken == "mat4" || rawToken == "mat2x2" ||
                      rawToken == "mat3x3" || rawToken == "mat4x4")) {
                const uint32_t target = static_cast<uint32_t>(rawToken[3] - '0');
                const SR_UTILS_NS::StringView argType = InferExprType(pExpr->args[0]);
                const uint32_t source = WGSLDetail::SquareMatrixDimension(argType);

                if (source == target) {
                    /// тип уже совпадает — конструктор не нужен
                    AppendExpression(code, pExpr->args[0]);
                } else if (source != 0) {
                    const SR_UTILS_NS::StringView helper = WGSLDetail::MatrixCastHelperName(target, source);
                    m_matrixCastHelpers[target - 2][source - 2] = true;
                    CodeGenDetail::Append(code, helper, "(");
                    AppendExpression(code, pExpr->args[0]);
                    code += ')';
                } else if (WGSLDetail::IsScalarType(argType) || WGSLDetail::IsNumericLiteral(pExpr->args[0]->token)) {
                    const SR_UTILS_NS::StringView helper = WGSLDetail::MatrixDiagonalHelperName(target);
                    m_diagonalHelpers[target - 2] = true;
                    CodeGenDetail::Append(code, helper, "(f32(");
                    AppendExpression(code, pExpr->args[0]);
                    code += "))";
                } else if (target < 4) {
                    /// тип неизвестен: усечение mat4 — самый частый случай (например mat3(MODEL_MATRIX))
                    const SR_UTILS_NS::StringView helper = WGSLDetail::MatrixCastHelperName(target, 4);
                    m_matrixCastHelpers[target - 2][4 - 2] = true;
                    CodeGenDetail::Append(code, helper, "(");
                    AppendExpression(code, pExpr->args[0]);
                    code += ')';
                } else {
                    const SR_UTILS_NS::StringView helper = WGSLDetail::MatrixDiagonalHelperName(target);
                    m_diagonalHelpers[target - 2] = true;
                    CodeGenDetail::Append(code, helper, "(f32(");
                    AppendExpression(code, pExpr->args[0]);
                    code += "))";
                }
            } else {
                WGSLDetail::AppendIdentifier(code, funcName);
                code += '(';
                const SRSLFunction* pCalledFunction =
                    m_pCurrentShader && m_pCurrentShader->GetAnalyzedTree()
                        ? m_pCurrentShader->GetAnalyzedTree()->pLexicalTree->FindFunction(rawToken)
                        : nullptr;
                for (uint32_t i = 0; i < pExpr->args.size(); ++i) {
                    const auto argumentBegin = code.size();
                    AppendExpression(code, pExpr->args[i]);
                    const auto argumentSize = code.size() - argumentBegin;
                    if (pCalledFunction && i < pCalledFunction->args.size() && pCalledFunction->args[i]->pType) {
                        m_tmpBuffer.clear();
                        const SR_UTILS_NS::StringView type = pCalledFunction->args[i]->pType->ToString(0, m_tmpBuffer);
                        if (type == "sampler2D" || type == "sampler2DArray" || type == "sampler2DArrayDepth" ||
                            type == "sampler3D" || type == "samplerCube" || type == "sampler2DShadow") {
                            code += ", ";
                            CodeGenDetail::AppendRange(code, argumentBegin, argumentSize);
                            code += "_sampler";
                        }
                    }
                    if (i + 1 < pExpr->args.size()) {
                        code += ", ";
                    }
                }
                code += ")";
            }
        } else if (pExpr->isArray) {
            if (pExpr->args.size() == 1) {
                // array type declaration `type[]` → handled elsewhere; here it's indexing with no index (shouldn't
                // happen)
                AppendExpression(code, pExpr->args[0], 0);
                code += "[]";
            }

            else {
                SRHalt("WGSLGenerator::GenerateExpression() : invalid array expression!");
            }
        } else if (pExpr->isList) {
            // Struct/array literal: use the type constructor style if we know the type,
            // otherwise emit as WGSL array<> initializer via array()
            code += "array(";
            for (uint32_t i = 0; i < pExpr->args.size(); ++i) {
                AppendExpression(code, pExpr->args.at(i));
                if (i + 1 < pExpr->args.size()) {
                    code += ", ";
                }
            }
            code += ")";
        }

        else if (pExpr->args.size() >= 2 && (pExpr->token == "=" || WGSLDetail::IsCompoundAssignment(pExpr->token))) {
            // Assignment / compound-assignment: emit without wrapping parens (WGSL disallows assignment in expression).
            // WGSL does NOT support compound-assignment operators on swizzles (e.g. offset.xyz /= offset.w is illegal).
            // To be safe, always expand every compound-assignment a OP= b → a = a OP b.
            if (WGSLDetail::IsCompoundAssignment(pExpr->token)) {
                // Strip trailing '=' to get the base operator (e.g. "/=" → "/", "+=" → "+")
                const auto baseOp = pExpr->token.ToStringView().substr(0, pExpr->token.size() - 1);
                const auto leftBegin = code.size();
                AppendExpression(code, pExpr->args[0]);
                const auto leftSize = code.size() - leftBegin;
                code += " = (";
                CodeGenDetail::AppendRange(code, leftBegin, leftSize);
                CodeGenDetail::Append(code, " ", baseOp, " ");
                AppendExpression(code, pExpr->args[1]);
                code += ')';
            } else {
                AppendExpression(code, pExpr->args[0], 0);
                code += " ";
                code += pExpr->token;
                code += " ";
                AppendExpression(code, pExpr->args[1], 0);
            }
        } else if (pExpr->args.size() == 2 && pExpr->token.empty()) {
            // Concatenation: used for prefix/postfix ++/-- patterns like { "++", i } or { i, "++" }
            // WGSL doesn't support ++ / --, so we detect and convert to += 1 / -= 1
            const SR_UTILS_NS::String left = GenerateExpression(pExpr->args[0], 0);
            const SR_UTILS_NS::String right = GenerateExpression(pExpr->args[1], 0);

            if (left == "__INC__") {
                CodeGenDetail::Append(code, right, " += 1");
            } else if (left == "__DEC__") {
                CodeGenDetail::Append(code, right, " -= 1");
            } else if (right == "__INC__") {
                CodeGenDetail::Append(code, left, " += 1");
            } else if (right == "__DEC__") {
                CodeGenDetail::Append(code, left, " -= 1");
            } else {
                CodeGenDetail::Append(code, left, right);
            }
        }

        else if (pExpr->args.size() == 2 && pExpr->token == "?") {
            const auto* colon = pExpr->args[1];
            code += "select(";
            if (colon && colon->token == ":" && colon->args.size() == 2) {
                AppendExpression(code, colon->args[1]);
                code += ", ";
                AppendExpression(code, colon->args[0]);
            } else {
                const auto begin = code.size();
                AppendExpression(code, pExpr->args[1]);
                const auto size = code.size() - begin;
                code += ", ";
                CodeGenDetail::AppendRange(code, begin, size);
            }
            code += ", ";
            AppendExpression(code, pExpr->args[0]);
            code += ')';
        } else if (pExpr->args.size() == 2 && pExpr->token == ":") {
            const auto* question = pExpr->args[0];
            if (question && question->token == "?" && question->args.size() == 2) {
                code += "select(";
                AppendExpression(code, pExpr->args[1]);
                code += ", ";
                AppendExpression(code, question->args[1]);
                code += ", ";
                AppendExpression(code, question->args[0]);
                code += ')';
            } else {
                AppendExpression(code, pExpr->args[0]);
            }
        } else if (pExpr->args.size() == 2) {
            code += "(";
            AppendExpression(code, pExpr->args[0], 0);
            code += " ";
            code += WGSLDetail::ReplaceToken(pExpr->token);
            code += " ";
            AppendExpression(code, pExpr->args[1], 0);
            code += ")";
        }

        return;
    }

} // namespace SR_SRSL_NS
