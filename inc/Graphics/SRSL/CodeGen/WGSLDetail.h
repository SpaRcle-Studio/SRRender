//
// Created by Monika on 12.11.2026.
//

#ifndef SR_ENGINE_SRSL_WGSL_DETAIL_H
#define SR_ENGINE_SRSL_WGSL_DETAIL_H

#include <Graphics/Pipeline/ShaderUtils.h>

namespace SR_SRSL_NS::WGSLDetail {
    SR_UTILS_NS::StringView GenerateType(SR_UTILS_NS::StringView type);
    SR_UTILS_NS::StringView VertexAttributeFormatToString(SR_UTILS_NS::VertexAttributeFormat format, uint8_t count);
    SR_UTILS_NS::StringView InverseHelperName(uint32_t dimension);
    SR_UTILS_NS::String SafeIdentifier(SR_UTILS_NS::StringView token);
    SR_UTILS_NS::String ToWGSLArrayType(SR_UTILS_NS::StringView baseType, const SR_UTILS_NS::Vector<uint64_t>& dims);
    SR_UTILS_NS::StringView MatrixColumnType(SR_UTILS_NS::StringView wgslType);
    SR_UTILS_NS::StringView MatrixCastHelperName(uint32_t target, uint32_t source);
    SR_UTILS_NS::StringView MatrixDiagonalHelperName(uint32_t target);
    SR_UTILS_NS::StringView MatrixTypeName(uint32_t dimension);
    SR_UTILS_NS::StringView VectorTypeName(uint32_t dimension);
    SR_UTILS_NS::String GenerateMatrixCastHelper(uint32_t target, uint32_t source);
    SR_UTILS_NS::String GenerateMatrixDiagonalHelper(uint32_t target);
    SR_UTILS_NS::String GenerateInverseHelper(uint32_t dimension);
    SR_UTILS_NS::StringView ReplaceToken(SR_UTILS_NS::StringView token);

    void AppendIdentifier(SR_UTILS_NS::String& code, SR_UTILS_NS::StringView token);
    void AppendToken(SR_UTILS_NS::String& code, SR_UTILS_NS::StringView token);

    uint32_t SquareMatrixDimension(SR_UTILS_NS::StringView wgslType);
    uint32_t MatrixColumns(SR_UTILS_NS::StringView wgslType);

    bool IsWGSLReservedKeyword(SR_UTILS_NS::StringView token);
    bool IsCompoundAssignment(SR_UTILS_NS::StringView token);
    bool IsScalarType(SR_UTILS_NS::StringView wgslType);
    bool NeedsFlatInterpolation(SR_UTILS_NS::StringView wgslType);
    bool IsNumericLiteral(SR_UTILS_NS::StringView token);

} // namespace SR_SRSL_NS::WGSLDetail

#endif
