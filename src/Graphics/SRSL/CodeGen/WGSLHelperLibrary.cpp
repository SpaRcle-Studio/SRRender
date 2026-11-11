//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLDetail.h>
#include <Graphics/SRSL/CodeGen/Common.h>

namespace SR_SRSL_NS::WGSLDetail {
    SR_UTILS_NS::StringView MatrixCastHelperName(uint32_t target, uint32_t source) {
        SR_TRACY_ZONE;
        static constexpr SR_UTILS_NS::StringView names[3][3] = {
            {"sr_mat2_from_mat2", "sr_mat2_from_mat3", "sr_mat2_from_mat4"},
            {"sr_mat3_from_mat2", "sr_mat3_from_mat3", "sr_mat3_from_mat4"},
            {"sr_mat4_from_mat2", "sr_mat4_from_mat3", "sr_mat4_from_mat4"}};
        return names[target - 2][source - 2];
    }

    SR_UTILS_NS::StringView MatrixDiagonalHelperName(uint32_t target) {
        SR_TRACY_ZONE;
        static constexpr SR_UTILS_NS::StringView names[] = {"sr_mat2_diag", "sr_mat3_diag", "sr_mat4_diag"};
        return names[target - 2];
    }

    SR_UTILS_NS::StringView InverseHelperName(uint32_t dimension) {
        SR_TRACY_ZONE;
        static constexpr SR_UTILS_NS::StringView names[] = {"sr_inverse2", "sr_inverse3", "sr_inverse4"};
        return names[dimension - 2];
    }

    SR_UTILS_NS::StringView MatrixTypeName(uint32_t dimension) {
        SR_TRACY_ZONE;
        static constexpr SR_UTILS_NS::StringView names[] = {"mat2x2<f32>", "mat3x3<f32>", "mat4x4<f32>"};
        return names[dimension - 2];
    }

    SR_UTILS_NS::StringView VectorTypeName(uint32_t dimension) {
        SR_TRACY_ZONE;
        static constexpr SR_UTILS_NS::StringView names[] = {"vec2<f32>", "vec3<f32>", "vec4<f32>"};
        return names[dimension - 2];
    }

    SR_UTILS_NS::String GenerateMatrixCastHelper(uint32_t target, uint32_t source) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(1024);
        CodeGenDetail::Append(code, "fn ", MatrixCastHelperName(target, source), "(m : ", MatrixTypeName(source),
                              ") -> ", MatrixTypeName(target), " {\n\treturn ", MatrixTypeName(target), "(");

        for (uint32_t column = 0; column < target; ++column) {
            if (column > 0) {
                code += ", ";
            }
            CodeGenDetail::Append(code, VectorTypeName(target), "(");
            for (uint32_t row = 0; row < target; ++row) {
                if (row > 0) {
                    code += ", ";
                }
                if (column < source && row < source) {
                    SR_UTILS_NS::FormatTo(code, "m[{}][{}]", column, row);
                } else {
                    code += (column == row) ? "1.0" : "0.0";
                }
            }
            code += ")";
        }

        code += ");\n}\n\n";
        return code;
    }

    SR_UTILS_NS::String GenerateMatrixDiagonalHelper(uint32_t target) {
        SR_TRACY_ZONE;
        SR_UTILS_NS::String code;
        code.reserve(512);
        CodeGenDetail::Append(code, "fn ", MatrixDiagonalHelperName(target), "(s : f32) -> ", MatrixTypeName(target),
                              " {\n\treturn ", MatrixTypeName(target), "(");

        for (uint32_t column = 0; column < target; ++column) {
            if (column > 0) {
                code += ", ";
            }
            CodeGenDetail::Append(code, VectorTypeName(target), "(");
            for (uint32_t row = 0; row < target; ++row) {
                if (row > 0) {
                    code += ", ";
                }
                code += (column == row) ? "s" : "0.0";
            }
            code += ")";
        }

        code += ");\n}\n\n";
        return code;
    }

    SR_UTILS_NS::String GenerateInverseHelper(uint32_t dimension) {
        SR_TRACY_ZONE;
        switch (dimension) {
        case 2:
            return "fn sr_inverse2(m : mat2x2<f32>) -> mat2x2<f32> {\n"
                   "\tlet invDet = 1.0 / (m[0][0] * m[1][1] - m[1][0] * m[0][1]);\n"
                   "\treturn mat2x2<f32>(vec2<f32>( m[1][1], -m[0][1]) * invDet,\n"
                   "\t                   vec2<f32>(-m[1][0],  m[0][0]) * invDet);\n"
                   "}\n\n";
        case 3:
            return "fn sr_inverse3(m : mat3x3<f32>) -> mat3x3<f32> {\n"
                   "\tlet r0 = cross(m[1], m[2]);\n"
                   "\tlet r1 = cross(m[2], m[0]);\n"
                   "\tlet r2 = cross(m[0], m[1]);\n"
                   "\tlet invDet = 1.0 / dot(m[0], r0);\n"
                   "\treturn mat3x3<f32>(vec3<f32>(r0.x, r1.x, r2.x),\n"
                   "\t                   vec3<f32>(r0.y, r1.y, r2.y),\n"
                   "\t                   vec3<f32>(r0.z, r1.z, r2.z)) * invDet;\n"
                   "}\n\n";
        case 4:
        default:
            return "fn sr_inverse4(m : mat4x4<f32>) -> mat4x4<f32> {\n"
                   "\tlet a00 = m[0][0]; let a01 = m[0][1]; let a02 = m[0][2]; let a03 = m[0][3];\n"
                   "\tlet a10 = m[1][0]; let a11 = m[1][1]; let a12 = m[1][2]; let a13 = m[1][3];\n"
                   "\tlet a20 = m[2][0]; let a21 = m[2][1]; let a22 = m[2][2]; let a23 = m[2][3];\n"
                   "\tlet a30 = m[3][0]; let a31 = m[3][1]; let a32 = m[3][2]; let a33 = m[3][3];\n"
                   "\tlet b00 = a00 * a11 - a01 * a10;\n"
                   "\tlet b01 = a00 * a12 - a02 * a10;\n"
                   "\tlet b02 = a00 * a13 - a03 * a10;\n"
                   "\tlet b03 = a01 * a12 - a02 * a11;\n"
                   "\tlet b04 = a01 * a13 - a03 * a11;\n"
                   "\tlet b05 = a02 * a13 - a03 * a12;\n"
                   "\tlet b06 = a20 * a31 - a21 * a30;\n"
                   "\tlet b07 = a20 * a32 - a22 * a30;\n"
                   "\tlet b08 = a20 * a33 - a23 * a30;\n"
                   "\tlet b09 = a21 * a32 - a22 * a31;\n"
                   "\tlet b10 = a21 * a33 - a23 * a31;\n"
                   "\tlet b11 = a22 * a33 - a23 * a32;\n"
                   "\tlet invDet = 1.0 / (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06);\n"
                   "\treturn mat4x4<f32>(\n"
                   "\t\tvec4<f32>(a11 * b11 - a12 * b10 + a13 * b09,\n"
                   "\t\t          a02 * b10 - a01 * b11 - a03 * b09,\n"
                   "\t\t          a31 * b05 - a32 * b04 + a33 * b03,\n"
                   "\t\t          a22 * b04 - a21 * b05 - a23 * b03) * invDet,\n"
                   "\t\tvec4<f32>(a12 * b08 - a10 * b11 - a13 * b07,\n"
                   "\t\t          a00 * b11 - a02 * b08 + a03 * b07,\n"
                   "\t\t          a32 * b02 - a30 * b05 - a33 * b01,\n"
                   "\t\t          a20 * b05 - a22 * b02 + a23 * b01) * invDet,\n"
                   "\t\tvec4<f32>(a10 * b10 - a11 * b08 + a13 * b06,\n"
                   "\t\t          a01 * b08 - a00 * b10 - a03 * b06,\n"
                   "\t\t          a30 * b04 - a31 * b02 + a33 * b00,\n"
                   "\t\t          a21 * b02 - a20 * b04 - a23 * b00) * invDet,\n"
                   "\t\tvec4<f32>(a11 * b07 - a10 * b09 - a12 * b06,\n"
                   "\t\t          a00 * b09 - a01 * b07 + a02 * b06,\n"
                   "\t\t          a31 * b01 - a30 * b03 - a32 * b00,\n"
                   "\t\t          a20 * b03 - a21 * b01 + a22 * b00) * invDet);\n"
                   "}\n\n";
        }
    }
} // namespace SR_SRSL_NS::WGSLDetail
