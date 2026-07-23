//
// Created by Monika on 19.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_CAMERA_RENDER_PARAMETERS_H
#define SR_ENGINE_GRAPHICS_CAMERA_RENDER_PARAMETERS_H

#include <Graphics/Settings/Quality.h>

#include <Utils/Serialization/Serializable.h>
#include <Utils/Types/Optional.h>

namespace SR_GRAPH_NS {
    struct CameraRenderParameters : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::Set<SR_UTILS_NS::StringAtom> includeLayers;
        /// @property
        SR_UTILS_NS::Set<SR_UTILS_NS::StringAtom> excludeLayers;
        /// @property
        SR_UTILS_NS::Optional<bool> postProcess;
        /// @property
        SR_UTILS_NS::Optional<bool> hdr;
        /// @property
        SR_UTILS_NS::Optional<bool> autoExposure;
        /// @property
        SR_UTILS_NS::Optional<Quality> shadowsQuality;
        /// @property
        SR_UTILS_NS::Optional<Quality> colorBufferQuality;
        /// @property
        SR_UTILS_NS::Optional<bool> multisampling;
        /// @property
        SR_UTILS_NS::Optional<SR_MATH_NS::IVector2> screenSize;
        /// @property
        SR_UTILS_NS::Optional<SR_MATH_NS::FVector2> screenScale;
        /// @property
        SR_UTILS_NS::Optional<Quality> SSAO;

    };
}

#endif //SR_ENGINE_GRAPHICS_CAMERA_RENDER_PARAMETERS_H
