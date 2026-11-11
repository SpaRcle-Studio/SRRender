//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    SR_UTILS_NS::String WGSLGenerator::GenerateUniforms(const SRSLShader* pShader) const {
        SR_TRACY_ZONE;
        // This function generates the combined uniforms block for the whole shader module.
        // It should be called ONCE per shader, not per stage.
        SR_UTILS_NS::String code;

        // ---- Uniform Blocks → @group(0) @binding(N) var<uniform> Name : StructName; ----
        for (auto&& [name, uniformBlock] : pShader->GetUniformBlocks()) {
            // Emit an anonymous struct for the UBO fields
            SR_UTILS_NS::String structName;
            structName.reserve(name.size() + 2);
            CodeGenDetail::Append(structName, name.ToStringView(), "_t");
            CodeGenDetail::Append(code, "struct ", structName, " {\n");
            for (auto&& field : uniformBlock.fields) {
                auto&& baseType = WGSLDetail::GenerateType(
                    DescribeType(field.type, pShader->GetAnalyzedTree()).base
                );
                if (baseType.empty()) { continue; }
                SR_UTILS_NS::StringView fieldType = GetResourceType(field.type);
                SR_UTILS_NS::FormatTo(code, "\t// ({} bytes) {}\n", field.size, field.isPublic ? "public" : "private");
                SR_UTILS_NS::FormatTo(code, "\t{} : {},\n", field.name.ToStringView(), fieldType);
            }
            code += "};\n";
            SR_UTILS_NS::FormatTo(code, "@group(0) @binding({}) var<uniform> {} : {};\n\n",
                uniformBlock.binding, name.ToStringView(), structName);
        }

        // ---- SSBO Blocks → struct StorageBuffer_<Name> + var<storage> ----
        // Always emit a wrapper struct so that field names remain stable and accessible
        // via blockVar.fieldName. The expression generator rewrites bare field references
        // (e.g. `samples[i]`) to their qualified form (e.g. `SSAOKernel.samples[i]`).
        m_ssboFieldToQualified.clear();
        for (auto&& [name, ssboBlock] : pShader->GetSSBOBlocks()) {
            // Determine access mode
            SR_UTILS_NS::StringView accessMode = "read_write";
            if (ssboBlock.isReadOnly.has_value()) {
                accessMode = ssboBlock.isReadOnly.value() ? "read" : "read_write";
            }

            const SR_UTILS_NS::StringView blockVarName = name.ToStringView();
            SR_UTILS_NS::String structName;
            structName.reserve(14 + blockVarName.size());
            CodeGenDetail::Append(structName, "StorageBuffer_", blockVarName);

            CodeGenDetail::Append(code, "struct ", structName, " {\n");
            for (auto&& field : ssboBlock.fields) {
                auto&& baseType = WGSLDetail::GenerateType(
                    DescribeType(field.type, pShader->GetAnalyzedTree()).base
                );
                if (baseType.empty()) { continue; }
                SR_UTILS_NS::StringView fieldType = GetResourceType(field.type);
                SR_UTILS_NS::FormatTo(code, "\t{} : {},\n", field.name.ToStringView(), fieldType);

                // Register field → qualified access for expression rewriting
                const SR_UTILS_NS::StringView fieldName = field.name.ToStringView();
                auto& qualifiedName = m_ssboFieldToQualified[fieldName];
                qualifiedName = SR_UTILS_NS::String(&m_scratch);
                qualifiedName.reserve(blockVarName.size() + 1 + fieldName.size());
                CodeGenDetail::Append(qualifiedName, blockVarName, ".", fieldName);
            }
            code += "};\n";
            SR_UTILS_NS::FormatTo(code, "@group(0) @binding({}) var<storage, {}> {} : {};\n\n",
                ssboBlock.binding, accessMode, blockVarName, structName);
        }

        // ---- Samplers → @group(1) @binding(N) var tex : texture_2d<f32> + sampler ----
        uint32_t samplerBinding = 0;
        for (auto&& [name, sampler] : pShader->GetSamplers()) {
            /// Размерность обязана совпадать с той, что бэкенд объявляет в layout'е,
            /// поэтому берется из общего маппинга (см. SamplerDimensionFromTypeName)
            SR_UTILS_NS::StringView wgslTextureType = "texture_2d<f32>";
            switch (SamplerDimensionFromTypeName(sampler.type.ToStringView())) {
                case SamplerDimension::Texture3D:
                    wgslTextureType = "texture_3d<f32>";
                    break;
                case SamplerDimension::TextureCube:
                    wgslTextureType = "texture_cube<f32>";
                    break;
                case SamplerDimension::Texture2DArray:
                    wgslTextureType = "texture_2d_array<f32>";
                    break;
                case SamplerDimension::Texture2DArrayDepth:
                    wgslTextureType = "texture_depth_2d_array";
                    break;
                case SamplerDimension::Texture2DShadow:
                    wgslTextureType = "texture_depth_2d";
                    break;
                default:
                    break;
            }

            SR_UTILS_NS::FormatTo(code, "@group(1) @binding({}) var {} : {};\n", samplerBinding * 2, name.ToStringView(), wgslTextureType);
            SR_UTILS_NS::FormatTo(code, "@group(1) @binding({}) var {}_sampler : sampler;\n\n", samplerBinding * 2 + 1, name.ToStringView());
            ++samplerBinding;
        }

        // ---- Push constants → emulated as @group(2) @binding(0) var<uniform> PushConstants : PushConstants_t ----
        // WGSL does NOT allow module-scope variable initializers that reference uniform buffers.
        // We emit only the struct and binding declaration here; the field aliases are emitted
        // as function-local `let` statements in preCode of each stage function.
        if (!pShader->GetPushConstants().fields.empty()) {
            code += "struct PushConstants_t {\n";
            for (auto&& field : pShader->GetPushConstants().fields) {
                    const SR_UTILS_NS::StringView typeName = GetResourceType(field.type);
                if (typeName.empty()) { continue; }
                SR_UTILS_NS::FormatTo(code, "\t{} : {},\n", WGSLDetail::SafeIdentifier(field.name.ToStringView()), typeName);
            }
            code += "};\n";
            code += "@group(2) @binding(0) var<uniform> _pushConstants : PushConstants_t;\n\n";
        }

        return code;
    }

    SR_UTILS_NS::String WGSLGenerator::GeneratePushConstantAliases(const SRSLShader* pShader, ShaderStage stage) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

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
            const SR_UTILS_NS::StringView typeName = GetResourceType(field.type);
            if (typeName.empty()) { continue; }
            const SR_UTILS_NS::String safeFieldName = WGSLDetail::SafeIdentifier(field.name.ToStringView());
            SR_UTILS_NS::FormatTo(code, "{}let {} : {} = _pushConstants.{};\n",
                GenerateTab(1), field.name.ToStringView(), typeName, safeFieldName);
        }

        return code;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateUniformBlockAliases(const SRSLShader* pShader, ShaderStage stage) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

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
                    const SR_UTILS_NS::StringView typeName = GetResourceType(field.type);
                if (typeName.empty()) { continue; }
                const SR_UTILS_NS::String safeField = WGSLDetail::SafeIdentifier(field.name.ToStringView());
                SR_UTILS_NS::FormatTo(code, "{}let {} : {} = {}.{};\n",
                    GenerateTab(1), field.name.ToStringView(), typeName,
                    blockName.ToStringView(), safeField);
            }
        }

        return code;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateSSBOBlockAliases(const SRSLShader* pShader, ShaderStage stage) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

        auto&& pFunction = pShader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));
        if (!pFunction) {
            return code;
        }

        // For SSBO blocks, dynamic-array fields are rewritten at expression level via m_ssboFieldToQualified.
        // Non-array fields (e.g. a plain uint counter) can be aliased with a let-binding.
        for (auto&& [blockName, ssboBlock] : pShader->GetSSBOBlocks()) {
            for (auto&& field : ssboBlock.fields) {
                if (!pFunction->IsVariableUsed(field.name)) {
                    continue;
                }
                auto&& dims = DescribeType(field.type, pShader->GetAnalyzedTree()).dimensions;
                // Dynamic arrays cannot be aliased with let — they are handled by expression rewriting
                if (!dims.empty() && dims.back() == 0) {
                    continue;
                }
                auto&& typeName = WGSLDetail::GenerateType(
                    DescribeType(field.type, pShader->GetAnalyzedTree()).base
                );
                if (typeName.empty()) { continue; }
                const SR_UTILS_NS::String safeField = WGSLDetail::SafeIdentifier(field.name.ToStringView());
                SR_UTILS_NS::FormatTo(code, "{}let {} : {} = {}.{};\n",
                    GenerateTab(1), field.name.ToStringView(), typeName,
                    blockName.ToStringView(), safeField);
            }
        }

        return code;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateConstants(const SRSLShader* pShader) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        if (!pShader->GetConstants().empty()) { code.reserve(1024); }
        for (const auto& [name, variable] : pShader->GetConstants()) {
            if (!variable->pType || !variable->pExpr) { continue; }
            m_tmpBuffer.clear();
            const auto rawType = variable->pType->ToString(0, m_tmpBuffer);
            const SR_UTILS_NS::String baseType = WGSLDetail::GenerateType(DescribeType(rawType, pShader->GetAnalyzedTree()).base);
            const auto dimensions = SRSLTypeInfo::Instance().GetDimension(variable->pName, pShader->GetAnalyzedTree());
            const auto typeName = WGSLDetail::ToWGSLArrayType(baseType, dimensions);
            if (typeName.empty()) { continue; }
            SR_UTILS_NS::FormatTo(code, "const {} : {} = ", name.ToStringView(), typeName);
            if (dimensions.empty()) {
                AppendExpression(code, variable->pExpr);
            }
            else {
                auto appendArray = [&](auto&& self, const SRSLExpr* expression, size_t depth) -> void {
                    SR_TRACY_ZONE_N("WGSL constant array initializer");
                    for (size_t i = depth; i < dimensions.size(); ++i) { code += "array<"; }
                    code += baseType;
                    for (size_t i = dimensions.size(); i > depth; --i) {
                        SR_UTILS_NS::FormatTo(code, ", {}>", dimensions[i - 1]);
                    }
                    code += '(';
                    for (size_t i = 0; i < expression->args.size(); ++i) {
                        if (i > 0) { code += ", "; }
                        if (depth + 1 < dimensions.size()) { self(self, expression->args[i], depth + 1); }
                        else { AppendExpression(code, expression->args[i]); }
                    }
                    code += ')';
                };
                appendArray(appendArray, variable->pExpr, 0);
            }
            code += ";\n";
        }
        return code;
    }
}
