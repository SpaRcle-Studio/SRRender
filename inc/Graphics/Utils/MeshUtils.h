//
// Created by Monika on 20.03.2023.
//

#ifndef SR_ENGINE_MESH_UTILS_H
#define SR_ENGINE_MESH_UTILS_H

#include <Graphics/stdInclude.h>

#include <Utils/Types/SharedPtr.h>

namespace SR_GTYPES_NS {
    class IRenderComponent;
    class Shader;
}

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(FrustumCullingType, uint8_t,
        None = 0,
        Sphere,
        AABB,
        OBB,
        DOP8,
        ConvexHull
    );

    class RenderScene;
    class BaseMaterial;
    class RenderQueue;

    struct RenderObjectRegistrationInfoInternal {
        uint32_t poolId = static_cast<uint32_t>(SR_ID_INVALID);
        BaseMaterial* pMaterial = nullptr;
        SR_UTILS_NS::StringAtom layer;
        std::optional<int32_t> VBO;
        std::optional<int64_t> priority;
    };

    struct RenderObjectRegistrationInfo {
        SR_GTYPES_NS::IRenderComponent* pObject = nullptr;
        RenderObjectRegistrationInfoInternal internal;
    };

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SR_SUPPORTED_MESH_FORMATS = "obj,pmx,fbx,blend,stl,dae,3ds";
    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SR_SUPPORTED_FONT_FORMATS = "ttf";

    void DrawRenderObject(SR_GTYPES_NS::IRenderComponent* pObject,
        uint32_t indices,
        int32_t& ubo,
        int32_t& descriptor,
        bool& dirtyMaterial,
        bool& hasErrors
   );
}

#endif //SR_ENGINE_MESH_UTILS_H
