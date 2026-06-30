//
// Created by Monika on 11.04.2026.
//

#include <Graphics/SRSL/WGSLCodeGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>

namespace SR_SRSL_NS {
    namespace WGSLDetail {
        std::string_view GenerateTab(const int32_t deep) {
            static std::string result;
            result.clear();
            if (deep <= 0) {
                return result;
            }

            result.resize(deep * 4);
            memset(result.data(), ' ', deep * 4);
            return result;
        }

        std::string_view GenerateType(std::string_view type) {
            static std::string result;
            result.clear();

            result = "stubType";

            return result;
        }

        std::string_view VertexAttributeFormatToString(SR_UTILS_NS::VertexAttributeFormat format, uint8_t count) {
            static std::string result;
            result.clear();

            switch (format) {
                case SR_UTILS_NS::VertexAttributeFormat::Float32: result = "f32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Int32: result = "i32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UInt32: result = "u32"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Float16: result = "f16"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Int16: result = "i16"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UInt16: result = "u16"; break;
                case SR_UTILS_NS::VertexAttributeFormat::Int8: result = "i8"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UInt8: result = "u8"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UNorm8:
                case SR_UTILS_NS::VertexAttributeFormat::SNorm8: result = "u8"; break;
                case SR_UTILS_NS::VertexAttributeFormat::UNorm16:
                case SR_UTILS_NS::VertexAttributeFormat::SNorm16: result = "u16"; break;
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

        std::string_view GenerateFunction(
            SRSLFunction* pFunction,
            const int32_t deep,
            const std::string_view& preArgs = std::string_view(),
            const std::string_view& preCode = std::string_view(),
            const std::string_view& postCode = std::string_view(),
            const std::string_view& returnType = std::string_view()
        ) {
            static std::string result;
            result.clear();

            if (!pFunction) {
                return result;
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
                result += parameter->GetName();
                result += ": ";
                result += GenerateType(parameter->GetType());
                if (i + 1 < pFunction->args.size()) {
                    result += ", ";
                }
            }

            std::string_view returnTypeFn = returnType.empty() ? GenerateType(pFunction->pType->ToString(0)) : returnType;
            if (!returnTypeFn.empty() && returnTypeFn != "void") {
                result += ") -> ";
                result += returnTypeFn;
            }
            else {
                result += ") ";
            }
            result += " {\n";

            if (!preCode.empty()) {
                result += preCode;
            }

            // for (auto&& statement : pFunction->statements) {
            //     result += GenerateLexicalTree(statement.get(), deep + 1);
            // }

            if (!postCode.empty()) {
                result += postCode;
            }

            result += GenerateTab(deep);
            result += "}\n";

            return result;
        }

        std::string_view GenerateStage(const SRSLShader* pShader, SRSLResult& result, ShaderStage stage, const std::string& preCode = std::string()) {
            static std::string code;
            code.clear();

            auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(stage);
            /*
            if (auto&& pFunctionCallStack = m_shader->GetUseStack()->FindFunction(entryPoint)) {
                for (auto&& [name, pVariable] : m_shader->GetSharedWorkgroup()) {
                    /// если переменную не передали, значит ее нет
                    if (stage != ShaderStage::Compute || !pFunctionCallStack->IsVariableUsed(name)) {
                        continue;
                    }

                    auto&& type = GenerateVariable(pVariable, 0);
                    code += SR_FORMAT("shared {};\n", type.c_str());
                }

                for (auto&& pUnit : m_shader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                    if (auto&& pStructure = dynamic_cast<SRSLStructureStatement*>(pUnit)) {
                        if (!pFunctionCallStack->IsStructUsed(pStructure->pName->token)) {
                            continue;
                        }

                        std::string structureCode = GenerateStructure(pStructure, 0);
                        if (structureCode.empty()) {
                            continue;
                        }

                        code += structureCode + "\n\n";
                    }
                }
            }

            if (auto&& uniformsCode = GenerateUniforms(stage); !uniformsCode.empty()) {
                code += uniformsCode + "\n";
            }*/

            code += preCode;

            if (auto&& pFunctionCallStack = pShader->GetUseStack()->FindFunction(entryPoint)) {
                for (auto&& pUnit : pShader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                    if (auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit)) {
                        if (!pFunctionCallStack->IsFunctionUsed(pFunction->pName->token)) {
                            continue;
                        }

                        code += GenerateFunction(pFunction, 0);
                        code += "\n\n";
                    }
                }
            }

            return code;
        }

        std::optional<std::string_view> GenerateVertexStage(const SRSLShader* pShader, SRSLResult& result) {
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
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                preCode += SR_FORMAT("{}{} = input.{};\n", GenerateTab(1), attributeName, attributeName);
            });

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("VERTEX_INDEX")) {
                preCode += GenerateTab(1);
                preCode += "u32 VERTEX_INDEX = vertexIndex;\n";
                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(vertex_index) vertexIndex : u32";
            }

            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                preCode += SR_FORMAT("{}vsOut.{} = {};\n", GenerateTab(1), attributeName, attributeName);
            });

            if (isOutPositionUsed) {
                postCode += GenerateTab(1);
                postCode += "vsOut.position = OUT_POSITION;\n";
            }
            else if (vertexLayoutDescriptions.Find(SR_UTILS_NS::VertexAttribute::Position)) {
                postCode += GenerateTab(1);
                postCode += "vsOut.position = vec4<f32>(VERTEX, 1.0);\n";
            }
            postCode += GenerateTab(1);
            postCode += "return vsOut;\n";

            resultCode += "@vertex\n";

            static const std::string vertexOutputId = "VertexOutput";
            resultCode += GenerateFunction(pStageFunction, 0, preArgs, preCode, postCode, vertexOutputId);

            return resultCode;
        }

        std::optional<std::string_view> GenerateFragmentStage(const SRSLShader* pShader, SRSLResult& result) {
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

            uint64_t outLocation = 0;
            for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
                const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
                if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                    preCode += GenerateTab(1);
                    preCode += SR_FORMAT("var {} : vec4<f32>; /// location {}\n", layer.c_str(), outLocation);
                }
                ++outLocation;
            }

            preCode += GenerateTab(1);
            preCode += "var fsOut : FragmentOutput;\n";

            if (isFragCoordUsed) {
                preCode += GenerateTab(1);
                preCode += "FRAG_COORD = gl_FragCoord;\n\n";
            }

            if (isColorUsed) {
                postCode += GenerateTab(1);
                postCode += SR_SRSL_MAIN_OUT_LAYER;
                postCode += " = COLOR;\n";
            }

            /// color buffer pass code
            if (isColorPassDefined) {
                const bool discardExists = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_color_buffer_discard") != nullptr;
                if (discardExists) {
                    postCode += GenerateTab(1);
                    postCode += "fragment_color_buffer_discard();\n";
                }
                preCode += GenerateTab(1);
                preCode += "COLOR = vec4<f32>({}, 1.0);\n"_format(SHADER_PC_COLOR_BUFFER_VALUE);
            }
            else if (isCascadedMapPassDefined) {
                const bool discardExists = pShader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_depth_buffer_discard") != nullptr;
                if (discardExists) {
                    postCode += GenerateTab(1);
                    postCode += "fragment_depth_buffer_discard();\n";
                }
            }

            outLocation = 0;
            for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
                const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
                if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                    if (pShader->IsMacroDefined(SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[outLocation])) {
                        postCode += GenerateTab(1);
                        postCode += SR_FORMAT("fsOut.{} = {};\n", layer.c_str(), layer.c_str());
                    }
                }
                ++outLocation;
            }

            postCode += GenerateTab(1);
            postCode += "return fsOut;\n";

            code += GenerateFunction(pStageFunction, 0, std::string_view(), preCode, postCode);

            return code;
        }

        std::optional<std::string_view> GenerateComputeStage(const SRSLShader* pShader, SRSLResult& result) {
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

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("GLOBAL_INVOCATION_ID")) {
                preCode += GenerateTab(1);
                preCode += "var GLOBAL_INVOCATION_ID : vec3<u32> = global_id;\n";

                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(global_invocation_id) global_id : vec3<u32>";
            }

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("WORK_GROUP_ID")) {
                preCode += GenerateTab(1);
                preCode += "var WORK_GROUP_ID : vec<u32> = workgroup_id;\n";

                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(workgroup_id) workgroup_id : vec3<u32>";
            }

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("NUM_WORK_GROUPS")) {
                preCode += GenerateTab(1);
                preCode += "var NUM_WORK_GROUPS : vec<u32> = num_workgroups;\n";

                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(num_workgroups) num_workgroups : vec3<u32>";
            }

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_ID")) {
                preCode += GenerateTab(1);
                preCode += "var LOCAL_INVOCATION_ID : vec<u32> = local_id;\n";

                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(local_invocation_id) local_id : vec3<u32>";
            }

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_INDEX")) {
                preCode += GenerateTab(1);
                preCode += "var LOCAL_INVOCATION_INDEX : u32 = local_index;\n";

                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(local_invocation_index) local_index : u32";
            }

            code = GenerateStage(pShader, result, ShaderStage::Compute, preCode);
            code += GenerateFunction(pStageFunction, 0, std::string_view(), preCode);

            return code;
        }
    }

    ISRSLCodeGenerator::SRSLCodeGenRes WGSLCodeGenerator::GenerateStages(const SRSLShader* pShader) {
        SR_GLOBAL_LOCK

        Clear();

        ISRSLCodeGenerator::SRSLCodeGenRes codeGenRes;

        auto&& [result, stages] = codeGenRes;

        if (!pShader->GetAnalyzedTree()) {
            result = SRSLResult(SRSLReturnCode::InvalidLexicalTree);
            return codeGenRes;
        }

        std::string& code = stages[ShaderStage::All];
        code.reserve(4096);

        code += "/// [WARNING: THIS FILE WAS CREATED BY SRSL CODE GENERATION]\n\n";
        if (pShader->IsGLayerUsed()) {
            code += "/// WARNING: required GLayer, but not supported in WGSL. Please, remove GLayer usage or use another shader type.\n\n";
        }
        code += "/// Shader type: " + SR_UTILS_NS::EnumReflector::ToStringAtom(pShader->GetType()).ToStringRef() + "\n\n";

        auto&& vertexLayoutDescriptions = pShader->GetCreateInfo().vertexLayoutDescriptions;
        if (vertexLayoutDescriptions.GetAttributesCount() > 0) {
            code += "struct VertexInput {\n";
            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t i) {
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "\t@location({}) {}_INPUT : {},\n"_format(i, attributeName, attributeType);
            });
            code += "};\n\n";

            code += "struct VertexOutput {\n";
            code += "\t@builtin(position) position : vec4<f32>,\n";
            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t i) {
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "\t@location({}) {} : {},\n"_format(std::max(i, 0u), attributeName, attributeType);
            });
            code += "};\n\n";

            vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                code += "var<private> {} : {};\n"_format(SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute), WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count));
            });
            code += "\n";
        }

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

        if (auto&& stageCode = WGSLDetail::GenerateVertexStage(pShader, result)) {
            code += stageCode.value();
            code += "\n";
        }

        if (auto&& stageCode = WGSLDetail::GenerateFragmentStage(pShader, result)) {
            code += stageCode.value();
            code += "\n";
        }

        if (auto&& stageCode = WGSLDetail::GenerateComputeStage(pShader, result)) {
            code += stageCode.value();
        }

        result = SR_UTILS_NS::Exchange(m_result, { });

        return codeGenRes;
    }
}
