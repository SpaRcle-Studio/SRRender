//
// Created by Nikita on 29.03.2021.
//

#ifndef SR_ENGINE_GRAPHICS_SHADER_UTILS_H
#define SR_ENGINE_GRAPHICS_SHADER_UTILS_H

#include <Graphics/SRSL/ShaderType.h>

#include <Utils/Common/Enumerations.h>
#include <Utils/Common/Vertices.h>

namespace SR_GTYPES_NS {
    class Texture;
    class Shader;
}

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(MaterialStageUseType, uint8_t,
        Full,
        None,
        Uniforms,
        Samplers
    );

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
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_COMPUTE_STAGE = "COMPUTE_STAGE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_FILL_CENTER = "FILL_CENTER";
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
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_INVERSE_VIEW_MATRIX = "INVERSE_VIEW_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SSAO_SAMPLES = "SSAO_SAMPLES";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_LIGHT_SPACE_MATRIX = "LIGHT_SPACE_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_NO_TRANSLATE_MATRIX = "VIEW_NO_TRANSLATE_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PROJECTION_MATRIX = "PROJECTION_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_INVERSE_PROJECTION_MATRIX = "INVERSE_PROJECTION_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PROJECTION_NO_FOV_MATRIX = "PROJECTION_NO_FOV_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_ORTHOGONAL_MATRIX = "ORTHOGONAL_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PIXEL_ORTHOGONAL_MATRIX = "PIXEL_ORTHOGONAL_MATRIX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_DIRECTION = "VIEW_DIRECTION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_VIEW_POSITION = "VIEW_POSITION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CAMERA_FAR = "CAMERA_FAR";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CAMERA_NEAR = "CAMERA_NEAR";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TIME = "TIME";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_ASPECT = "ASPECT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_RESOLUTION = "RESOLUTION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKYBOX_DIFFUSE = "SKYBOX_DIFFUSE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_DEPTH_ATTACHMENT = "DEPTH_ATTACHMENT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_X = "TEXT_RECT_X";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_Y = "TEXT_RECT_Y";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_WIDTH = "TEXT_RECT_WIDTH";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_RECT_HEIGHT = "TEXT_RECT_HEIGHT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_DIRECTIONAL_LIGHT_DIRECTION = "DIRECTIONAL_LIGHT_DIRECTION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PC_SHADOW_CASCADE_INDEX = "PC_SHADOW_CASCADE_INDEX";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CASCADE_LIGHT_SPACE_MATRICES = "CASCADE_LIGHT_SPACE_MATRICES";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CASCADE_CENTERS = "CASCADE_CENTERS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CASCADE_RADII = "CASCADE_RADII";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_RENDER_PASS_TYPE = "RENDER_PASS_TYPE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_CASCADE_SPLITS = "CASCADE_SPLITS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PC_COLOR_BUFFER_MODE = "PC_COLOR_BUFFER_MODE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_PC_COLOR_BUFFER_VALUE = "PC_COLOR_BUFFER_VALUE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SSAO_NOISE = "SSAO_NOISE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_RGBA_VALUE = "RGBA_VALUE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_ATLAS_SIZE = "TEXT_ATLAS_SIZE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_TEXT_ATLAS_TEXTURE = "TEXT_ATLAS_TEXTURE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_NDC_RECT = "NDC_RECT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_UI_SCALE = "UI_SCALE";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_UI_PIVOT = "UI_PIVOT";

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SUN_COLOR = "SUN_COLOR";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SUN_INTENSITY = "SUN_INTENSITY";

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SKY_COLOR = "SKY_COLOR";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_GROUND_COLOR = "GROUND_COLOR";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_AMBIENT_INTENSITY = "AMBIENT_INTENSITY";

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SHADOW_STRENGTH = "SHADOW_STRENGTH";

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_POST_PROCESS_VIGNETTE_INTENSITY = "vignetteIntensity";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_POST_PROCESS_CHROMATIC_ABERRATION_INTENSITY = "chromaticAberrationIntensity";

    /**
     * 0 - sliced
     * 1 - filled
     */
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SPRITE_MODE = "SPRITE_MODE";

    /**
     * 0 - horizontal
     * 1 - vertical
     * 2 - radial90
     * 3 - radial180
     * 4 - radial360
     */
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SPRITE_FILL_METHOD = "SPRITE_FILL_METHOD";

    /**
     * 0 - left
     * 1 - right
     * 2 - bottom
     * 3 - top
     * 4 - bottom-left
     * 5 - top-left
     * 6 - top-right
     * 7 - bottom-right
     */
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SPRITE_FILL_ORIGIN = "SPRITE_FILL_ORIGIN";

    /// range [0..1]
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SPRITE_FILL_AMOUNT = "SPRITE_FILL_AMOUNT";

    /// 0 - disabled, 1 - enabled
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_SPRITE_FILL_CLOCKWISE = "SPRITE_FILL_CLOCKWISE";

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_COLOR_PASS = "SR_DEFINE_COLOR_PASS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_CASCADED_SHADOW_MAP_PASS = "SR_DEFINE_CASCADED_SHADOW_MAP_PASS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_USE_CASCADED_SHADOW_MAP = "SR_DEFINE_USE_CASCADED_SHADOW_MAP";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_NORMAL = "HAS_NORMAL";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_SSS = "HAS_SSS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_SKELETON = "HAS_SKELETON";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_ALPHA = "HAS_ALPHA";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_ROUGHNESS = "HAS_ROUGHNESS";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_DETAIL_WEIGHT = "HAS_DETAIL_WEIGHT";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_DISABLE_BLENDING = "DISABLE_BLENDING";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_ALPHA_MASK = "HAS_ALPHA_MASK";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_EMISSION = "HAS_EMISSION";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SHADER_MACRO_SR_DEFINE_HAS_ORM = "HAS_ORM";

    SR_ENUM_NS_CLASS_T(ShaderStage, uint8_t,
        Unknown,
        All,
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
    )

    SR_ENUM_NS_CLASS(LayoutBinding, Unknown = 0, Uniform, Sampler2D, Attachhment, SSBO)
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

    struct SR_GRAPHICS_DLL_API Uniform {
        LayoutBinding type = LayoutBinding::Unknown;
        ShaderStage stage = ShaderStage::Unknown;
        uint64_t binding = 0;
        uint64_t size = 0;
    };

    typedef SR_UTILS_NS::Vector<Uniform> UBOInfo;

    struct SR_GRAPHICS_DLL_API SRShaderPushConstant {
        uint64_t size = 0;
        uint64_t offset = 0;
    };

    struct SR_GRAPHICS_DLL_API SRShaderStageInfo {
    public:
        SR_UTILS_NS::Path path;
        SR_UTILS_NS::Vector<SRShaderPushConstant> pushConstants;

    };

    struct SR_GRAPHICS_DLL_API SRShaderCreateInfo {
    public:
        SR_NODISCARD bool Validate() const noexcept;

    public:
        SRShaderCreateInfo() = default;
        SRShaderCreateInfo(SRShaderCreateInfo&& ref) noexcept;
        SRShaderCreateInfo& operator=(SRShaderCreateInfo&& ref) noexcept;
        SRShaderCreateInfo(const SRShaderCreateInfo&) = default;
        SRShaderCreateInfo& operator=(const SRShaderCreateInfo&) = default;

        SR_UTILS_NS::Map<ShaderStage, SRShaderStageInfo> stages;

        SR_SRSL_NS::ShaderType shaderType = SR_SRSL_NS::ShaderType::Unknown;

        SR_UTILS_NS::VertexLayoutDescriptions vertexLayoutDescriptions;

        PolygonMode       polygonMode       = PolygonMode::Unknown;
        CullMode          cullMode          = CullMode::Unknown;
        DepthCompare      depthCompare      = DepthCompare::Unknown;
        PrimitiveTopology primitiveTopology = PrimitiveTopology::Unknown;

        UBOInfo uniforms;

        bool blendEnabled = false;
        bool depthWrite   = false;
        bool depthTest    = false;
        bool alphaCoverage = false;

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

    struct SourceShader {
        std::string m_path;
        ShaderStage m_stage;

        SourceShader(const std::string& path, ShaderStage stage) {
            m_path  = path;
            m_stage = stage;
        }
    };
}

#endif //SR_ENGINE_GRAPHICS_SHADER_UTILS_H
