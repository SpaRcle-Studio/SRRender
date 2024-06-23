//
// Created by Monika on 22.01.2023.
//

#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/Lexer.h>
#include <Graphics/SRSL/PseudoCodeGenerator.h>
#include <Graphics/SRSL/GLSLCodeGenerator.h>
#include <Graphics/SRSL/AssignExpander.h>
#include <Graphics/SRSL/PreProcessor.h>
#include <Graphics/SRSL/TypeInfo.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <Utils/Platform/Platform.h>

namespace SR_SRSL_NS {
    void SRSLUniformBlock::Align(const SRSLAnalyzedTree::Ptr& pAnalyzedTree) {
        for (auto&& field : fields) {
            field.size = SRSLTypeInfo::Instance().GetTypeSize(field.type, pAnalyzedTree);
            field.alignedSize = SRSLTypeInfo::Instance().GetAlignedTypeSize(field.type, pAnalyzedTree);
            size += field.alignedSize;
        }

        std::sort(fields.begin(), fields.end(), [](const SRSLUniformBlock::Field& a, const SRSLUniformBlock::Field& b) -> bool {
            return a.size > b.size;
        });
    }

    SRSLShader::SRSLShader(SR_UTILS_NS::Path path)
        : Super()
        , m_path(std::move(path))
    { }

    SRSLShader::Ptr SRSLShader::Load(SR_UTILS_NS::Path path) {
        auto&& absPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);

        if (!absPath.Exists()) {
            SR_ERROR("SRSLShader::Load() : file not exists!\n\tPath: " + path.ToString());
            return nullptr;
        }

        auto&& pShader = SRSLShader::Ptr(new SRSLShader(path));

        auto&& lexems = SR_SRSL_NS::SRSLLexer::Instance().Parse(absPath, 0);
        if (lexems.empty()) {
            SR_ERROR("SRSLShader::Load() : failed to parse lexems!\n\tPath: " + path.ToString());
            return nullptr;
        }

        SRSLPreProcessor::Includes includes = { path.ToStringRef() };

        auto&& [preProcessedLexems, preProcessResult] = SRSLPreProcessor::Instance().Process(std::move(lexems), includes);
        if (preProcessResult.HasErrors()) {
            SR_ERROR("SRSLShader::Load() : failed to pre-process shader!" + preProcessResult.ToString(includes));
            return nullptr;
        }

        lexems = std::move(preProcessedLexems);

        auto&& [expandedLexems, expandResult] = SR_SRSL_NS::SRSLAssignExpander::Instance().Expand(std::move(lexems));
        if (expandResult.HasErrors()) {
            SR_ERROR("SRSLShader::Load() : failed to expand assign shader!" + expandResult.ToString(includes));
            return nullptr;
        }

        lexems = std::move(expandedLexems);

        auto&& [pAnalyzedTree, analyzeResult] = SR_SRSL_NS::SRSLLexicalAnalyzer::Instance().Analyze(std::move(lexems));

        if (!pAnalyzedTree || analyzeResult.HasErrors()) {
            SR_ERROR("SRSLShader::Load() : failed to analyze shader!" + analyzeResult.ToString(includes));
            return nullptr;
        }

        pShader->m_analyzedTree = std::move(pAnalyzedTree);
        pShader->m_includes = std::move(includes);

        if (!pShader->Prepare()) {
            SR_ERROR("SRSLShader::Load() : failed to prepare shader!\n\tPath: " + path.ToString());
            return nullptr;
        }

        if (!pShader->SaveCache()) {
            SR_WARN("SRSLShader::Load() : failed to save shader cache shader!\n\tPath: " + path.ToString());
        }

        return pShader;
    }

    bool SRSLShader::IsCacheActual() const {
        auto&& cachedPath = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(m_path);
        return GetHash() == SR_UTILS_NS::FileSystem::ReadHashFromFile(cachedPath.ConcatExt("hash"));
    }

    bool SRSLShader::IsCacheActual(ShaderLanguage shaderLanguage) const {
        auto&& cachedPath = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(m_path);
        auto&& cachedHash = SR_UTILS_NS::FileSystem::ReadHashFromFile(cachedPath.ConcatExt("hash").ConcatExt(
                SR_UTILS_NS::EnumReflector::ToStringAtom(shaderLanguage)));
        return GetHash() == cachedHash;
    }

    uint64_t SRSLShader::GetHash() const {
        uint64_t hash = 0;

        for (auto&& include : m_includes) {
            auto&& absPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(include);
            hash = SR_UTILS_NS::CombineTwoHashes(hash, absPath.GetFileHash());
        }

        return hash;
    }

    bool SRSLShader::SaveCache() const {
        auto&& cachedPath = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(m_path);
        SR_UTILS_NS::FileSystem::WriteHashToFile(cachedPath.ConcatExt("hash"), GetHash());
        return true;
    }

    std::string SRSLShader::ToString(ShaderLanguage shaderLanguage) const {
        auto&& [result, stages] = GenerateStages(shaderLanguage);

        if (result.HasErrors()) {
            return "SRSLShader::ToStringAtom() : " + result.ToString(m_includes);
        }

        std::string code;

        for (auto&& [stage, stageCode] : stages) {
            code += "Stage[" + SR_UTILS_NS::EnumReflector::ToStringAtom(stage).ToStringRef() + "] {\n" + stageCode + "\n}";
        }

        return code;
    }

    const SRSLAnalyzedTree::Ptr SRSLShader::GetAnalyzedTree() const {
        return m_analyzedTree;
    }

    const SRSLUseStack::Ptr SRSLShader::GetUseStack() const {
        return m_useStack;
    }

    bool SRSLShader::Prepare() {
        m_useStack = SRSLRefAnalyzer::Instance().Analyze(m_analyzedTree);
        if (!m_useStack) {
            SR_ERROR("SRSLShader::Prepare() : failed to analyze shader refs!");
            return false;
        }

        if (!PrepareSettings()) {
            SR_ERROR("SRSLShader::Prepare() : failed to prepare shader settings!");
            return false;
        }

        if (!PrepareUniformBlocks()) {
            SR_ERROR("SRSLShader::Prepare() : failed to prepare shader uniform blocks!");
            return false;
        }

        if (!PrepareSamplers()) {
            SR_ERROR("SRSLShader::Prepare() : failed to prepare shader samplers!");
            return false;
        }

        if (!PrepareStages()) {
            SR_ERROR("SRSLShader::Prepare() : failed to prepare shader stages!");
            return false;
        }

        return true;
    }

    SR_SRSL_NS::ShaderType SRSLShader::GetType() const {
        return m_type;
    }

    Vertices::VertexType SRSLShader::GetVertexType() const {
        switch (GetType()) {
            case ShaderType::Spatial:
            case ShaderType::SpatialCustom:
                return Vertices::VertexType::StaticMeshVertex;
            case ShaderType::Skinned:
                return Vertices::VertexType::SkinnedMeshVertex;
            case ShaderType::PostProcessing:
                return Vertices::VertexType::None;
            case ShaderType::Skybox:
            case ShaderType::Simple:
            case ShaderType::Line:
                return Vertices::VertexType::SimpleVertex;
            case ShaderType::Text:
            case ShaderType::TextUI:
            case ShaderType::Canvas:
                return Vertices::VertexType::None;
            case ShaderType::Custom:
            case ShaderType::Particles:
            case ShaderType::Compute:
            case ShaderType::Unknown:
            default:
                SRHalt0();
                return Vertices::VertexType::Unknown;
        }
    }

    bool SRSLShader::PrepareSettings() {
        for (auto&& pUnit : m_analyzedTree->pLexicalTree->lexicalTree) {
            if (auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit)) {
                std::string& varName = pVariable->pType->token;
                std::string& varValue = pVariable->pName->token;

                if (varName == "ShaderType") {
                    m_type = SR_UTILS_NS::EnumReflector::FromString<SR_SRSL_NS::ShaderType>(varValue);
                }
                else if (varName == "PolygonMode") {
                    m_createInfo.polygonMode = SR_UTILS_NS::EnumReflector::FromString<PolygonMode>(varValue);
                }
                else if (varName == "CullMode") {
                    m_createInfo.cullMode = SR_UTILS_NS::EnumReflector::FromString<CullMode>(varValue);
                }
                else if (varName == "DepthCompare") {
                    m_createInfo.depthCompare = SR_UTILS_NS::EnumReflector::FromString<DepthCompare>(varValue);
                }
                else if (varName == "PrimitiveTopology") {
                    m_createInfo.primitiveTopology = SR_UTILS_NS::EnumReflector::FromString<PrimitiveTopology>(varValue);
                }
                else if (varName == "BlendEnabled") {
                    m_createInfo.blendEnabled = SR_UTILS_NS::LexicalCast<bool>(varValue);
                }
                else if (varName == "DepthWrite") {
                    m_createInfo.depthWrite = SR_UTILS_NS::LexicalCast<bool>(varValue);
                }
                else if (varName == "DepthTest") {
                    m_createInfo.depthTest = SR_UTILS_NS::LexicalCast<bool>(varValue);
                }
            }
        }

        if (GetType() == ShaderType::Unknown) {
            SR_ERROR("SRSLShader::PrepareSettings() : shader type is not set!");
            return false;
        }

        return true;
    }

    bool SRSLShader::PrepareUniformBlocks() {
        for (auto&& pUnit : m_analyzedTree->pLexicalTree->lexicalTree) {
            auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit);
            if (!pVariable || !pVariable->pDecorators) {
                continue;
            }

            if (SR_SRSL_NS::IsSampler(pVariable->GetType())) {
                continue;
            }

            /// не добавляем в блок переменные, которые объявили и не используем
            if (!m_useStack->IsVariableUsedInEntryPoints(pVariable->GetName())) {
                continue;
            }

            if (auto&& pDecorator = pVariable->pDecorators->Find("ssbo")) {
                SRSLUniformBlock::Field field;

                field.name = pVariable->pName->ToString(0);
                field.type = pVariable->pType->ToString(0);

                if (pDecorator->args.empty()) {
                    SR_ERROR("SRSLShader::PrepareUniformBlocks() : ssbo block name is not set!");
                    continue;
                }
                std::string blockName = pDecorator->args[0]->token;

                auto&& usedStages = m_useStack->IsVariableUsedInEntryPointsExt(field.name);

                auto&& uniformBlock = m_ssboBlocks[blockName];
                uniformBlock.fields.emplace_back(field);
                uniformBlock.stages.insert(usedStages.begin(), usedStages.end());
            }
            else if ((pDecorator = pVariable->pDecorators->Find("shared"))) {
                m_shared[pVariable->pName->ToString(0)] = pVariable;
            }
            else if ((pDecorator = pVariable->pDecorators->Find("uniform"))) {
                SRSLUniformBlock::Field field;

                field.name = pVariable->pName->ToString(0);
                field.type = pVariable->pType->ToString(0);
                field.isPublic = bool(pVariable->pDecorators->Find("public"));
                field.defaultValue = EvalExpressionValue(pVariable->pExpr);

                std::string blockName;

                if (pDecorator->args.empty()) {
                    if (SR_SRSL_DEFAULT_SHARED_UNIFORMS.count(field.name) == 1) {
                        blockName = "SHARED";
                    }
                    else {
                        blockName = "BLOCK";
                    }
                }
                else {
                    blockName = pDecorator->args[0]->token;
                }

                auto&& usedStages = m_useStack->IsVariableUsedInEntryPointsExt(field.name);

                if (pVariable->pDecorators->Find("const")) {
                    m_pushConstants.fields.emplace_back(field);
                    m_pushConstants.stages.insert(usedStages.begin(), usedStages.end());
                }
                else {
                    auto&& uniformBlock = m_uniformBlocks[blockName];
                    uniformBlock.fields.emplace_back(field);
                    uniformBlock.stages.insert(usedStages.begin(), usedStages.end());
                }
            }
            else if ((pDecorator = pVariable->pDecorators->Find("const"))) {
                m_constants[pVariable->pName->ToString(0)] = pVariable;
            }
        }

        /// ------------------------------------------------------------------

        for (auto&& [defaultUniform, type] : SR_SRSL_DEFAULT_UNIFORMS) {
            auto&& usedStages = m_useStack->IsVariableUsedInEntryPointsExt(defaultUniform);
            if (!usedStages.empty()) {
                SRSLUniformBlock::Field field;

                field.name = defaultUniform;
                field.type = type;
                field.isPublic = false;

                auto&& uniformBlock = m_uniformBlocks["BLOCK"];
                uniformBlock.fields.emplace_back(field);
                uniformBlock.stages.insert(usedStages.begin(), usedStages.end());
            }
        }

        for (auto&& [defaultUniform, type] : SR_SRSL_DEFAULT_SHARED_UNIFORMS) {
            auto&& usedStages = m_useStack->IsVariableUsedInEntryPointsExt(defaultUniform);
            if (!usedStages.empty()) {
                SRSLUniformBlock::Field field;

                field.name = defaultUniform;
                field.type = type;
                field.isPublic = false;

                auto&& uniformBlock = m_uniformBlocks["SHARED"];
                uniformBlock.fields.emplace_back(field);
                uniformBlock.stages.insert(usedStages.begin(), usedStages.end());
            }
        }

        for (auto&& [defaultPushConstant, type] : SR_SRSL_DEFAULT_PUSH_CONSTANTS) {
            auto&& usedStages = m_useStack->IsVariableUsedInEntryPointsExt(defaultPushConstant);
            if (!usedStages.empty()) {
                SRSLUniformBlock::Field field;

                field.name = defaultPushConstant;
                field.type = type;
                field.isPublic = false;

                m_pushConstants.fields.emplace_back(field);
                m_pushConstants.stages.insert(usedStages.begin(), usedStages.end());
            }
        }

        /// ------------------------------------------------------------------

        for (auto&& [name, block] : m_uniformBlocks) {
            block.Align(m_analyzedTree);
        }

        for (auto&& [name, block] : m_ssboBlocks) {
            block.Align(m_analyzedTree);
        }

        m_pushConstants.Align(m_analyzedTree);

        /// ------------------------------------------------------------------

        return true;
    }

    bool SRSLShader::PrepareSamplers() {
        for (auto&& pUnit : m_analyzedTree->pLexicalTree->lexicalTree) {
            auto&& pVariable = dynamic_cast<SRSLVariable*>(pUnit);
            if (!pVariable || !pVariable->pDecorators) {
                continue;
            }

            if (!SR_SRSL_NS::IsSampler(pVariable->GetType())) {
                continue;
            }

            auto&& stages = m_useStack->IsVariableUsedInEntryPointsExt(pVariable->GetName());
            if (stages.empty()) {
                continue;
            }

            if (auto&& pDecorator = pVariable->pDecorators->Find("uniform")) {
                SRSLSampler sampler;

                sampler.type = pVariable->GetType();
                sampler.isPublic = bool(pVariable->pDecorators->Find("public"));
                sampler.stages = std::move(stages);

                if (auto&& pAttachment = pVariable->pDecorators->Find("attachment"); pAttachment && pAttachment->args.size() == 1) {
                    sampler.attachment = SR_UTILS_NS::LexicalCast<int32_t>(pAttachment->args.front()->token);
                }

                m_samplers[pVariable->GetName()] = sampler;
            }
        }

        for (auto&& [defaultSampler, type] : SR_SRSL_DEFAULT_SAMPLERS) {
            auto&& stages = m_useStack->IsVariableUsedInEntryPointsExt(defaultSampler);
            if (stages.empty()) {
                continue;
            }

            SRSLSampler sampler;

            sampler.type = type;
            sampler.isPublic = false;
            sampler.stages = stages;

            m_samplers[defaultSampler] = sampler;
        }

        return true;
    }

    bool SRSLShader::Export(ShaderLanguage shaderLanguage) const {
        if (IsCacheActual(shaderLanguage)) {
            return true;
        }

        auto&& [result, stages] = GenerateStages(shaderLanguage);

        if (result.HasErrors()) {
            SR_ERROR("SRSLShader::Export() : " + result.ToString(m_includes));
            return false;
        }

        for (auto&& [stage, code] : stages) {
            if (m_createInfo.stages.count(stage) == 0) {
                SRHalt("Unknown stage!");
                return false;
            }

            auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(m_createInfo.stages.at(stage).path);

            if (!path.Create() || !SR_UTILS_NS::FileSystem::WriteToFile(path, code)) {
                SR_ERROR("SRSLShader::Export() : failed to write file!\n\tPath: " + path.ToString());
                return false;
            }
        }

        auto&& absPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(m_path);
        auto&& cachedPath = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(m_path);

        SR_UTILS_NS::FileSystem::WriteHashToFile(
                cachedPath.ConcatExt("hash").ConcatExt(SR_UTILS_NS::EnumReflector::ToStringAtom(shaderLanguage)),
                absPath.GetFileHash()
        );

        return true;
    }

    bool SRSLShader::PrepareStages() {
        /// расставляем биндинги всем данным в шейдере
        {
            uint64_t binding = 0;

            for (auto&& [name, block] : m_uniformBlocks) {
                block.binding = binding;
                ++binding;
            }

            for (auto&& [name, block] : m_ssboBlocks) {
                block.binding = binding;
                ++binding;
            }

            for (auto&& [samplerName, sampler] : m_samplers) {
                sampler.binding = binding;
                ++binding;
            }
        }

        for (auto&& [stage, entryPoint] : SR_SRSL_ENTRY_POINTS) {
            auto&& pFunction = m_analyzedTree->pLexicalTree->FindFunction(entryPoint);
            if (!pFunction) {
                continue;
            }

            m_createInfo.stages[stage].path = m_path.ToString() + "/shader." + SR_SRSL_STAGE_EXTENSIONS.at(stage);

            /// блоки юниформ

            for (auto&& [name, block] : m_uniformBlocks) {
                if (block.stages.count(stage) == 0) {
                    continue;
                }

                Uniform uniform = { };
                uniform.binding = block.binding;
                uniform.size = block.size;
                uniform.stage = stage;
                uniform.type = LayoutBinding::Uniform;

                m_createInfo.uniforms.emplace_back(uniform);
            }

            for (auto&& [name, block] : m_ssboBlocks) {
                if (block.stages.count(stage) == 0) {
                    continue;
                }

                Uniform uniform = { };
                uniform.binding = block.binding;
                uniform.size = block.size;
                uniform.stage = stage;
                uniform.type = LayoutBinding::SSBO;

                m_createInfo.uniforms.emplace_back(uniform);
            }

            /// текстуры/аттачменты

            for (auto&& [name, sampler] : m_samplers) {
                if (sampler.stages.count(stage) == 0) {
                    continue;
                }

                Uniform uniform = { };
                uniform.binding = sampler.binding;
                uniform.size = 0;
                uniform.stage = stage;

                if (sampler.attachment >= 0) {
                    uniform.type = LayoutBinding::Attachhment;
                }
                else {
                    uniform.type = LayoutBinding::Sampler2D;
                }

                m_createInfo.uniforms.emplace_back(uniform);
            }

            /// push-constant'ы

            if (m_pushConstants.stages.count(stage) == 1) {
                SRShaderPushConstant pushConstant;
                pushConstant.offset = 0;
                /// Выравнивание по 16 байт. Работает для Vulkan, для остальных неизвестно. Может отличаться в зависимости от устройства
                pushConstant.size = SR_MAX(16, m_pushConstants.size);
                m_createInfo.stages[stage].pushConstants.emplace_back(pushConstant);
            }
        }

        auto&& vertexInfo = Vertices::GetVertexInfo(GetVertexType());
        m_createInfo.vertexAttributes = vertexInfo.m_attributes;
        m_createInfo.vertexDescriptions = vertexInfo.m_descriptions;

        return true;
    }

    float_t SRSLShader::EvalExpressionFloat(SRSLExpr* pExpression) const {
        if (!pExpression->args.empty()) {
            SR_ERROR("SRSLShader::EvalExpressionFloat() : invalid expression args count! Count: " + std::to_string(pExpression->args.size()));
            return 0.0f;
        }
        return SR_UTILS_NS::LexicalCast<float_t>(pExpression->token);
    }

    SR_MATH_NS::FVector3 SRSLShader::EvalExpressionVec3(SRSLExpr* pExpression) const {
        if (pExpression->args.size() == 3) {
            const float_t x = EvalExpressionFloat(pExpression->args[0]);
            const float_t y = EvalExpressionFloat(pExpression->args[1]);
            const float_t z = EvalExpressionFloat(pExpression->args[2]);
            return SR_MATH_NS::FVector3(x, y, z);
        }

        if (pExpression->args.size() == 1) {
            const float_t x = EvalExpressionFloat(pExpression->args[0]);
            return SR_MATH_NS::FVector3(x, x, x);
        }

        SR_ERROR("SRSLShader::EvalExpressionVec3() : invalid expression args count! Count: " + std::to_string(pExpression->args.size()));
        return SR_MATH_NS::FVector3::Zero();
    }

    SR_MATH_NS::FVector4 SRSLShader::EvalExpressionVec4(SRSLExpr* pExpression) const {
        if (pExpression->args.size() == 4) {
            const float_t x = EvalExpressionFloat(pExpression->args[0]);
            const float_t y = EvalExpressionFloat(pExpression->args[1]);
            const float_t z = EvalExpressionFloat(pExpression->args[2]);
            const float_t w = EvalExpressionFloat(pExpression->args[3]);
            return SR_MATH_NS::FVector4(x, y, z, w);
        }

        if (pExpression->args.size() == 1) {
            const float_t x = EvalExpressionFloat(pExpression->args[0]);
            return SR_MATH_NS::FVector4(x, x, x, x);
        }

        SR_ERROR("SRSLShader::EvalExpressionVec4() : invalid expression args count! Count: " + std::to_string(pExpression->args.size()));
        return SR_MATH_NS::FVector4();
    }

    std::optional<ShaderPropertyVariant> SRSLShader::EvalExpressionValue(SRSLExpr* pExpression) const {
        if (!pExpression) {
            return std::nullopt;
        }

        if (pExpression->token == "vec3") {
            return EvalExpressionVec3(pExpression);
        }

        if (pExpression->token == "vec4") {
            return EvalExpressionVec4(pExpression);
        }

        SR_ERROR("SRSLShader::EvalExpressionValue() : unknown expression token! Type: " + pExpression->token);

        return std::nullopt;
    }

    ISRSLCodeGenerator::SRSLCodeGenRes SRSLShader::GenerateStages(ShaderLanguage shaderLanguage) const {
        ISRSLCodeGenerator::SRSLCodeGenRes codeGenRes;

        switch (shaderLanguage) {
            case ShaderLanguage::PseudoCode:
                codeGenRes = SRSLPseudoCodeGenerator::Instance().GenerateStages(this);
                break;
            case ShaderLanguage::GLSL:
                codeGenRes = GLSLCodeGenerator::Instance().GenerateStages(this);
                break;
            case ShaderLanguage::HLSL:
            case ShaderLanguage::Metal:
            default:
                SR_ERROR("SRSLShader::ToStringAtom() : unknown shader language! Language: " + SR_UTILS_NS::EnumReflector::ToStringAtom(shaderLanguage).ToStringRef());
                codeGenRes.first = SRSLReturnCode::UnknownShaderLanguage;
                return codeGenRes;
        }

        return codeGenRes;
    }

    const SRSLUniformBlock::Field* SRSLShader::FindField(const SR_UTILS_NS::StringAtom& name) const {
        for (auto&& [blockName, block] : m_uniformBlocks) {
            for (auto&& field : block.fields) {
                if (field.name == name) {
                    return &field;
                }
            }
        }

        SR_WARN("SRSLShader::FindField() : field \"" + name.ToStringRef() + "\" not found!");

        return nullptr;
    }

    const SRSLUniformBlock *SRSLShader::FindUniformBlock(const SR_UTILS_NS::StringAtom& name) const {
        for (auto&& [blockName, block] : m_uniformBlocks) {
            if (name == blockName) {
                return &block;
            }
        }

        return nullptr;
    }

    void SRSLShader::ClearShadersCache() {
        auto&& cachedPath = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders");
        if (cachedPath.Exists(SR_UTILS_NS::Path::Type::Folder)) {
            SR_PLATFORM_NS::Delete(cachedPath);
            SR_LOG("SRSLShader::ClearShadersCache() : cache was cleared.");
        }
        else {
            SR_WARN("SRSLShader::ClearShadersCache() : cache is already clean!");
        }
    }
}