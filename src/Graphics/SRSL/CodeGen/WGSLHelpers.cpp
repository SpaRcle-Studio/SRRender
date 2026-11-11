//
// Created by Monika on 12.11.2026.
//

#include <Graphics/SRSL/CodeGen/WGSLGenerator.h>
#include <Graphics/SRSL/CodeGen/WGSLDetail.h>

namespace SR_SRSL_NS {
    SR_UTILS_NS::String WGSLGenerator::GenerateHelperFunctions() const {
        SR_TRACY_ZONE;
        bool used = false;
        for (uint32_t i = 0; i < 3; ++i) {
            used |= m_inverseHelpers[i] || m_diagonalHelpers[i];
            for (uint32_t j = 0; j < 3; ++j) { used |= m_matrixCastHelpers[i][j]; }
        }
        if (!used) { return { }; }

        SR_UTILS_NS::String code;
        code.reserve(2048);
        code += "/// Вспомогательные функции: в WGSL нет аналогов этих встроенных функций GLSL\n";
        for (uint32_t dimension = 2; dimension <= 4; ++dimension) {
            if (m_inverseHelpers[dimension - 2]) { code += WGSLDetail::GenerateInverseHelper(dimension); }
        }
        for (uint32_t target = 2; target <= 4; ++target) {
            for (uint32_t source = 2; source <= 4; ++source) {
                if (m_matrixCastHelpers[target - 2][source - 2]) {
                    code += WGSLDetail::GenerateMatrixCastHelper(target, source);
                }
            }
        }
        for (uint32_t target = 2; target <= 4; ++target) {
            if (m_diagonalHelpers[target - 2]) { code += WGSLDetail::GenerateMatrixDiagonalHelper(target); }
        }
        return code;
    }
}
