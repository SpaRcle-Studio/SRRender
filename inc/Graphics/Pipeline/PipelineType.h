//
// Created by Nikita on 01.04.2021.
//

#ifndef SR_ENGINE_PIPELINE_TYPE_H
#define SR_ENGINE_PIPELINE_TYPE_H

#include <Utils/Common/Enumerations.h>
#include <Utils/Debug.h>
#include <Utils/Math/Matrix4x4.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(PipelineType, uint8_t,
        Unknown, OpenGL, Vulkan, DirectX9, DirectX10, DirectX11, DirectX12
    );
}

#endif //SR_ENGINE_PIPELINE_TYPE_H
