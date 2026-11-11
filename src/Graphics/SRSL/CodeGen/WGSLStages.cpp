//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    SR_UTILS_NS::String WGSLGenerator::GenerateDependencies(const SRSLShader* pShader, ShaderStage stage) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

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
                        SR_UTILS_NS::String rawType = SR_UTILS_NS::String(pVariable->pType->ToString(0, m_tmpBuffer));
                        // GetTypeName strips array brackets → returns base type ("float")
                        SR_UTILS_NS::String strippedType = DescribeType(rawType, pShader->GetAnalyzedTree()).base;
                        SR_UTILS_NS::String baseType = WGSLDetail::GenerateType(strippedType.empty() ? rawType : strippedType);
                        // Get dimensions using the raw SRSL type string
                        auto dims = DescribeType(rawType, pShader->GetAnalyzedTree()).dimensions;
                        SR_UTILS_NS::String varType = WGSLDetail::ToWGSLArrayType(baseType, dims);
                        if (!varType.empty()) {
                            SR_UTILS_NS::FormatTo(code, "var<workgroup> {} : {};\n", name.ToStringView(), varType);
                        }
                    }
                }
            }
        }

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

                    if (m_emittedFunctions.insert(pFunction).second) {
                        AppendFunctionBody(code, pFunction, 0);
                        code += "\n\n";
                    }
                }
            }
        }

        return code;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateVertexStage(const SRSLShader* pShader) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String resultCode;
        SR_UTILS_NS::String preArgs;
        SR_UTILS_NS::String preCode;
        SR_UTILS_NS::String postCode;


        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Vertex);
        auto&& pStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = pShader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction) {
            return { };
        }

        resultCode = GenerateDependencies(pShader, ShaderStage::Vertex);

        const bool isOutPositionUsed = pUseStackFunction->IsVariableUsed("OUT_POSITION");

        // Inject UBO field aliases, SSBO scalar aliases and push constant aliases (must be inside function, not module scope)
        preCode += GenerateUniformBlockAliases(pShader, ShaderStage::Vertex);
        preCode += GenerateSSBOBlockAliases(pShader, ShaderStage::Vertex);
        preCode += GeneratePushConstantAliases(pShader, ShaderStage::Vertex);

        preCode += GenerateTab(1);
        preCode += "var vsOut : VertexOutput;\n";
        if (isOutPositionUsed) {
            preCode += GenerateTab(1);
            preCode += "var OUT_POSITION : vec4<f32>;\n";
        }

        auto&& vertexLayoutDescriptions = pShader->GetCreateInfo().vertexLayoutDescriptions;
        if (vertexLayoutDescriptions.GetAttributesCount() > 0) {
            preArgs += "input : VertexInput";
        }

        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            SR_UTILS_NS::FormatTo(preCode, "{}{} = input.{}_INPUT;\n", GenerateTab(1), attributeName, attributeName);
        });

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("VERTEX_INDEX")) {
            preCode += GenerateTab(1);
            preCode += "var VERTEX_INDEX : u32 = vertexIndex;\n";
            if (!preArgs.empty()) {
                preArgs += ", ";
            }
            preArgs += "@builtin(vertex_index) vertexIndex : u32";
        }

        if (isOutPositionUsed) {
            if (pShader->GetType() == ShaderType::PostProcessing) {
                postCode += GenerateTab(1);
                postCode += "OUT_POSITION.y = -OUT_POSITION.y;\n";
            }
            postCode += GenerateTab(1);
            postCode += "vsOut.position = OUT_POSITION;\n";
        }
        else if (vertexLayoutDescriptions.Find(SR_UTILS_NS::VertexAttribute::Position)) {
            postCode += GenerateTab(1);
            postCode += "vsOut.position = vec4<f32>(VERTEX, 1.0);\n";
        }

        // Copy vertex attributes into vsOut
        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            SR_UTILS_NS::FormatTo(postCode, "{}vsOut.{} = {};\n", GenerateTab(1), attributeName, attributeName);
        });

        // Copy shared vars into vsOut for interpolation to fragment (using _s_ prefix to avoid name clash)
        for (auto&& [name, pVariable] : pShader->GetShared()) {
            const SR_UTILS_NS::StringView typeName = GetSharedVariableType(pShader, pVariable);
            if (typeName.empty()) {
                continue;
            }
            /// Матрицы передаются постолбцово — WGSL не разрешает матрицы в межстадийном интерфейсе
            if (const uint32_t columns = WGSLDetail::MatrixColumns(typeName)) {
                for (uint32_t column = 0; column < columns; ++column) {
                    SR_UTILS_NS::FormatTo(postCode, "{}vsOut._s_{}_{} = {}[{}];\n", GenerateTab(1), name.ToStringView(), column, name.ToStringView(), column);
                }
            }
            else {
                SR_UTILS_NS::FormatTo(postCode, "{}vsOut._s_{} = {};\n", GenerateTab(1), name.ToStringView(), name.ToStringView());
            }
        }

        postCode += GenerateTab(1);
        postCode += "return vsOut;\n";

        resultCode += "@vertex\n";

        static const SR_UTILS_NS::String vertexOutputId = "VertexOutput";
        AppendFunction(resultCode, pStageFunction, 0, preArgs, preCode, postCode, vertexOutputId);

        return resultCode;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateFragmentStage(const SRSLShader* pShader) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        SR_UTILS_NS::String preCode;
        SR_UTILS_NS::String postCode;


        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment);
        auto&& pStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = pShader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction || !pUseStackFunction) {
            return { };
        }

        const bool isColorPassDefined = pShader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = pShader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);
        const bool isColorUsed = isColorPassDefined || pUseStackFunction->IsVariableUsed("COLOR");
        const bool isFragCoordUsed = pUseStackFunction->IsVariableUsed("FRAG_COORD");

        code = GenerateDependencies(pShader, ShaderStage::Fragment);

        // Inject UBO field aliases, SSBO scalar aliases and push constant aliases
        preCode += GenerateUniformBlockAliases(pShader, ShaderStage::Fragment);
        preCode += GenerateSSBOBlockAliases(pShader, ShaderStage::Fragment);
        preCode += GeneratePushConstantAliases(pShader, ShaderStage::Fragment);

        // Fragment input: vertex output interpolants
        SR_UTILS_NS::String fragArgs;
        auto&& vertexLayoutDescriptions = pShader->GetCreateInfo().vertexLayoutDescriptions;
        auto&& sharedVarsF = pShader->GetShared();
        const bool needsFsIn = vertexLayoutDescriptions.GetAttributesCount() > 0
                            || isFragCoordUsed
                            || !sharedVarsF.empty();
        if (needsFsIn) {
            fragArgs = "fsIn : VertexOutput";
        }

        if (isFragCoordUsed) {
            preCode += GenerateTab(1);
            preCode += "var FRAG_COORD : vec4<f32> = fsIn.position;\n\n";
        }

        // Copy vertex interpolants into private vars
        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            SR_UTILS_NS::FormatTo(preCode, "{}{} = fsIn.{};\n", GenerateTab(1), attributeName, attributeName);
        });

        // Copy shared (inter-stage) interpolants into private vars (using _s_ prefix from VertexOutput)
        for (auto&& [name, pVariable] : sharedVarsF) {
            const SR_UTILS_NS::StringView typeName = GetSharedVariableType(pShader, pVariable);
            if (typeName.empty()) {
                continue;
            }
            /// Матрица собирается обратно из столбцов
            if (const uint32_t columns = WGSLDetail::MatrixColumns(typeName)) {
                SR_UTILS_NS::FormatTo(preCode, "{}{} = {}(", GenerateTab(1), name.ToStringView(), typeName);
                for (uint32_t column = 0; column < columns; ++column) {
                    SR_UTILS_NS::FormatTo(preCode, "{}fsIn._s_{}_{}", column > 0 ? ", " : "", name.ToStringView(), column);
                }
                preCode += ");\n";
            }
            else {
                SR_UTILS_NS::FormatTo(preCode, "{}{} = fsIn._s_{};\n", GenerateTab(1), name.ToStringView(), name.ToStringView());
            }
        }

        uint64_t outLocation = 0;
        for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
            const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
            if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                preCode += GenerateTab(1);
                SR_UTILS_NS::FormatTo(preCode, "var {} : vec4<f32>; /// location {}\n", layer, outLocation);
            }
            ++outLocation;
        }

        bool hasFragmentOutput = false;
        for (auto&& macro : SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO) {
            hasFragmentOutput = hasFragmentOutput || pShader->IsMacroDefined(macro);
        }
        if (hasFragmentOutput) {
            preCode += GenerateTab(1);
            preCode += "var fsOut : FragmentOutput;\n";
        }

        if (isColorUsed) {
            postCode += GenerateTab(1);
            postCode += SR_SRSL_MAIN_OUT_LAYER;
            postCode += " = COLOR;\n";
        }

        // Color buffer pass code
        if (isColorPassDefined) {
            const bool discardExists = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_color_buffer_discard") != nullptr;
            if (discardExists) {
                postCode += GenerateTab(1);
                postCode += "fragment_color_buffer_discard();\n";
            }
            preCode += GenerateTab(1);
            SR_UTILS_NS::FormatTo(preCode, "COLOR = vec4<f32>({}, 1.0);\n", SHADER_PC_COLOR_BUFFER_VALUE);
        }
        else if (isCascadedMapPassDefined) {
            const bool discardExists = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_depth_buffer_discard") != nullptr;
            if (discardExists) {
                postCode += GenerateTab(1);
                postCode += "fragment_depth_buffer_discard();\n";
            }
        }

        if (hasFragmentOutput) {
            outLocation = 0;
            for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
                const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
                if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                    if (pShader->IsMacroDefined(SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[outLocation])) {
                        postCode += GenerateTab(1);
                        SR_UTILS_NS::FormatTo(postCode, "fsOut.{} = {};\n", layer, layer);
                    }
                }
                ++outLocation;
            }

            postCode += GenerateTab(1);
            postCode += "return fsOut;\n";
        }

        code += "@fragment\n";
        static const SR_UTILS_NS::String fragmentOutputId = "FragmentOutput";
        AppendFunction(code, pStageFunction, 0, fragArgs, preCode, postCode,
            hasFragmentOutput ? SR_UTILS_NS::StringView(fragmentOutputId) : SR_UTILS_NS::StringView());

        return code;
    }

    SR_UTILS_NS::String WGSLGenerator::GenerateComputeStage(const SRSLShader* pShader) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        SR_UTILS_NS::String preCode;
        SR_UTILS_NS::String preArgs;


        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Compute);
        auto&& pStageFunction = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = pShader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction || !pUseStackFunction) {
            return { };
        }

        // Inject UBO field aliases, SSBO scalar aliases and push constant aliases
        preCode += GenerateUniformBlockAliases(pShader, ShaderStage::Compute);
        preCode += GenerateSSBOBlockAliases(pShader, ShaderStage::Compute);
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
            preCode += GenerateTab(1);
            preCode += "var GLOBAL_INVOCATION_ID : vec3<u32> = global_id;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("WORK_GROUP_ID")) {
            preCode += GenerateTab(1);
            preCode += "var WORK_GROUP_ID : vec3<u32> = workgroup_id;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("NUM_WORK_GROUPS")) {
            preCode += GenerateTab(1);
            preCode += "var NUM_WORK_GROUPS : vec3<u32> = num_workgroups;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_ID")) {
            preCode += GenerateTab(1);
            preCode += "var LOCAL_INVOCATION_ID : vec3<u32> = local_id;\n";
        }

        if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_INDEX")) {
            preCode += GenerateTab(1);
            preCode += "var LOCAL_INVOCATION_INDEX : u32 = local_index;\n";
        }

        // Emit @workgroup_size decorator
        auto&& computeGroupSize = pShader->GetComputeWorkGroupSize();
        SR_UTILS_NS::String workgroupDecorator;
        workgroupDecorator.reserve(96);
        SR_UTILS_NS::FormatTo(workgroupDecorator, "@compute @workgroup_size({}, {}, {})\n",
            computeGroupSize.x, computeGroupSize.y, computeGroupSize.z);

        // Check for THREADS decorator on the compute function
        if (pStageFunction->pDecorators) {
            auto&& threadsDecorator = pStageFunction->pDecorators->Find("THREADS");
            if (threadsDecorator && threadsDecorator->args.size() == 3) {
                workgroupDecorator.clear();
                SR_UTILS_NS::FormatTo(workgroupDecorator, "@compute @workgroup_size({}, {}, {})\n",
                    threadsDecorator->args[0]->token,
                    threadsDecorator->args[1]->token,
                    threadsDecorator->args[2]->token
                );
            }
        }

        code = GenerateDependencies(pShader, ShaderStage::Compute);
        code += workgroupDecorator;
        AppendFunction(code, pStageFunction, 0, preArgs, preCode);

        return code;
    }
}
