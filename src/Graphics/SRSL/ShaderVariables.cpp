//
// Created by Monika on 26.01.2024.
//

#include <Graphics/SRSL/ShaderVariables.h>

namespace SR_SRSL_NS {
    std::string ShaderRenderPassTypeToString(ShaderRenderPassType type) {
        return "RenderPassType_" + SR_UTILS_NS::EnumReflector::ToStringAtom(type).ToString();
    }
}
