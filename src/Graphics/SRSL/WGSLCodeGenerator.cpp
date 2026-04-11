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
            const std::string& preArgs = std::string(),
            const std::string& preCode = std::string(),
            const std::string& postCode = std::string(),
            const std::string& returnType = std::string()
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

            result += ") -> ";
            if (!returnType.empty()) {
                result += returnType;
            }
            else {
                result += GenerateType(pFunction->pType->ToString(0));
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
            preCode += "var output : VertexOutput;\n";
            if (isOutPositionUsed) {
                preCode += GenerateTab(1);
                preCode += "vec4<f32> OUT_POSITION;\n";
            }

            auto&& vertexLayoutDescription = pShader->GetCreateInfo().vertexLayoutDescription;
            if (vertexLayoutDescription.attributesCount > 0) {
                preArgs += "input : VertexInput";
            }

            for (uint32_t i = 0; i < vertexLayoutDescription.attributesCount; ++i) {
                auto&& vertexAttribute = vertexLayoutDescription.attributes[i];
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                preCode += SR_FORMAT("{}{} = input.{};\n", GenerateTab(1), attributeName, attributeName);
            }

            if (pShader->GetUseStack()->IsVariableUsedInEntryPoints("VERTEX_INDEX")) {
                preCode += GenerateTab(1);
                preCode += "u32 VERTEX_INDEX = vertexIndex;\n";
                if (!preArgs.empty()) {
                    preArgs += ", ";
                }
                preArgs += "@builtin(vertex_index) vertexIndex : u32";
            }

            for (uint32_t i = 0; i < vertexLayoutDescription.attributesCount; ++i) {
                auto&& vertexAttribute = vertexLayoutDescription.attributes[i];
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                preCode += SR_FORMAT("{}output.{} = {};\n", GenerateTab(1), attributeName, attributeName);
            }

            if (isOutPositionUsed) {
                postCode += GenerateTab(1);
                postCode += "output.position = OUT_POSITION;\n";
            }
            else if (vertexLayoutDescription.Find(SR_UTILS_NS::VertexAttribute::Position)) {
                postCode += GenerateTab(1);
                postCode += "output.position = vec4<f32>(VERTEX, 1.0);\n";
            }
            postCode += GenerateTab(1);
            postCode += "return output;\n";

            resultCode += "@vertex\n";

            static const std::string vertexOutputId = "VertexOutput";
            resultCode += GenerateFunction(pStageFunction, 0, preArgs, preCode, postCode, vertexOutputId);

            return resultCode;
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

        if (pShader->GetCreateInfo().vertexLayoutDescription.attributesCount > 0) {
            code += "struct VertexInput {\n";
            for (int32_t i = 0; i < pShader->GetCreateInfo().vertexLayoutDescription.attributesCount; ++i) {
                auto&& vertexAttribute = pShader->GetCreateInfo().vertexLayoutDescription.attributes[i];
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "\t@location({}) {}_INPUT : {},\n"_format(i, attributeName, attributeType);
            }
            code += "};\n\n";

            code += "struct VertexOutput {\n";
            code += "\t@builtin(position) position : vec4<f32>,\n";
            for (int32_t i = 0; i < pShader->GetCreateInfo().vertexLayoutDescription.attributesCount; ++i) {
                auto&& vertexAttribute = pShader->GetCreateInfo().vertexLayoutDescription.attributes[i];
                std::string_view attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                std::string_view attributeType = WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count);
                code += "\t@location({}) {} : {},\n"_format(std::max(i, 0), attributeName, attributeType);
            }
            code += "};\n\n";

            for (int32_t i = 0; i < pShader->GetCreateInfo().vertexLayoutDescription.attributesCount; ++i) {
                auto&& vertexAttribute = pShader->GetCreateInfo().vertexLayoutDescription.attributes[i];
                code += "var<private> {} : {};\n"_format(SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute), WGSLDetail::VertexAttributeFormatToString(vertexAttribute.format, vertexAttribute.count));
            }
            code += "\n";
        }

        if (auto&& stageCode = WGSLDetail::GenerateVertexStage(pShader, result)) {
            code += stageCode.value();
        }

        /*if (auto&& code = GenerateFragmentStage()) {
            stages[ShaderStage::Fragment] = code.value();
        }

        if (auto&& code = GenerateComputeStage()) {
            stages[ShaderStage::Compute] = code.value();
        }
        */

        result = SR_UTILS_NS::Exchange(m_result, { });

        return codeGenRes;
    }
}
