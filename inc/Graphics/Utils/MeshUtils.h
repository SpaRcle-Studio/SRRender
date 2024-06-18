//
// Created by Monika on 20.03.2023.
//

#ifndef SR_ENGINE_MESH_UTILS_H
#define SR_ENGINE_MESH_UTILS_H

#include <Graphics/Utils/MeshTypes.h>

namespace SR_GTYPES_NS {
    class Mesh;
    class Shader;
    class IMeshComponent;
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
    class MeshRenderStage;
    class BaseMaterial;
    class RenderQueue;

    struct MeshRegistrationInfo {
        uint32_t poolId = static_cast<uint32_t>(SR_ID_INVALID);
        SR_GTYPES_NS::Mesh* pMesh = nullptr;
        BaseMaterial* pMaterial = nullptr;
        SR_UTILS_NS::StringAtom layer;
        std::optional<int32_t> VBO;
        std::optional<int64_t> priority;
        SR_GRAPH_NS::RenderScene* pScene = nullptr;
    };

    SR_INLINE_STATIC SR_UTILS_NS::StringAtom SR_SUPPORTED_MESH_FORMATS = "obj,pmx,fbx,blend,stl,dae,3ds";

    SR_GTYPES_NS::Mesh* CreateMeshByType(MeshType type);
    SR_GTYPES_NS::IMeshComponent* CreateMeshComponentByType(MeshType type);
    uint16_t RoundBonesCount(uint16_t count);
}

#endif //SR_ENGINE_MESH_UTILS_H
