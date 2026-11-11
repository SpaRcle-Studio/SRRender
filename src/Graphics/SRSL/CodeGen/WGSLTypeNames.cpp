//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLDetail.h>
#include <Graphics/SRSL/CodeGen/Common.h>

namespace SR_SRSL_NS::WGSLDetail {
    SR_UTILS_NS::StringView GenerateType(SR_UTILS_NS::StringView type) {
        SR_TRACY_ZONE;
        // ---- scalars ----
        if (type == "float")
            return "f32";
        if (type == "double")
            return "f32"; // WGSL has no f64 in shaders
        if (type == "int")
            return "i32";
        if (type == "uint")
            return "u32";
        if (type == "bool")
            return "bool";

        // ---- float vectors ----
        if (type == "vec2" || type == "vec2f")
            return "vec2<f32>";
        if (type == "vec3" || type == "vec3f")
            return "vec3<f32>";
        if (type == "vec4" || type == "vec4f")
            return "vec4<f32>";

        // ---- int vectors ----
        if (type == "ivec2")
            return "vec2<i32>";
        if (type == "ivec3")
            return "vec3<i32>";
        if (type == "ivec4")
            return "vec4<i32>";

        // ---- uint vectors ----
        if (type == "uvec2")
            return "vec2<u32>";
        if (type == "uvec3")
            return "vec3<u32>";
        if (type == "uvec4")
            return "vec4<u32>";

        // ---- bool vectors ----
        if (type == "bvec2")
            return "vec2<bool>";
        if (type == "bvec3")
            return "vec3<bool>";
        if (type == "bvec4")
            return "vec4<bool>";

        // ---- matrices (GLSL column-major → WGSL matCxR) ----
        if (type == "mat2" || type == "mat2x2")
            return "mat2x2<f32>";
        if (type == "mat3" || type == "mat3x3")
            return "mat3x3<f32>";
        if (type == "mat4" || type == "mat4x4")
            return "mat4x4<f32>";
        if (type == "mat2x3")
            return "mat2x3<f32>";
        if (type == "mat2x4")
            return "mat2x4<f32>";
        if (type == "mat3x2")
            return "mat3x2<f32>";
        if (type == "mat3x4")
            return "mat3x4<f32>";
        if (type == "mat4x2")
            return "mat4x2<f32>";
        if (type == "mat4x3")
            return "mat4x3<f32>";

        // ---- void ----
        if (type == "void")
            return "void";

        // ---- samplers — not a direct type in WGSL, but we return empty so callers skip ----
        if (type == "sampler2D")
            return {};
        if (type == "sampler2DArrayDepth")
            return {};
        if (type == "sampler3D")
            return {};
        if (type == "samplerCube")
            return {};
        if (type == "sampler2DShadow")
            return {};

        // ---- unknown / user struct → keep as-is ----
        return type;
    }

    SR_UTILS_NS::StringView VertexAttributeFormatToString(SR_UTILS_NS::VertexAttributeFormat format, uint8_t count) {
        SR_TRACY_ZONE;
        if (count == 0 || count > 4) {
            SRHalt0();
            return "unknown";
        }
        static constexpr SR_UTILS_NS::StringView types[4][4] = {{"f32", "vec2<f32>", "vec3<f32>", "vec4<f32>"},
                                                                {"i32", "vec2<i32>", "vec3<i32>", "vec4<i32>"},
                                                                {"u32", "vec2<u32>", "vec3<u32>", "vec4<u32>"},
                                                                {"f16", "vec2<f16>", "vec3<f16>", "vec4<f16>"}};
        return types[static_cast<uint8_t>(CodeGenDetail::GetVertexScalarKind(format))][count - 1];
    }

    bool IsWGSLReservedKeyword(SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        // https://www.w3.org/TR/WGSL/#reserved-words
        static const SR_HTYPES_NS::FlatHashSet<SR_UTILS_NS::StringView> reserved = {"target",
                                                                                    "result",
                                                                                    "output",
                                                                                    "input",
                                                                                    "type",
                                                                                    "module",
                                                                                    "abstract",
                                                                                    "active",
                                                                                    "alignas",
                                                                                    "alignof",
                                                                                    "as",
                                                                                    "asm",
                                                                                    "asm_fragment",
                                                                                    "async",
                                                                                    "attribute",
                                                                                    "auto",
                                                                                    "await",
                                                                                    "become",
                                                                                    "binding_array",
                                                                                    "cast",
                                                                                    "catch",
                                                                                    "class",
                                                                                    "co_await",
                                                                                    "co_return",
                                                                                    "co_yield",
                                                                                    "coherent",
                                                                                    "column_major",
                                                                                    "common",
                                                                                    "compile",
                                                                                    "compile_fragment",
                                                                                    "concept",
                                                                                    "const_cast",
                                                                                    "consteval",
                                                                                    "constexpr",
                                                                                    "constinit",
                                                                                    "crate",
                                                                                    "debugger",
                                                                                    "decltype",
                                                                                    "delete",
                                                                                    "demote",
                                                                                    "demote_to_helper",
                                                                                    "do",
                                                                                    "dynamic_cast",
                                                                                    "enum",
                                                                                    "explicit",
                                                                                    "export",
                                                                                    "extends",
                                                                                    "extern",
                                                                                    "external",
                                                                                    "fallthrough",
                                                                                    "filter",
                                                                                    "final",
                                                                                    "finally",
                                                                                    "friend",
                                                                                    "from",
                                                                                    "fxgroup",
                                                                                    "get",
                                                                                    "goto",
                                                                                    "groupshared",
                                                                                    "highp",
                                                                                    "impl",
                                                                                    "implements",
                                                                                    "import",
                                                                                    "inline",
                                                                                    "instanceof",
                                                                                    "interface",
                                                                                    "layout",
                                                                                    "lowp",
                                                                                    "macro",
                                                                                    "macro_rules",
                                                                                    "match",
                                                                                    "mediump",
                                                                                    "meta",
                                                                                    "mod",
                                                                                    "modf",
                                                                                    "move",
                                                                                    "mut",
                                                                                    "mutable",
                                                                                    "namespace",
                                                                                    "new",
                                                                                    "nil",
                                                                                    "noexcept",
                                                                                    "noinline",
                                                                                    "nointerpolation",
                                                                                    "noperspective",
                                                                                    "null",
                                                                                    "nullptr",
                                                                                    "of",
                                                                                    "operator",
                                                                                    "package",
                                                                                    "packoffset",
                                                                                    "partition",
                                                                                    "pass",
                                                                                    "patch",
                                                                                    "pixelfragment",
                                                                                    "precise",
                                                                                    "precision",
                                                                                    "premerge",
                                                                                    "priv",
                                                                                    "protected",
                                                                                    "pub",
                                                                                    "public",
                                                                                    "readonly",
                                                                                    "ref",
                                                                                    "regardless",
                                                                                    "register",
                                                                                    "reinterpret_cast",
                                                                                    "require",
                                                                                    "resource",
                                                                                    "restrict",
                                                                                    "self",
                                                                                    "set",
                                                                                    "shared",
                                                                                    "sizeof",
                                                                                    "smooth",
                                                                                    "snorm",
                                                                                    "static",
                                                                                    "static_assert",
                                                                                    "static_cast",
                                                                                    "std",
                                                                                    "subroutine",
                                                                                    "super",
                                                                                    "target",
                                                                                    "template",
                                                                                    "this",
                                                                                    "thread_local",
                                                                                    "throw",
                                                                                    "trait",
                                                                                    "try",
                                                                                    "typedef",
                                                                                    "typeid",
                                                                                    "typename",
                                                                                    "typeof",
                                                                                    "union",
                                                                                    "unless",
                                                                                    "unorm",
                                                                                    "unsafe",
                                                                                    "unsized",
                                                                                    "use",
                                                                                    "using",
                                                                                    "varying",
                                                                                    "virtual",
                                                                                    "volatile",
                                                                                    "wgsl",
                                                                                    "where",
                                                                                    "with",
                                                                                    "writeonly",
                                                                                    "yield"};
        return reserved.count(token) > 0;
    }

    void AppendIdentifier(SR_UTILS_NS::String& code, SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        if (IsWGSLReservedKeyword(token)) {
            code += '_';
        }
        code += token;
    }

    void AppendToken(SR_UTILS_NS::String& code, SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        AppendIdentifier(code, ReplaceToken(token));
    }

    SR_UTILS_NS::String SafeIdentifier(SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(token.size() + 1);
        AppendIdentifier(code, token);
        return code;
    }

    bool IsCompoundAssignment(SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        return token == "+=" || token == "-=" || token == "*=" || token == "/=" || token == "%=" || token == "&=" ||
               token == "|=" || token == "^=" || token == "<<=" || token == ">>=" || token == "||=" || token == "&&=";
    }

    SR_UTILS_NS::String ToWGSLArrayType(SR_UTILS_NS::StringView baseType,
                                        const SR_UTILS_NS::Vector<uint64_t>& dims) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(baseType.size() + dims.size() * 32);
        for (size_t i = 0; i < dims.size(); ++i) {
            code += "array<";
        }
        code += baseType;
        for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
            if (*it != 0) {
                SR_UTILS_NS::FormatTo(code, ", {}", *it);
            }
            code += '>';
        }
        return code;
    }

    uint32_t SquareMatrixDimension(SR_UTILS_NS::StringView wgslType) {
        SR_TRACY_ZONE;
        if (wgslType == "mat2x2<f32>")
            return 2;
        if (wgslType == "mat3x3<f32>")
            return 3;
        if (wgslType == "mat4x4<f32>")
            return 4;
        return 0;
    }

    uint32_t MatrixColumns(SR_UTILS_NS::StringView wgslType) {
        SR_TRACY_ZONE;
        if (wgslType.size() >= 7 && wgslType.starts_with("mat") && wgslType[4] == 'x') {
            return static_cast<uint32_t>(wgslType[3] - '0');
        }
        return 0;
    }

    SR_UTILS_NS::StringView MatrixColumnType(SR_UTILS_NS::StringView wgslType) {
        SR_TRACY_ZONE;
        if (MatrixColumns(wgslType) == 0) {
            return {};
        }
        static constexpr SR_UTILS_NS::StringView columns[] = {"vec2<f32>", "vec3<f32>", "vec4<f32>"};
        return columns[wgslType[5] - '2'];
    }

    bool IsScalarType(SR_UTILS_NS::StringView wgslType) {
        SR_TRACY_ZONE;
        return wgslType == "f32" || wgslType == "i32" || wgslType == "u32";
    }

    bool NeedsFlatInterpolation(SR_UTILS_NS::StringView wgslType) {
        SR_TRACY_ZONE;
        return wgslType.find("i32") != SR_UTILS_NS::String::npos || wgslType.find("u32") != SR_UTILS_NS::String::npos ||
               wgslType.find("bool") != SR_UTILS_NS::String::npos;
    }

    bool IsNumericLiteral(SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        if (token.empty()) {
            return false;
        }
        return std::isdigit(static_cast<unsigned char>(token.front())) || token.front() == '.';
    }

    SR_UTILS_NS::StringView ReplaceToken(SR_UTILS_NS::StringView token) {
        SR_TRACY_ZONE;
        // Type replacements inside expressions (constructors, casts)
        if (token == "float")
            return "f32";
        if (token == "int")
            return "i32";
        if (token == "uint")
            return "u32";
        if (token == "vec2")
            return "vec2<f32>";
        if (token == "vec3")
            return "vec3<f32>";
        if (token == "vec4")
            return "vec4<f32>";
        if (token == "ivec2")
            return "vec2<i32>";
        if (token == "ivec3")
            return "vec3<i32>";
        if (token == "ivec4")
            return "vec4<i32>";
        if (token == "uvec2")
            return "vec2<u32>";
        if (token == "uvec3")
            return "vec3<u32>";
        if (token == "uvec4")
            return "vec4<u32>";
        if (token == "bvec2")
            return "vec2<bool>";
        if (token == "bvec3")
            return "vec3<bool>";
        if (token == "bvec4")
            return "vec4<bool>";
        if (token == "mat2")
            return "mat2x2<f32>";
        if (token == "mat3")
            return "mat3x3<f32>";
        if (token == "mat4")
            return "mat4x4<f32>";

        // Builtin functions that differ
        if (token == "mix")
            return "mix";
        if (token == "fract")
            return "fract";
        if (token == "dFdx")
            return "dpdx";
        if (token == "dFdy")
            return "dpdy";
        if (token == "inversesqrt")
            return "inverseSqrt";
        if (token == "fma")
            return "fma";
        if (token == "mod")
            return ""; // handled in GenerateExpression as %
        if (token == "atan")
            return "atan2";
        // texture/textureSample/textureLod are handled specially in GenerateExpression
        // to inject the sampler argument — these mappings are kept as fallback identifiers.
        if (token == "texture")
            return "textureSample";
        if (token == "textureLod")
            return "textureSampleLevel";

        // GLSL textureSize → WGSL textureDimensions
        if (token == "textureSize")
            return "textureDimensions";

        // GLSL barrier → WGSL workgroupBarrier
        if (token == "barrier")
            return "workgroupBarrier";

        // GLSL builtins → WGSL equivalents
        if (token == "gl_Position")
            return "vsOut.position";
        if (token == "gl_FragCoord")
            return "fsIn.position";
        if (token == "gl_VertexIndex")
            return "vertexIndex";
        if (token == "gl_GlobalInvocationID")
            return "global_id";
        if (token == "gl_LocalInvocationID")
            return "local_id";
        if (token == "gl_WorkGroupID")
            return "workgroup_id";
        if (token == "gl_NumWorkGroups")
            return "num_workgroups";
        if (token == "gl_LocalInvocationIndex")
            return "local_index";

        return token;
    }
} // namespace SR_SRSL_NS::WGSLDetail
