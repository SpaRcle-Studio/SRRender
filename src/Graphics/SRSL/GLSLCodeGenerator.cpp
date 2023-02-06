//
// Created by Monika on 30.01.2023.
//

#include <Graphics/SRSL/GLSLCodeGenerator.h>
#include <Graphics/SRSL/Shader.h>

namespace SR_SRSL_NS {
    ISRSLCodeGenerator::SRSLCodeGenRes GLSLCodeGenerator::GenerateStages(const SRSLShader* pShader) {
        SR_GLOBAL_LOCK

        Clear();

        m_shader = pShader;

        ISRSLCodeGenerator::SRSLCodeGenRes codeGenRes;

        auto&& [result, stages] = codeGenRes;

        if (!pShader->GetAnalyzedTree()) {
            result = SRSLResult(SRSLReturnCode::InvalidLexicalTree);
            return codeGenRes;
        }

        if (auto&& vertexCode = GenerateVertexStage()) {
            stages[ShaderStage::Vertex] = vertexCode.value();
        }

        if (auto&& fragmentCode = GenerateFragmentStage()) {
            stages[ShaderStage::Fragment] = fragmentCode.value();
        }

        result = SR_UTILS_NS::Exchange(m_result, { });

        return codeGenRes;
    }

    std::string GLSLCodeGenerator::GenerateStage(ShaderStage stage) {
        std::string code;

        code += "/// [WARNING: THIS FILE WAS CREATED BY SRSL CODE GENERATION]\n\n";
        code += "/// Shader stage: " + SR_UTILS_NS::EnumReflector::ToString(stage) + "\n";
        code += "/// Shader type: " + SR_UTILS_NS::EnumReflector::ToString(m_shader->GetType()) + "\n\n";

        code += "#version " + GetVersion(stage) + "\n\n";

        if (auto&& vertexLocations = GenerateInputVertexLocations(); !vertexLocations.empty()) {
            code += vertexLocations + "\n";
        }

        if (auto&& vertexLocations = GenerateOutputVertexLocations(); !vertexLocations.empty()) {
            code += vertexLocations + "\n";
        }

        if (auto&& uniformsCode = GenerateUniforms(stage); !uniformsCode.empty()) {
            code += uniformsCode + "\n";
        }

        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(stage);
        if (auto&& pFunctionCallStack = m_shader->GetUseStack()->FindFunction(entryPoint)) {
            for (auto &&pUnit : m_shader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                auto&& pFunction = dynamic_cast<SRSLFunction*>(pUnit);

                if (!pFunction) {
                    continue;
                }

                if (!pFunctionCallStack->IsFunctionUsed(pFunction->pName->token)) {
                    continue;
                }

                code += GenerateFunction(pFunction, 0) + "\n\n";
            }
        }

        return code;
    }

    std::optional<std::string> GLSLCodeGenerator::GenerateVertexStage() {
        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Vertex);
        auto&& pStageFunction = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        if (!pStageFunction) {
            return std::optional<std::string>();
        }

        std::string code = GenerateStage(ShaderStage::Vertex);

        std::string preCode;

        auto&& vertexInfo = Vertices::GetVertexInfo(m_shader->GetVertexType());
        for (auto&& vertexAttribute : vertexInfo.m_names) {
            preCode += SR_UTILS_NS::Format("%s%s = %s_INPUT;\n", GenerateTab(1).c_str(), vertexAttribute.c_str(), vertexAttribute.c_str());
        }

        std::string postCode = GenerateTab(1) + "gl_Position = vec4(VERTEX, 1.0);\n";

        code += GenerateFunction(pStageFunction, 0, preCode, postCode);

        return code;
    }

    std::optional<std::string> GLSLCodeGenerator::GenerateFragmentStage() {
        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment);
        auto&& pStageFunction = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        if (!pStageFunction) {
            return std::optional<std::string>();
        }

        std::string code = GenerateStage(ShaderStage::Fragment);

        std::string preCode;
        std::string postCode;

        code += GenerateFunction(pStageFunction, 0, preCode, postCode);

        return code;
    }

    std::string GLSLCodeGenerator::GetVersion(ShaderStage stage) const {
        return "450";
    }

    std::string GLSLCodeGenerator::GenerateInputVertexLocations() const {
        std::string code;

        auto&& vertexInfo = Vertices::GetVertexInfo(m_shader->GetVertexType());

        uint32_t location = 0;
        for (auto&& vertexAttribute : vertexInfo.m_names) {
            std::string type = VertexAttributeToString(vertexInfo.m_attributes[location].first);
            code += SR_UTILS_NS::Format("layout (location = %i) in %s %s_INPUT;\n", location, type.c_str(), vertexAttribute.c_str());
            ++location;
        }

        return code;
    }

    std::string GLSLCodeGenerator::GenerateOutputVertexLocations() const {
        std::string code;

        auto&& vertexInfo = Vertices::GetVertexInfo(m_shader->GetVertexType());

        uint32_t location = 0;
        for (auto&& vertexAttribute : vertexInfo.m_names) {
            std::string type = VertexAttributeToString(vertexInfo.m_attributes[location].first);
            code += SR_UTILS_NS::Format("layout (location = %i) out %s %s;\n", location, type.c_str(), vertexAttribute.c_str());
            ++location;
        }

        if (std::find(vertexInfo.m_names.begin(), vertexInfo.m_names.end(), "VERTEX") == vertexInfo.m_names.end()) {
            code += SR_UTILS_NS::Format("layout (location = %i) out vec3 VERTEX;\n", location);
            ++location;
        }

        if (std::find(vertexInfo.m_names.begin(), vertexInfo.m_names.end(), "UV") == vertexInfo.m_names.end()) {
            code += SR_UTILS_NS::Format("layout (location = %i) out vec2 UV;\n", location);
            ++location;
        }

        SR_UNUSED_VARIABLE(location);

        return code;
    }

    std::string GLSLCodeGenerator::GenerateVariable(SRSLVariable* pVariable, int32_t deep) const {
        std::string code = GenerateTab(deep);

        if (pVariable->pType) {
            code += GenerateType(pVariable->pType, 0) + " ";
        }

        if (pVariable->pName) {
            code += GenerateName(pVariable->pName, 0);
        }

        if (pVariable->pExpr) {
            code += " = " + GenerateExpression(pVariable->pExpr, 0);
        }

        return code;
    }

    std::string GLSLCodeGenerator::GenerateFunction(SRSLFunction* pFunction, int32_t deep) const {
        return GenerateFunction(pFunction, deep, std::string(), std::string());
    }

    std::string GLSLCodeGenerator::GenerateFunction(SRSLFunction* pFunction, int32_t deep, const std::string& preCode, const std::string& postCode) const {
        std::string code;

        code += GenerateTab(deep);

        if (pFunction->pType) {
            code += GenerateType(pFunction->pType, 0) + " ";
        }

        if (pFunction->pName) {
            if (IsShaderEntryPoint(pFunction->GetName())) {
                code += "main";
            }
            else {
                code += GenerateType(pFunction->pName, 0);
            }
        }

        code += "(";

        for (uint32_t i = 0; i < pFunction-> args.size(); ++i) {
            code += GenerateVariable(pFunction->args[i], 0);

            if (i + 1 < pFunction->args.size()) {
                code += ", ";
            }
        }

        code += ") ";

        if (pFunction->pLexicalTree) {
            code += GenerateLexicalTree(pFunction->pLexicalTree, deep, preCode, postCode);
        }

        return code;
    }

    std::string GLSLCodeGenerator::GenerateType(SRSLExpr* pExpr, int32_t deep) const {
        return GenerateExpression(pExpr, deep);
    }

    std::string GLSLCodeGenerator::GenerateName(SRSLExpr* pExpr, int32_t deep) const {
        return GenerateExpression(pExpr, deep);
    }

    std::string GLSLCodeGenerator::GenerateExpression(SRSLExpr* pExpr, int32_t deep) const {
        if ((pExpr->token == "++" || pExpr->token == "--") && !pExpr->args.empty()) {
            SRHalt0();
            return std::string();
        }

        std::string code = GenerateTab(deep);

        if (pExpr->isCall) {
            code += pExpr->token + "(";

            for (uint32_t i = 0; i < pExpr->args.size(); ++i) {
                code += GenerateExpression(pExpr->args[i], 0);

                if (i + 1 < pExpr->args.size()) {
                    code += ", ";
                }
            }

            code += ")";
        }
        else if (pExpr->isArray) {
            code += GenerateExpression(pExpr->args[0], 0) + "[" + GenerateExpression(pExpr->args[1], 0) + "]";
        }
        else if (pExpr->args.empty()) {
            code += pExpr->token;
        }
        else if (pExpr->args.size() == 1) {
            code += "(" + pExpr->token + GenerateExpression(pExpr->args[0], 0) + ")";
        }
        else if (pExpr->args.size() == 2 && (pExpr->token == "=" || pExpr->token == ".")) {
            if (pExpr->token == ".") {
                code += GenerateExpression(pExpr->args[0], 0) + pExpr->token + GenerateExpression(pExpr->args[1], 0);
            }
            else {
                code += GenerateExpression(pExpr->args[0], 0) + " " + pExpr->token + " " + GenerateExpression(pExpr->args[1], 0);
            }
        }
        else if (pExpr->args.size() == 2) {
            code += "(" + GenerateExpression(pExpr->args[0], 0) + " " + pExpr->token + " " +  GenerateExpression(pExpr->args[1], 0) + ")";
        }

        return code;
    }

    std::string GLSLCodeGenerator::GenerateLexicalTree(SRSLLexicalTree *pLexicalTree, int32_t deep) const {
        return GenerateLexicalTree(pLexicalTree, deep, std::string(), std::string());
    }

    std::string GLSLCodeGenerator::GenerateLexicalTree(SRSLLexicalTree* pLexicalTree, int32_t deep, const std::string &preCode, const std::string &postCode) const {
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
                code += GenerateFunction(pFunction, deep + 1) + "\n";
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
            code += GenerateTab(deep) + "}";
        }

        return code;
    }

    std::string GLSLCodeGenerator::GenerateTab(int32_t deep) const {
        if (deep <= 0) {
            return std::string();
        }

        return std::string(deep * 4, ' ');
    }

    std::string GLSLCodeGenerator::GenerateUniforms(ShaderStage stage) const {
        std::string code;
        uint32_t binding = 0;

        auto&& pFunction = m_shader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));
        if (!pFunction) {
            return code;
        }

        /// ------------------------------------------------------------------------------------------------------------

        std::string blocksCode;

        for (auto&& [name, uniformBlock] : m_shader->GetUniformBlocks()) {
            std::string blockCode = SR_UTILS_NS::Format("layout (std140, binding = %i) uniform %s {\n", binding, name.c_str());
            bool hasUsage = false;

            for (auto&& field : uniformBlock.fields) {
                hasUsage |= pFunction->IsVariableUsed(field.name);

                auto&& typeName = SRSLTypeInfo::Instance().GetTypeName(field.type);
                auto&& dimension = SRSLTypeInfo::Instance().GetDimension(field.type, nullptr);

                std::string strDimension;

                for (auto&& dim : dimension) {
                    strDimension += "[" +  std::to_string(dim) + "]";
                }

                blockCode += SR_UTILS_NS::Format("\t// (%i bytes) %s\n", field.size, field.isPublic ? "public" : "private");
                blockCode += SR_UTILS_NS::Format("\t%s %s%s;\n", typeName.c_str(), field.name.c_str(), strDimension.c_str());
            }

            blockCode += "};\n";

            if (hasUsage) {
                blocksCode += blockCode;
            }

            ++binding;
        }

        /// ------------------------------------------------------------------------------------------------------------

        std::string samplersCode;

        for (auto&& [name, sampler] : m_shader->GetSamplers()) {
            if (pFunction->IsVariableUsed(name)) {
                samplersCode += SR_UTILS_NS::Format("layout (binding = %i) uniform %s %s; // (sampler) %s\n",
                        binding,
                        sampler.type.c_str(),
                        name.c_str(),
                        sampler.isPublic ? "public" : "private"
                );
            }

            ++binding;
        }

        /// ------------------------------------------------------------------------------------------------------------

        code += blocksCode;

        if (!samplersCode.empty() && !blocksCode.empty()) {
            code += "\n";
        }

        code += samplersCode;

        return code;
    }

    std::string GLSLCodeGenerator::VertexAttributeToString(Vertices::Attribute attribute) const {
        switch (attribute) {
            case Vertices::Attribute::FLOAT_R32G32B32A32: return "vec4";
            case Vertices::Attribute::FLOAT_R32G32B32: return "vec3";
            case Vertices::Attribute::FLOAT_R32G32: return "vec2";
            case Vertices::Attribute::INT_R32G32B32A32: return "ivec4";
            case Vertices::Attribute::INT_R32G32B32: return "ivec3";
            case Vertices::Attribute::INT_R32G32: return "ivec2";
            case Vertices::Attribute::Unknown:
            default:
                SRHalt0();
                return std::string();
        }
    }

    std::string GLSLCodeGenerator::GenerateIfStatement(SRSLIfStatement* pIfStatement, int32_t deep) const {
        std::string code;

        code += GenerateTab(deep);

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
}