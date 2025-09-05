//
// Created by Monika on 05.09.2025.
//

#ifndef SR_ENGINE_GRAPHICS_SRSL_SHADER_CACHE_H
#define SR_ENGINE_GRAPHICS_SRSL_SHADER_CACHE_H

#include <Graphics/macros.h>

#include <Utils/FileSystem/Path.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_HTYPES_NS {
    class Marshal;
}

namespace SR_SRSL_NS {
    class SRSLShader;

    class SRSLShaderCache : public SR_UTILS_NS::NonCopyable, public SR_UTILS_NS::NonMovable {
    public:
        SRSLShaderCache() = delete;

    public:
        static void SaveShaderToCache(const SR_UTILS_NS::Path& cachePath, const SR_SRSL_NS::SRSLShader* pShader);
        static bool LoadShaderFromCache(const SR_UTILS_NS::Path &cachePath, SR_SRSL_NS::SRSLShader* pShader);

    };
}

namespace SR_GRAPH_NS {
    namespace Memory {
        struct ShaderUBOBlock;
    }

    class ShaderCache : public SR_UTILS_NS::NonCopyable, public SR_UTILS_NS::NonMovable {
    public:
        ShaderCache() = delete;

    public:
        static void SaveShaderToCache(const SR_UTILS_NS::Path& cachePath, const  SR_GTYPES_NS::Shader* pShader);
        static bool LoadShaderFromCache(const SR_UTILS_NS::Path &cachePath,  SR_GTYPES_NS::Shader* pShader);

    private:
        static uint64_t GetVersion();

        static void SaveUBOBlock(SR_HTYPES_NS::Marshal& marshal, const Memory::ShaderUBOBlock& block);
        static void LoadUBOBlock(SR_HTYPES_NS::Marshal& marshal, Memory::ShaderUBOBlock& block);

    };
}

#endif //SR_ENGINE_GRAPHICS_SRSL_SHADER_CACHE_H
