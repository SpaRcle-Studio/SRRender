//
// Created by Monika on 07.02.2024.
//

#ifndef SR_ENGINE_FRAME_BUFFER_FEATURES_H
#define SR_ENGINE_FRAME_BUFFER_FEATURES_H

#include <Graphics/macros.h>

#include <Utils/Serialization/Serializable.h>

namespace SR_GRAPH_NS {
    struct FrameBufferFeatures : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        bool depthLoad = false;
        /// @property
        bool colorLoad = false;
        /// @property
        bool depthTransferSrc = false;
        /// @property
        bool colorTransferSrc = false;
        /// @property
        bool depthTransferDst = false;
        /// @property
        bool colorTransferDst = false;
        /// @property
        bool depthShaderRead = false;
        /// @property
        bool colorShaderRead = true;
    };
}

#endif //SR_ENGINE_FRAME_BUFFER_FEATURES_H
