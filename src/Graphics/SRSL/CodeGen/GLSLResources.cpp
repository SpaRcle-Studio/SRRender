//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/GLSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>

namespace SR_SRSL_NS {
    void GLSLGenerator::GenerateSSBOBlock(SR_UTILS_NS::String& code, SR_UTILS_NS::StringAtom name, const SRSLUniformBlock& uniformBlock, const SRSLUseStack::Ptr& pFunction) const {
        SR_TRACY_ZONE;
        const SR_UTILS_NS::StringView accessMode = uniformBlock.isReadOnly ? (uniformBlock.isReadOnly.value() ? " readonly" : " writeonly") : "";

        SR_UTILS_NS::String blockCode;
        blockCode.reserve(256 + uniformBlock.fields.size() * 64);
        SR_UTILS_NS::FormatTo(blockCode, "layout (set = 0, binding = {}){}{}{}{} buffer StorageBuffer_{} {{\n",
            uniformBlock.binding, accessMode, uniformBlock.isCoherent ? " coherent" : "",
            uniformBlock.isVolatile ? " volatile" : "", uniformBlock.isRestrict ? " restrict" : "", name);

        for (auto&& field : uniformBlock.fields) {
            SR_UTILS_NS::StringView fieldType = field.type;
            SR_UTILS_NS::StringView fieldName = field.name;

            auto&& typeName = ReplaceToken(DescribeType(fieldType, m_shader->GetAnalyzedTree()).base, true);
            auto&& dimension = DescribeType(fieldType, m_shader->GetAnalyzedTree()).dimensions;

            SR_UTILS_NS::String strDimension;

            for (auto&& dim : dimension) {
                if (dim == 0) {
                    strDimension += "[]";
                }
                else {
                    SR_UTILS_NS::FormatTo(strDimension, "[{}]", dim);
                }
            }

            SR_UTILS_NS::FormatTo(blockCode, "\t{} {}{};\n", typeName, field.name, strDimension);
        }

        blockCode += "};\n";
        code += blockCode;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateUniforms(ShaderStage stage) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

        auto&& pFunction = m_shader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));
        if (!pFunction) {
            return code;
        }

        /// ------------------------------------------------------------------------------------------------------------

        SR_UTILS_NS::String uniformsCode;

        for (auto&& [name, uniformBlock] : m_shader->GetUniformBlocks()) {
            SR_UTILS_NS::String blockCode;
            blockCode.reserve(256 + uniformBlock.fields.size() * 64);
            SR_UTILS_NS::FormatTo(blockCode, "layout (std140, binding = {}) uniform {} {{\n", uniformBlock.binding, name);
            bool hasUsage = false;

            for (auto&& field : uniformBlock.fields) {
            #ifdef SR_DEBUG
                if (name != "SHARED" && SR_SRSL_DEFAULT_SHARED_UNIFORMS.count(field.name) > 0) {
                    SRHalt("GLSLGenerator::GenerateUniforms() : default shared uniform '{}' found in uniform block '{}'! "
                        "\n\tShader: {}", field.name, name, m_shader->GetPath()
                    );
                }
            #endif

                hasUsage |= pFunction->IsVariableUsed(field.name);

                auto&& typeName = ReplaceToken(DescribeType(field.type, m_shader->GetAnalyzedTree()).base, true);
                auto&& dimension = DescribeType(field.type, m_shader->GetAnalyzedTree()).dimensions;

                SR_UTILS_NS::String strDimension;

                for (auto&& dim : dimension) {
                    SR_UTILS_NS::FormatTo(strDimension, "[{}]", dim);
                }

                SR_UTILS_NS::FormatTo(blockCode, "\t// ({} bytes) {}\n", field.size, field.isPublic ? "public" : "private");
                SR_UTILS_NS::FormatTo(blockCode, "\t{} {}{};\n", typeName, field.name, strDimension);
            }

            blockCode += "};\n";

            if (hasUsage) {
                uniformsCode += blockCode;
            }
        }

        for (auto&& [name, uniformBlock] : m_shader->GetSSBOBlocks()) {
            GenerateSSBOBlock(uniformsCode, name, uniformBlock, pFunction);
        }

        /// ------------------------------------------------------------------------------------------------------------

        SR_UTILS_NS::String samplersCode;

        for (auto&& [name, sampler] : m_shader->GetSamplers()) {
            if (pFunction->IsVariableUsed(name)) {
                SR_UTILS_NS::String layout;

                if (sampler.attachment >= 0) {
                    SR_UTILS_NS::FormatTo(layout, "(input_attachment_index = {}, binding = {})", sampler.attachment, sampler.binding);
                }
                else {
                    SR_UTILS_NS::FormatTo(layout, "(binding = {})", sampler.binding);
                }

                SR_UTILS_NS::FormatTo(samplersCode, "layout {} uniform {} {}; // (sampler) {}\n",
                        layout,
                        sampler.type.ToStringView() == "sampler2DArrayDepth" ? "sampler2DArray" : sampler.type,
                        name,
                        sampler.isPublic ? "public" : "private"
                );
            }
        }

        /// ------------------------------------------------------------------------------------------------------------

        SR_UTILS_NS::String pushConstantsCode;

        if (!m_shader->GetPushConstants().fields.empty()) {
            SR_UTILS_NS::String blockCode = "layout(push_constant) uniform PushConstants {\n";
            bool hasUsage = false;

            for (auto&& field : m_shader->GetPushConstants().fields) {
                hasUsage |= pFunction->IsVariableUsed(field.name);

                auto&& typeName = ReplaceToken(DescribeType(field.type, m_shader->GetAnalyzedTree()).base, true);

                auto&& dimension = DescribeType(field.type, m_shader->GetAnalyzedTree()).dimensions;

                SR_UTILS_NS::String strDimension;

                for (auto&& dim : dimension) {
                    SR_UTILS_NS::FormatTo(strDimension, "[{}]", dim);
                }

                SR_UTILS_NS::FormatTo(blockCode, "\t// ({} bytes) {}\n", field.size, field.isPublic ? "public" : "private");
                SR_UTILS_NS::FormatTo(blockCode, "\t{} {}{};\n", typeName, field.name, strDimension);
            }

            blockCode += "};\n";

            if (hasUsage) {
                pushConstantsCode += blockCode;
            }
        }

        /// ------------------------------------------------------------------------------------------------------------

        code += uniformsCode;

        if (!samplersCode.empty() && !uniformsCode.empty()) {
            code += "\n";
        }

        code += samplersCode;

        if (!pushConstantsCode.empty() && (!samplersCode.empty() || !uniformsCode.empty())) {
            code += "\n";
        }

        code += pushConstantsCode;

        return code;
    }
}
