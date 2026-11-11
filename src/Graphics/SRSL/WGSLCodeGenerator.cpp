#include <Graphics/SRSL/WGSLCodeGenerator.h>
#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    ISRSLCodeGenerator::SRSLCodeGenRes WGSLCodeGenerator::GenerateStages(SR_UTILS_NS::IAllocator* allocator, const SRSLShader* shader) {
        SR_TRACY_ZONE;
        SR_GLOBAL_LOCK
        WGSLGenerator generator;
        return generator.GenerateStages(allocator, shader);
    }

    ISRSLCodeGenerator::SRSLCodeGenRes WGSLGenerator::GenerateStages(SR_UTILS_NS::IAllocator*, const SRSLShader* pShader) {
        SR_TRACY_ZONE;

        Clear();
        m_variableTypes.reserve(64);
        m_functionReturnTypes.reserve(32);
        m_typeBindings.reserve(64);
        m_pCurrentShader = pShader;
        ISRSLCodeGenerator::SRSLCodeGenRes codeGenRes;

        auto&& [result, stages] = codeGenRes;

        if (!pShader || !pShader->GetAnalyzedTree() || !pShader->GetAnalyzedTree()->pLexicalTree) {
            result = SR_UTILS_NS::LexerDetails::LexerResult(SR_UTILS_NS::LexerDetails::LexerReturnCode::InvalidLexicalTree);
            return codeGenRes;
        }

        for (auto&& [name, sampler] : pShader->GetSamplers()) {
            if (sampler.attachment >= 0) {
                m_attachmentSamplerNames.insert(SR_UTILS_NS::String(name.ToStringView()));
            }
        }

        auto& code = stages[ShaderStage::All];
        code.reserve(4096);

        code += "/// [WARNING: THIS FILE WAS CREATED BY SRSL CODE GENERATION]\n\n";
        if (pShader->IsGLayerUsed()) {
            SR_ERROR("WGSL cannot generate GLayer. Use the per-layer render path for this shader.");
            result = SR_UTILS_NS::LexerDetails::LexerResult(SR_UTILS_NS::LexerDetails::LexerReturnCode::InvalidExpression);
            stages.clear();
            return codeGenRes;
        }
        CodeGenDetail::Append(code, "/// Shader type: ", SR_UTILS_NS::EnumReflector::ToStringAtom(pShader->GetType()).ToStringRef(), "\n\n");

        CollectDeclaredTypes(pShader);

        for (const auto* unit : pShader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
            if (unit->GetLexicalUnitType() != LexicalUnitType::Struct) { continue; }
            const auto* structure = static_cast<const SRSLStructureStatement*>(unit);
            bool used = false;
            for (const auto& [stage, entry] : SR_SRSL_ENTRY_POINTS) {
                if (const auto function = pShader->GetUseStack()->FindFunction(entry)) {
                    used |= function->IsStructUsed(structure->pName->token);
                }
            }
            if (used) {
                auto structureCode = GenerateStructure(structure, 0);
                if (!structureCode.empty()) { CodeGenDetail::Append(code, structureCode, ";\n\n"); }
            }
        }

        // ---- Uniforms / SSBOs / Samplers (emitted once for the whole module) ----
        {
            auto&& uniformsCode = GenerateUniforms(pShader);
            if (!uniformsCode.empty()) {
                code += uniformsCode;
                code += "\n";
            }
        }

        // constants (emitted once for the whole module)
        {
            auto&& constantsCode = GenerateConstants(pShader);
            if (!constantsCode.empty()) {
                code += constantsCode;
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
                SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                SR_UTILS_NS::String attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                SR_UTILS_NS::FormatTo(code, "\t@location({}) {}_INPUT : {},\n", i, attributeName, attributeType);
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
                SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                const SR_UTILS_NS::String attributeType = SR_UTILS_NS::String(WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count));
                /// целочисленные интерполянты в WGSL обязаны быть flat
                const SR_UTILS_NS::StringView interpolate = WGSLDetail::NeedsFlatInterpolation(attributeType) ? "@interpolate(flat) " : "";
                SR_UTILS_NS::FormatTo(code, "\t@location({}) {}{} : {},\n", locationIdx++, interpolate, attributeName, attributeType);
            });
            // Add shared (inter-stage) variables as interpolants with "_s_" prefix
            for (auto&& [name, pVariable] : sharedVars) {
                const SR_UTILS_NS::StringView typeName = GetSharedVariableType(pShader, pVariable);
                if (typeName.empty()) {
                    continue;
                }
                /// Матрицы нельзя передавать между стадиями — разбиваем их на столбцы
                if (const uint32_t columns = WGSLDetail::MatrixColumns(typeName)) {
                    const SR_UTILS_NS::StringView columnType = WGSLDetail::MatrixColumnType(typeName);
                    for (uint32_t column = 0; column < columns; ++column) {
                        SR_UTILS_NS::FormatTo(code, "\t@location({}) _s_{}_{} : {},\n", locationIdx++, name.ToStringView(), column, columnType);
                    }
                }
                else {
                    const SR_UTILS_NS::StringView interpolate = WGSLDetail::NeedsFlatInterpolation(typeName) ? "@interpolate(flat) " : "";
                    SR_UTILS_NS::FormatTo(code, "\t@location({}) {}_s_{} : {},\n", locationIdx++, interpolate, name.ToStringView(), typeName);
                }
            }
            code += "};\n\n";
        }

        // Private module-scope vars for vertex attributes (writable from vertex/fragment functions)
        if (vertexLayoutDescriptions.GetAttributesCount() > 0) {
            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                SR_UTILS_NS::String attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                SR_UTILS_NS::FormatTo(code, "var<private> {} : {};\n", attributeName, attributeType);
            });
            code += "\n";
        }

        // Private module-scope vars for shared (inter-stage) variables
        for (auto&& [name, pVariable] : sharedVars) {
            const SR_UTILS_NS::StringView typeName = GetSharedVariableType(pShader, pVariable);
            if (!typeName.empty()) {
                SR_UTILS_NS::FormatTo(code, "var<private> {} : {};\n", name.ToStringView(), typeName);
            }
        }
        if (!sharedVars.empty()) {
            code += "\n";
        }

        // ---- FragmentOutput struct ----
        auto&& fragmentEntryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment);
        auto&& pFragmentStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(fragmentEntryPoint);

        bool hasFragmentOutput = false;
        for (auto&& macro : SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO) {
            hasFragmentOutput = hasFragmentOutput || pShader->IsMacroDefined(macro);
        }

        if (pFragmentStageFunction && hasFragmentOutput) {
            code += "struct FragmentOutput {\n";
            uint64_t outLocation = 0;
            for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
                if (pShader->IsMacroDefined(SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[outLocation])) {
                    SR_UTILS_NS::FormatTo(code, "\t@location({}) {} : vec4<f32>,\n", outLocation, layer.c_str());
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

        /// Код стадий генерируется в отдельный буфер: только после этого известно,
        /// какие вспомогательные функции понадобились (их нужно объявить до использования).
        SR_UTILS_NS::String stagesCode;

        if (auto stageCode = GenerateVertexStage(pShader); !stageCode.empty()) {
            stagesCode += stageCode;
            stagesCode += "\n";
            hasVertex = true;
        }

        if (auto stageCode = GenerateFragmentStage(pShader); !stageCode.empty()) {
            stagesCode += stageCode;
            stagesCode += "\n";
            hasFragment = true;
        }

        if (auto stageCode = GenerateComputeStage(pShader); !stageCode.empty()) {
            stagesCode += stageCode;
            hasCompute = true;
        }

        code += GenerateHelperFunctions();
        code += stagesCode;

        if (hasVertex) { stages[ShaderStage::Vertex] = code; }
        if (hasFragment) { stages[ShaderStage::Fragment] = code; }
        if (hasCompute) { stages[ShaderStage::Compute] = code; }
        stages.erase(ShaderStage::All);

        result = SR_UTILS_NS::Exchange(m_result, { });

        m_pCurrentShader = nullptr;
        return codeGenRes;
    }
}
