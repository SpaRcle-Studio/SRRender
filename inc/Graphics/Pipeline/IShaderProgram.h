//
// Created by Nikita on 29.03.2021.
//

#ifndef SR_ENGINE_GRAPHICS_I_SHADER_PROGRAM_H
#define SR_ENGINE_GRAPHICS_I_SHADER_PROGRAM_H

#include <Utils/FileSystem/FileSystem.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Common/StringUtils.h>
#include <Utils/Common/Hashes.h>
#include <Utils/Common/Enumerations.h>

#include <Graphics/Types/Uniforms.h>
#include <Graphics/Types/Vertices.h>

namespace SR_GTYPES_NS {
    class Texture;
    class Shader;
}

namespace SR_GRAPH_NS {
    struct ShaderUseInfo {
        ShaderUseInfo() = default;
        explicit ShaderUseInfo(SR_GTYPES_NS::Shader* pShader)
            : pShader(pShader)
            , ignoreReplace(false)
            , useMaterial(true)
        { }

        SR_GTYPES_NS::Shader* pShader = nullptr;
        bool ignoreReplace = false;
        bool useMaterial = false;
    };

    SR_ENUM_NS_CLASS_T(ShaderBindResult, uint8_t,
        Failed = 0,  /// false
        Success = 1, /// true
        Duplicated,
        ReAllocated
    );

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_LINE_START_POINT = "LINE_START_POINT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_LINE_END_POINT = "LINE_END_POINT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_LINE_COLOR = "LINE_COLOR";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MODEL_MATRIX = "MODEL_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SLICED_TEXTURE_BORDER = "SLICED_TEXTURE_BORDER";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SLICED_WINDOW_BORDER = "SLICED_WINDOW_BORDER";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MODEL_NO_SCALE_MATRIX = "MODEL_NO_SCALE_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKELETON_MATRICES_128 = "SKELETON_MATRICES_128";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKELETON_MATRIX_OFFSETS_128 = "SKELETON_MATRIX_OFFSETS_128";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKELETON_MATRICES_256 = "SKELETON_MATRICES_256";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKELETON_MATRIX_OFFSETS_256 = "SKELETON_MATRIX_OFFSETS_256";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKELETON_MATRICES_384 = "SKELETON_MATRICES_384";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKELETON_MATRIX_OFFSETS_384 = "SKELETON_MATRIX_OFFSETS_384";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_MATRIX = "VIEW_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SSAO_SAMPLES = "SSAO_SAMPLES";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_LIGHT_SPACE_MATRIX = "LIGHT_SPACE_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_NO_TRANSLATE_MATRIX = "VIEW_NO_TRANSLATE_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PROJECTION_MATRIX = "PROJECTION_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_ORTHOGONAL_MATRIX = "ORTHOGONAL_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_DIRECTION = "VIEW_DIRECTION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_POSITION = "VIEW_POSITION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TIME = "TIME";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_RESOLUTION = "RESOLUTION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKYBOX_DIFFUSE = "SKYBOX_DIFFUSE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_DEPTH_ATTACHMENT = "DEPTH_ATTACHMENT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_X = "TEXT_RECT_X";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_Y = "TEXT_RECT_Y";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_WIDTH = "TEXT_RECT_WIDTH";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_HEIGHT = "TEXT_RECT_HEIGHT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_DIRECTIONAL_LIGHT_POSITION = "DIRECTIONAL_LIGHT_POSITION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SHADOW_CASCADE_INDEX = "SHADOW_CASCADE_INDEX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CASCADE_LIGHT_SPACE_MATRICES = "CASCADE_LIGHT_SPACE_MATRICES";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CASCADE_SPLITS = "CASCADE_SPLITS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_COLOR_BUFFER_MODE = "COLOR_BUFFER_MODE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_COLOR_BUFFER_VALUE = "COLOR_BUFFER_VALUE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SSAO_NOISE = "SSAO_NOISE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_ATLAS_TEXTURE = "TEXT_ATLAS_TEXTURE";

    typedef std::vector<std::pair<Vertices::Attribute, size_t>> VertexAttributes;
    typedef std::vector<SR_VERTEX_DESCRIPTION> VertexDescriptions;

    SR_DEPRECATED
    typedef std::variant<glm::mat4, glm::mat3, glm::mat2, float, int, glm::vec2, glm::vec3, glm::vec4, glm::ivec2, glm::ivec3, glm::ivec4> ShaderVariable;

    SR_ENUM_NS_CLASS_T(ShaderStage, uint8_t,
        Unknown,
        Vertex,
        Fragment,
        Geometry,
        Tesselation,
        Compute,
        Raygen,
        Intersection,
        HitClosest,
        HitAny,
        MissPrimary,
        MissSecondary
    );

    SR_ENUM_NS_CLASS(LayoutBinding, Unknown = 0, Uniform = 1, Sampler2D = 2, Attachhment=3)
    SR_ENUM_NS_CLASS(PolygonMode, Unknown, Fill, Line, Point)
    SR_ENUM_NS_CLASS(CullMode, Unknown, None, Front, Back, FrontAndBack)
    SR_ENUM_NS_CLASS(PrimitiveTopology,
            Unknown,
            PointList,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip,
            TriangleFan,
            LineListWithAdjacency,
            LineStripWithAdjacency,
            TriangleListWithAdjacency,
            TriangleStripWithAdjacency,
            PathList)

    SR_ENUM_NS_CLASS(DepthCompare,
        Unknown,
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always)

    struct SR_DLL_EXPORT Uniform {
        LayoutBinding type = LayoutBinding::Unknown;
        ShaderStage stage = ShaderStage::Unknown;
        uint64_t binding = 0;
        uint64_t size = 0;
    };

    typedef std::vector<Uniform> UBOInfo;

    struct SR_DLL_EXPORT SRShaderPushConstant {
        uint64_t size = 0;
        uint64_t offset = 0;
    };

    struct SR_DLL_EXPORT SRShaderStageInfo {
    public:
        SR_UTILS_NS::Path path;
        std::vector<SRShaderPushConstant> pushConstants;

    };

    struct SR_DLL_EXPORT SRShaderCreateInfo {
    public:
        SR_NODISCARD bool Validate() const noexcept {
            return polygonMode       != PolygonMode::Unknown
                   && cullMode          != CullMode::Unknown
                   && depthCompare      != DepthCompare::Unknown
                   && primitiveTopology != PrimitiveTopology::Unknown;
        }

    public:
        std::map<ShaderStage, SRShaderStageInfo> stages;

        PolygonMode       polygonMode       = PolygonMode::Unknown;
        CullMode          cullMode          = CullMode::Unknown;
        DepthCompare      depthCompare      = DepthCompare::Unknown;
        PrimitiveTopology primitiveTopology = PrimitiveTopology::Unknown;

        VertexAttributes vertexAttributes;
        VertexDescriptions vertexDescriptions;
        UBOInfo uniforms;

        bool blendEnabled = false;
        bool depthWrite   = false;
        bool depthTest    = false;

    };

    SR_MAYBE_UNUSED static CullMode InverseCullMode(CullMode cullMode) {
        switch (cullMode) {
            case CullMode::Back:
                return CullMode::Front;
            case CullMode::Front:
                return CullMode::Back;
            default:
                return cullMode;
        }
    }

    SR_MAYBE_UNUSED static LayoutBinding GetBindingType(const std::string& line) {
        //! first check sampler, after that check uniform

        if (SR_UTILS_NS::StringUtils::Contains(line, "sampler2DArray"))
            return LayoutBinding::Sampler2D;

        if (SR_UTILS_NS::StringUtils::Contains(line, "sampler2D"))
            return LayoutBinding::Sampler2D;

        if (SR_UTILS_NS::StringUtils::Contains(line, "samplerCube"))
            return LayoutBinding::Sampler2D;

        if (SR_UTILS_NS::StringUtils::Contains(line, "subpassInputMS"))
            return LayoutBinding::Attachhment;

        if (SR_UTILS_NS::StringUtils::Contains(line, "subpassInput"))
            return LayoutBinding::Attachhment;

        if (SR_UTILS_NS::StringUtils::Contains(line, "uniform"))
            return LayoutBinding::Uniform;

        return LayoutBinding::Unknown;
    }

    struct SourceShader {
        std::string m_path;
        ShaderStage m_stage;

        SourceShader(const std::string& path, ShaderStage stage) {
            m_path  = path;
            m_stage = stage;
        }
    };

    SR_MAYBE_UNUSED static std::optional<std::vector<Uniform>> AnalyseShader(const std::vector<SourceShader>& modules) {
        uint32_t count = 0;

        auto uniforms = std::vector<Uniform>();

        std::vector<std::string> lines = { };
        for (auto&& module : modules) {
            auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().Concat("Shaders").Concat(module.m_path);
            lines = SR_UTILS_NS::FileSystem::ReadAllLines(path);
            if (lines.empty()) {
                SR_ERROR("Graphics::AnalyseShader() : failed to read module! \n\tPath: " + path.ToString());
                return std::optional<std::vector<Uniform>>();
            }

            for (std::string line : lines) {
                if (const auto&& pos = line.find("//"); pos != std::string::npos) {
                    line.resize(pos);
                }

                int32_t bindingIndex = SR_UTILS_NS::StringUtils::IndexOf(line, "binding");
                if (bindingIndex >= 0) {
                    int32_t index = SR_UTILS_NS::StringUtils::IndexOf(line, '=', bindingIndex);

                    int32_t comment = SR_UTILS_NS::StringUtils::IndexOf(line, '/');
                    if (comment >= 0 && comment < index)
                        continue;

                    if (index <= 0) {
                        SRHalt("Graphics::AnalyseShader() : incorrect binding location!");
                        return std::optional<std::vector<Uniform>>();
                    }

                    const auto&& location = SR_UTILS_NS::StringUtils::ReadNumber(line, index + 2 /** space and assign */);
                    if (location.empty()) {
                        SR_ERROR("Graphics::AnalyseShader() : failed match location!");
                        return std::optional<std::vector<Uniform>>();
                    }

                    Uniform uniform {
                        .type = GetBindingType(line),
                        .stage = module.m_stage,
                        .binding = static_cast<uint32_t>(SR_UTILS_NS::LexicalCast<uint32_t>(location))
                    };

                    uniforms.emplace_back(uniform);

                    count++;
                }
            }
        }

        /// error correction
        for (auto&& uniform : uniforms) {
            if (uniform.stage == ShaderStage::Unknown || uniform.type == LayoutBinding::Unknown) {
                SR_ERROR("IShaderProgram::AnalyseShader() : incorrect uniforms!");
                return std::optional<std::vector<Uniform>>();
            }
        }

        return uniforms;
    }
}

#endif //SR_ENGINE_GRAPHICS_I_SHADER_PROGRAM_H
