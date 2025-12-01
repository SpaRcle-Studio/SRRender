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
    std::string ShaderRenderPassTypeToString(ShaderRenderPassType type);

    const std::map<std::string, std::string>& GetDefaultUniforms();

    extern const std::map<std::string, std::string> SR_SRSL_DEFAULT_PUSH_CONSTANTS;
    extern const std::map<std::string, std::string> SR_SRSL_DEFAULT_SHARED_UNIFORMS;
    extern const std::map<std::string, std::string> SR_SRSL_DEFAULT_SAMPLERS;
    extern const std::string SR_SRSL_MAIN_OUT_LAYER;
    extern const std::set<std::string> SR_SRSL_DEFAULT_OUT_LAYERS;
    extern const std::map<ShaderStage, std::string> SR_SRSL_ENTRY_POINTS;
    extern const std::map<ShaderStage, std::string> SR_SRSL_STAGE_EXTENSIONS;
    extern const std::map<std::string, ShaderVarType> SR_SRSL_TYPE_STRINGS;
    extern const std::map<std::string, uint64_t> SR_SRSL_TYPE_SIZE_TABLE;

    SR_INLINE_STATIC bool IsShaderEntryPoint(const std::string& name) {
        for (auto&& [stage, entryPoint] : SR_SRSL_ENTRY_POINTS) {
            if (entryPoint == name) {
                return true;
            }
        }

        return false;
    }

    SR_INLINE_STATIC bool IsSampler(const std::string& type) {
        return
            type.find("ampler") != std::string::npos ||
            type.find("mage2DMS") != std::string::npos ||
            type.find("ubpassInput") != std::string::npos;
    }

    SR_INLINE_STATIC uint64_t GetTypeSize(const std::string& type) {
        if (IsSampler(type)) {
            SRHalt("Samplers have not size!");
            return 0;
        }

        static std::map<std::string, uint64_t> typeSizes = {
            { "float", 4 }, { "int", 4 },
            { "vec2", 8 }, { "ivec2", 8 },
            { "vec3", 12 }, { "ivec3", 12 },
            { "vec4", 16 }, { "ivec4", 16 },
            { "mat2", 4 * 2 * 2 }, { "mat3", 4 * 3 * 3 }, { "mat4", 4 * 4 * 4 },
        };

        if (auto&& pIt = typeSizes.find(type); pIt != typeSizes.end()) {
            return pIt->second;
        }

        return 0;
    }
}

#endif //SR_ENGINE_SRSL_SHADER_VARIABLES_H
