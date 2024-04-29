//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Types/Geometry/DebugLine.h>

namespace SR_GTYPES_NS {
    DebugLine::DebugLine()
        : Super(MeshType::Line)
    { }

    DebugLine::DebugLine(SR_MATH_NS::FVector3 startPoint, SR_MATH_NS::FVector3 endPoint, SR_MATH_NS::FVector4 color)
        : Super(MeshType::Line)
        , m_startPoint(startPoint)
        , m_endPoint(endPoint)
        , m_color(color)
    { }

    void DebugLine::Draw() {
        SR_TRACY_ZONE;

        auto&& pShader = GetRenderContext()->GetCurrentShader();

        if (!pShader) {
            return;
        }

        if ((!IsCalculated() && !Calculate()) || m_hasErrors) {
            return;
        }

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_dirtyMaterial = false;

            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            m_material->UseSamplers();
        }

        m_uboManager.BindUBO(m_virtualUBO);

        if (m_descriptorManager.Bind(m_virtualDescriptor) != DescriptorManager::BindResult::Failed) {
            m_pipeline->DrawIndices(2);
        }
    }

    void DebugLine::UseMaterial() {
        SR_TRACY_ZONE;

        if (auto&& pShader = GetShader()) {
            pShader->SetVec3(SHADER_LINE_START_POINT, m_startPoint.Cast<float_t>());
            pShader->SetVec3(SHADER_LINE_END_POINT, m_endPoint.Cast<float_t>());
            pShader->SetVec4(SHADER_LINE_COLOR, SR_MATH_NS::Vector4<float_t>(
                m_color.r / 255,
                m_color.g / 255,
                m_color.b / 255,
                m_color.a / 255
            ));
        }

        Super::UseMaterial();
    }

    void DebugLine::SetEndPoint(const SR_MATH_NS::FVector3& endPoint) {
        m_endPoint = endPoint;
    }

    void DebugLine::SetColor(const SR_MATH_NS::FVector4& color) {
        m_color = color;
    }

    void DebugLine::SetStartPoint(const SR_MATH_NS::FVector3& startPoint) {
        m_startPoint = startPoint;
    }
}