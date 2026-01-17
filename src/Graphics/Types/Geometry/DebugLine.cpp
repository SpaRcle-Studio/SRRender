//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Types/Geometry/DebugLine.h>
#include <Graphics/Material/BaseMaterial.h>

namespace SR_GTYPES_NS {
    DebugLine::DebugLine(SR_MATH_NS::FVector3 startPoint, SR_MATH_NS::FVector3 endPoint, SR_MATH_NS::FVector4 color)
        : Super()
        , m_startPoint(startPoint)
        , m_endPoint(endPoint)
        , m_color(color)
    { }

    void DebugLine::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;

        shader.SetVec3(SHADER_LINE_START_POINT, m_startPoint.Cast<float_t>());
        shader.SetVec3(SHADER_LINE_END_POINT, m_endPoint.Cast<float_t>());
        shader.SetVec4(SHADER_LINE_COLOR, SR_MATH_NS::Vector4<float_t>(
            m_color.r / 255,
            m_color.g / 255,
            m_color.b / 255,
            m_color.a / 255
        ));

        Super::UseMaterial(shader);
    }

    void DebugLine::SetEndPoint(const SR_MATH_NS::FVector3& endPoint) {
        if (m_endPoint == endPoint) {
            return;
        }
        MarkUniformsDirty();
        m_endPoint = endPoint;
    }

    void DebugLine::SetColor(const SR_MATH_NS::FVector4& color) {
        if (m_color == color) {
            return;
        }
        MarkUniformsDirty();
        m_color = color;
    }

    void DebugLine::SetStartPoint(const SR_MATH_NS::FVector3& startPoint) {
        if (m_startPoint == startPoint) {
            return;
        }
        MarkUniformsDirty();
        m_startPoint = startPoint;
    }
}