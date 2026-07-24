//
// Created by Monika on 26.01.2024.
//

#ifndef SR_ENGINE_SRSL_SHADER_VARIABLES_H
#define SR_ENGINE_SRSL_SHADER_VARIABLES_H

#include <Graphics/SRSL/ShaderType.h>

namespace SR_GRAPH_NS {
    enum class ShaderStage : uint8_t;
}

namespace SR_SRSL_NS {
    const SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, SR_UTILS_NS::StringAtom>& GetDefaultUniforms();

    extern const SR_UTILS_NS::Map<SR_UTILS_NS::StringView, SR_UTILS_NS::StringView> SR_SRSL_DEFAULT_PUSH_CONSTANTS;
    extern const SR_UTILS_NS::Map<SR_UTILS_NS::StringView, SR_UTILS_NS::StringView> SR_SRSL_DEFAULT_SHARED_UNIFORMS;
    extern const SR_UTILS_NS::Map<SR_UTILS_NS::StringView, SR_UTILS_NS::StringView> SR_SRSL_DEFAULT_SAMPLERS;
    extern const SR_UTILS_NS::StringView SR_SRSL_MAIN_OUT_LAYER;
    extern const SR_UTILS_NS::Set<SR_UTILS_NS::StringView> SR_SRSL_DEFAULT_OUT_LAYERS;
    extern const SR_UTILS_NS::Vector<SR_UTILS_NS::StringView> SR_SRSL_DEFAULT_OUT_LAYERS_USE_MACRO;
    extern const SR_UTILS_NS::Map<ShaderStage, SR_UTILS_NS::StringView> SR_SRSL_ENTRY_POINTS;
    extern const SR_UTILS_NS::Map<ShaderStage, SR_UTILS_NS::StringView> SR_SRSL_STAGE_EXTENSIONS;
    extern const SR_UTILS_NS::Map<SR_UTILS_NS::StringView, ShaderVarType> SR_SRSL_TYPE_STRINGS;
    extern const SR_UTILS_NS::Map<SR_UTILS_NS::StringView, uint64_t> SR_SRSL_TYPE_SIZE_TABLE;

    SR_NODISCARD bool IsShaderEntryPoint(SR_UTILS_NS::StringView name);
    SR_NODISCARD bool IsSampler(SR_UTILS_NS::StringView type);
    SR_NODISCARD uint64_t GetTypeSize(SR_UTILS_NS::StringView type) ;
}

#endif //SR_ENGINE_SRSL_SHADER_VARIABLES_H
