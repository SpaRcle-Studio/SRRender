//
// Created by Monika on 19.12.2025.
//

#ifndef SR_ENGINE_FRAME_BUFFER_ACCESS_MODE_H
#define SR_ENGINE_FRAME_BUFFER_ACCESS_MODE_H

#include <Graphics/stdInclude.h>

namespace SR_GRAPH_NS {
    enum class FrameBufferAccessMode : uint8_t {
        Read,
        Write
    };
}

#endif //SR_ENGINE_FRAME_BUFFER_ACCESS_MODE_H
