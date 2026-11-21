//
// Created by Monika on 26.07.2025.
//

#include <Graphics/Pipeline/FrameBufferFeatures.h>

#include <Codegen/FrameBufferFeatures.generated.hpp>

namespace SR_GRAPH_NS {
    bool FrameBufferFeatures::operator==(const FrameBufferFeatures& other) const {
        return depthLoad == other.depthLoad &&
            colorLoad == other.colorLoad &&
            depthTransferSrc == other.depthTransferSrc &&
            colorTransferSrc == other.colorTransferSrc &&
            depthTransferDst == other.depthTransferDst &&
            colorTransferDst == other.colorTransferDst &&
            depthShaderRead == other.depthShaderRead &&
            colorShaderRead == other.colorShaderRead;
    }

    bool FrameBufferFeatures::operator!=(const FrameBufferFeatures& other) const {
        return !(*this == other);
    }
}
