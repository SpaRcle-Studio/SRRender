//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/GLSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Enum/VertexAttribute.hpp>
#include <Enum/VertexAttributeFormat.hpp>

namespace SR_SRSL_NS {
    static constexpr std::array BuiltInGLSLAttributes = {
        SR_UTILS_NS::VertexAttributeDescription { SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3, 0 },
        SR_UTILS_NS::VertexAttributeDescription { SR_UTILS_NS::VertexAttribute::Normal, SR_UTILS_NS::VertexAttributeFormat::Float32, 3, 0 },
        SR_UTILS_NS::VertexAttributeDescription { SR_UTILS_NS::VertexAttribute::UV0, SR_UTILS_NS::VertexAttributeFormat::Float32, 2, 0 },
        SR_UTILS_NS::VertexAttributeDescription { SR_UTILS_NS::VertexAttribute::Tangent, SR_UTILS_NS::VertexAttributeFormat::Float32, 4, 0 },
    };

    SR_UTILS_NS::StringView ToGLSLType(SR_UTILS_NS::VertexAttributeFormat format, uint8_t count) {
        SR_TRACY_ZONE;
        if (count == 0 || count > 4) { SRHalt0(); return "unknown"; }
        static constexpr SR_UTILS_NS::StringView types[4][4] = {
            { "float", "vec2", "vec3", "vec4" },
            { "int", "ivec2", "ivec3", "ivec4" },
            { "uint", "uvec2", "uvec3", "uvec4" },
            { "float", "vec2", "vec3", "vec4" }
        };
        return types[static_cast<uint8_t>(CodeGenDetail::GetVertexScalarKind(format))][count - 1];
    }
    SR_UTILS_NS::String GLSLGenerator::GenerateStage(ShaderStage stage, const SR_UTILS_NS::String& preCode) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(4096);

        code += "/// [WARNING: THIS FILE WAS CREATED BY SRSL CODE GENERATION]\n\n";
        CodeGenDetail::Append(code, "/// Shader stage: ", SR_UTILS_NS::EnumReflector::ToStringAtom(stage).ToStringRef(), "\n");
        CodeGenDetail::Append(code, "/// Shader type: ", SR_UTILS_NS::EnumReflector::ToStringAtom(m_shader->GetType()).ToStringRef(), "\n");
        code += "/// Shader macros:\n";
        m_shader->GetMacros().ForEach([&](auto&& key, auto&& value) {
            SR_UTILS_NS::FormatTo(code, "///\t\t{} = {}\n", key, value);
        });
        code += "\n";

        CodeGenDetail::Append(code, "#version ", GetVersion(stage), "\n\n");

        /// code += "#extension GL_ARB_separate_shader_objects : enable\n";
        /// code += "#extension GL_EXT_shader_atomic_float : enable\n\n";
        /// code += "#extension GL_ARB_shader_image_load_store : enable\n\n";

        if (m_shader->IsGLayerUsed()) {
            code += "#extension GL_ARB_shader_viewport_layer_array : enable\n\n";
            code += "#extension GL_NV_viewport_array2 : enable\n\n";
        }

        if (auto&& vertexLocations = GenerateInputLocations(stage); !vertexLocations.empty()) {
            CodeGenDetail::Append(code, vertexLocations, "\n");
        }

        if (auto&& vertexLocations = GenerateOutputLocations(stage); !vertexLocations.empty()) {
            CodeGenDetail::Append(code, vertexLocations, "\n");
        }

        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(stage);
        if (auto&& pFunctionCallStack = m_shader->GetUseStack()->FindFunction(entryPoint)) {
            for (auto&& [name, pVariable] : m_shader->GetSharedWorkgroup()) {
                /// если переменную не передали, значит ее нет
                if (stage != ShaderStage::Compute || !pFunctionCallStack->IsVariableUsed(name)) {
                    continue;
                }

                auto&& type = GenerateVariable(pVariable, 0);
                SR_UTILS_NS::FormatTo(code, "shared {};\n", type);
            }

            for (auto&& pUnit : m_shader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                if (auto&& pStructure = dynamic_cast<const SRSLStructureStatement*>(pUnit)) {
                    if (!pFunctionCallStack->IsStructUsed(pStructure->pName->token)) {
                        continue;
                    }

                    SR_UTILS_NS::String structureCode = GenerateStructure(pStructure, 0);
                    if (structureCode.empty()) {
                        continue;
                    }

                    CodeGenDetail::Append(code, structureCode, "\n\n");
                }
            }
        }

        if (auto&& uniformsCode = GenerateUniforms(stage); !uniformsCode.empty()) {
            CodeGenDetail::Append(code, uniformsCode, "\n");
        }

        code += preCode;

        if (auto&& pFunctionCallStack = m_shader->GetUseStack()->FindFunction(entryPoint)) {
            for (auto&& pUnit : m_shader->GetAnalyzedTree()->pLexicalTree->lexicalTree) {
                if (auto&& pFunction = dynamic_cast<const SRSLFunction*>(pUnit)) {
                    if (!pFunctionCallStack->IsFunctionUsed(pFunction->pName->token)) {
                        continue;
                    }

                    AppendFunction(code, pFunction, 0); code += "\n\n";
                }
            }
        }

        return code;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateVertexStage() {
        SR_TRACY_ZONE;
        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Vertex);
        auto&& pStageFunction = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = m_shader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction) {
            return { };
        }

        SR_UTILS_NS::String code = GenerateStage(ShaderStage::Vertex);

        const bool isOutPositionUsed = pUseStackFunction->IsVariableUsed("OUT_POSITION");

        SR_UTILS_NS::String preCode;

        if (isOutPositionUsed) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "vec4 OUT_POSITION;\n");
        }

        auto&& vertexLayoutDescriptions = m_shader->GetCreateInfo().vertexLayoutDescriptions;

        vertexLayoutDescriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexLayoutDescription, uint32_t) {
            SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexLayoutDescription.attribute);
            SR_UTILS_NS::FormatTo(preCode, "{}{} = {}_INPUT;\n", GenerateTab(1).c_str(), attributeName, attributeName);
        });

        if (m_shader->GetUseStack()->IsVariableUsedInEntryPoints("VERTEX_INDEX")) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "int VERTEX_INDEX = gl_VertexIndex;\n");
        }

        SR_UTILS_NS::String postCode;

        if (isOutPositionUsed) {
            CodeGenDetail::Append(postCode, GenerateTab(1), "gl_Position = OUT_POSITION;\n");
        }
        else if (vertexLayoutDescriptions.Find(SR_UTILS_NS::VertexAttribute::Position)) {
            CodeGenDetail::Append(postCode, GenerateTab(1), "gl_Position = vec4(VERTEX, 1.0);\n");
        }

        AppendFunction(code, pStageFunction, 0, preCode, postCode);

        return code;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateFragmentStage() {
        SR_TRACY_ZONE;
        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Fragment);
        auto&& pStageFunction = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = m_shader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction || !pUseStackFunction) {
            return { };
        }

        const bool isColorPassDefined = m_shader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_COLOR_PASS);
        const bool isCascadedMapPassDefined = m_shader->IsMacroDefined(SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS);
        const bool isColorUsed = isColorPassDefined || pUseStackFunction->IsVariableUsed("COLOR");
        const bool isFragCoordUsed = pUseStackFunction->IsVariableUsed("FRAG_COORD");

        SR_UTILS_NS::String variablesCode;

        if (isColorUsed) {
            variablesCode += "vec4 COLOR;\n\n";
        }

        if (isFragCoordUsed) {
            variablesCode += "vec4 FRAG_COORD;\n\n";
        }

        SR_UTILS_NS::String code = GenerateStage(ShaderStage::Fragment, variablesCode);

        SR_UTILS_NS::String locationsCode;

        uint64_t outLocation = 0;
        for (auto&& layer : SR_SRSL_DEFAULT_OUT_LAYERS) {
            const bool isNeedMain = layer == SR_SRSL_MAIN_OUT_LAYER && isColorUsed;
            if (isNeedMain || pUseStackFunction->IsVariableUsed(layer)) {
                if (m_shader->IsMacroDefined(SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO[outLocation])) {
                    SR_UTILS_NS::FormatTo(locationsCode, "layout (location = {}) out vec4 {};\n", outLocation, layer);
                }
                else {
                    SR_UTILS_NS::FormatTo(locationsCode, "vec4 {}; /// emulate location {}\n", layer, outLocation);
                }
            }
            ++outLocation;
        }

        if (!locationsCode.empty()) {
            code += locationsCode;
            code += "\n";
        }

        SR_UTILS_NS::String preCode;

        if (isFragCoordUsed) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "FRAG_COORD = gl_FragCoord;\n\n");
        }

        SR_UTILS_NS::String postCode;

        if (isColorUsed) {
            SR_UTILS_NS::FormatTo(postCode, "{}{} = COLOR;\n", GenerateTab(1), SR_SRSL_MAIN_OUT_LAYER);
        }

        /// color buffer pass code
        if (isColorPassDefined) {
            const bool discardExists = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_color_buffer_discard") != nullptr;
            if (discardExists) {
                CodeGenDetail::Append(postCode, GenerateTab(1), "fragment_color_buffer_discard();\n");
            }
            preCode += GenerateTab(1);
            SR_UTILS_NS::FormatTo(preCode, "COLOR = vec4({}, 1.0);\n", SHADER_PC_COLOR_BUFFER_VALUE);
        }
        else if (isCascadedMapPassDefined) {
            const bool discardExists = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction("fragment_depth_buffer_discard") != nullptr;
            if (discardExists) {
                CodeGenDetail::Append(postCode, GenerateTab(1), "fragment_depth_buffer_discard();\n");
            }
        }

        AppendFunction(code, pStageFunction, 0, preCode, postCode);

        return code;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateComputeStage() {
        SR_TRACY_ZONE;
        auto&& entryPoint = SR_SRSL_ENTRY_POINTS.at(ShaderStage::Compute);
        auto&& pStageFunction = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction(entryPoint);
        auto&& pUseStackFunction = m_shader->GetUseStack()->FindFunction(entryPoint);
        if (!pStageFunction || !pUseStackFunction) {
            return { };
        }

        SR_UTILS_NS::String preCode;

        if (m_shader->GetUseStack()->IsVariableUsedInEntryPoints("GLOBAL_INVOCATION_ID")) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "uvec3 GLOBAL_INVOCATION_ID = gl_GlobalInvocationID;\n");
        }

        if (m_shader->GetUseStack()->IsVariableUsedInEntryPoints("WORK_GROUP_ID")) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "uvec3 WORK_GROUP_ID = gl_WorkGroupID;\n");
        }

        if (m_shader->GetUseStack()->IsVariableUsedInEntryPoints("NUM_WORK_GROUPS")) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "uvec3 NUM_WORK_GROUPS = gl_NumWorkGroups;\n");
        }

        if (m_shader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_ID")) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "uvec3 LOCAL_INVOCATION_ID = gl_LocalInvocationID;\n");
        }

        if (m_shader->GetUseStack()->IsVariableUsedInEntryPoints("LOCAL_INVOCATION_INDEX")) {
            CodeGenDetail::Append(preCode, GenerateTab(1), "uint LOCAL_INVOCATION_INDEX = gl_LocalInvocationIndex;\n");
        }

        auto&& code = GenerateStage(ShaderStage::Compute);
        AppendFunction(code, pStageFunction, 0, preCode, SR_UTILS_NS::String());

        return code;
    }

    SR_UTILS_NS::StringView GLSLGenerator::GetVersion(ShaderStage stage) const {
        SR_TRACY_ZONE;
        return "450";
    }

    uint32_t GLSLGenerator::GetLocationMultiplier(const SR_UTILS_NS::StringView& type) const {
        SR_TRACY_ZONE;
        if (type == "mat4") {
            return 4;
        }
        else if (type == "mat3") {
            return 3;
        }
        else if (type == "mat2") {
            return 2;
        }
        return 1;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateInputLocations(ShaderStage stage) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

        auto&& pFunction = m_shader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));

        for (auto&& [name, pVariable] : m_shader->GetConstants()) {
            if (pFunction->IsVariableUsed(name)) {
                const SR_UTILS_NS::String type = ReplaceToken(SRSLTypeInfo::Instance().GetTypeName(pVariable->pType), true);
                m_tmpBuffer.clear();
                SR_UTILS_NS::FormatTo(code, "const {} {} = {};\n\n", type, pVariable->pName->ToString(0, m_tmpBuffer), GenerateExpression(pVariable->pExpr, 0).c_str());
            }
        }

        auto&& pVertexFunction = m_shader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(ShaderStage::Vertex));

        uint32_t location = 0;

        const SR_UTILS_NS::VertexLayoutDescriptions& descriptions = m_shader->GetCreateInfo().vertexLayoutDescriptions;

        descriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
            SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
            SR_UTILS_NS::StringView attributeType = ToGLSLType(vertexAttribute.format, vertexAttribute.count);

            const bool isUsed = pFunction->IsVariableUsed(attributeName);

            if (isUsed && stage != ShaderStage::Vertex) {
                if (IsFlatType(attributeType)) {
                    SR_UTILS_NS::FormatTo(code, "layout (location = {}) flat in {} {};\n", location, attributeType, attributeName);
                }
                else {
                    SR_UTILS_NS::FormatTo(code, "layout (location = {}) in {} {};\n", location, attributeType, attributeName);
                }
            }
            else if (stage == ShaderStage::Vertex) {
                SR_UTILS_NS::FormatTo(code, "layout (location = {}) in {} {}_INPUT;\n", location, attributeType, attributeName);
            }
            else {
                return;
            }

            location += GetLocationMultiplier(attributeType);
        });

        if (stage != ShaderStage::Vertex) {
            for (auto&& [name, pVariable] : m_shader->GetShared()) {
                /// если переменную не передали, значит ее нет
                if (pVertexFunction && !pVertexFunction->IsVariableUsed(name)) {
                    continue;
                }

                if (pFunction->IsVariableUsed(name)) {
                    const SR_UTILS_NS::String type = ReplaceToken(SRSLTypeInfo::Instance().GetTypeName(pVariable->pType), true);
                    SR_UTILS_NS::FormatTo(code, "layout (location = {}) in {} {};\n", location, type, name);
                    location += GetLocationMultiplier(type);
                }
            }

            if (pFunction->IsVariableUsed("VERTEX_INDEX")) {
                SR_UTILS_NS::FormatTo(code, "layout (location = {}) in int VERTEX_INDEX;\n", location);
            }

            for (auto&& builtInAttribute : BuiltInGLSLAttributes) {
                SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(builtInAttribute.attribute);
                if (descriptions.Find(builtInAttribute.attribute)) {
                    continue;
                }
                bool isUsed = false;
                for (auto&& [name, pVariable] : m_shader->GetShared()) {
                    if (name == attributeName) {
                        isUsed = true;
                        break;
                    }
                }
                if (isUsed) {
                    continue;
                }

                SR_UTILS_NS::StringView attributeType = ToGLSLType(builtInAttribute.format, builtInAttribute.count);
                SR_UTILS_NS::FormatTo(code, "{} {}; /// added by builtin\n", attributeType, attributeName);
            }
        }

        if (stage == ShaderStage::Compute) {
            auto&& pComputeFunction = m_shader->GetAnalyzedTree()->pLexicalTree->FindFunction("compute");
            if (pComputeFunction) {
                auto&& threadsDecorator = pComputeFunction->pDecorators->Find("THREADS");
                if (threadsDecorator && threadsDecorator->args.size() == 3) {
                    SR_UTILS_NS::FormatTo(code, "layout (local_size_x = {}, local_size_y = {}, local_size_z = {}) in;\n",
                        threadsDecorator->args[0]->token,
                        threadsDecorator->args[1]->token,
                        threadsDecorator->args[2]->token
                    );
                }
                else {
                    SR_ERROR("GLSLGenerator::GenerateInputLocations() : invalid threads decorator for compute function!"
                        "\n\tShader: {}", m_shader->GetPath()
                    );
                }
            }
        }

        SR_UNUSED_VARIABLE(location);

        return code;
    }

    SR_UTILS_NS::String GLSLGenerator::GenerateOutputLocations(ShaderStage stage) const {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;

        auto&& pFunction = m_shader->GetUseStack()->FindFunction(SR_SRSL_ENTRY_POINTS.at(stage));

        uint32_t location = 0;

        if (stage == ShaderStage::Vertex) {
            const SR_UTILS_NS::VertexLayoutDescriptions& descriptions = m_shader->GetCreateInfo().vertexLayoutDescriptions;

            descriptions.ForEachAttribute([&](const SR_UTILS_NS::VertexAttributeDescription& vertexAttribute, uint32_t) {
                SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(vertexAttribute.attribute);
                SR_UTILS_NS::StringView attributeType = ToGLSLType(vertexAttribute.format, vertexAttribute.count);

                if (m_shader->GetUseStack()->IsVariableUsedInEntryPoint(ShaderStage::Fragment, attributeName)) {
                    if (IsFlatType(attributeType)) {
                        SR_UTILS_NS::FormatTo(code, "layout (location = {}) flat out {} {};\n", location, attributeType, attributeName);
                    }
                    else {
                        SR_UTILS_NS::FormatTo(code, "layout (location = {}) out {} {};\n", location, attributeType, attributeName);
                    }
                    location += GetLocationMultiplier(attributeType);
                }
                else {
                    SR_UTILS_NS::FormatTo(code, "{} {};\n", attributeType, attributeName);
                }
            });

            for (auto&& builtInAttribute : BuiltInGLSLAttributes) {
                SR_UTILS_NS::StringView attributeName = SR_UTILS_NS::VertexAttributeToName(builtInAttribute.attribute);
                if (descriptions.Find(builtInAttribute.attribute)) {
                    continue;
                }
                bool isUsed = false;
                for (auto&& [name, pVariable] : m_shader->GetShared()) {
                    if (name == attributeName) {
                        isUsed = true;
                        break;
                    }
                }
                if (isUsed) {
                    continue;
                }

                SR_UTILS_NS::StringView attributeType = ToGLSLType(builtInAttribute.format, builtInAttribute.count);
                if (m_shader->GetUseStack()->IsVariableUsedInEntryPoint(ShaderStage::Fragment, attributeName)) {
                    SR_UTILS_NS::FormatTo(code, "layout (location = {}) out {} {}; /// added by builtin\n", location, attributeType, attributeName);
                    location += GetLocationMultiplier(attributeType);
                }
                else {
                    SR_UTILS_NS::FormatTo(code, "{} {}; /// added by builtin\n", attributeType, attributeName);
                }
            }

            for (auto&& [name, pVariable] : m_shader->GetShared()) {
                if (m_shader->GetUseStack()->IsVariableUsedInEntryPoint(ShaderStage::Fragment, name)) {
                    const SR_UTILS_NS::String type = ReplaceToken(SRSLTypeInfo::Instance().GetTypeName(pVariable->pType), true);
                    if (IsFlatType(type)) {
                        SR_UTILS_NS::FormatTo(code, "layout (location = {}) flat out {} {}; /// added by shared\n", location, type, name);
                    }
                    else {
                        SR_UTILS_NS::FormatTo(code, "layout (location = {}) out {} {}; /// added by shared\n", location, type, name);
                    }
                    location += GetLocationMultiplier(type);
                }
                else if (pFunction->IsVariableUsed(name)) {
                    const SR_UTILS_NS::String type = ReplaceToken(SRSLTypeInfo::Instance().GetTypeName(pVariable->pType), true);
                    SR_UTILS_NS::FormatTo(code, "{} {}; /// added by shared\n", type, name);
                }
            }
        }

        SR_UNUSED_VARIABLE(location);

        return code;
    }

    bool GLSLGenerator::IsFlatType(const SR_UTILS_NS::StringView type) const {
        SR_TRACY_ZONE;
        return type == "int" || type == "uint" || type == "bool";
    }
}
