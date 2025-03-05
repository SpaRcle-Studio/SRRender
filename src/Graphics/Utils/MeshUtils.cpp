//
// Created by Monika on 20.03.2023.
//

#include <Graphics/Utils/MeshUtils.h>

#include <Graphics/Types/Geometry/Mesh3D.h>
#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Graphics/Types/Geometry/SkinnedMesh.h>

#include <Graphics/Types/Geometry/Sprite.h>

namespace SR_GRAPH_NS {
    SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Mesh> CreateMeshByType(MeshType type) {
        switch (type) {
        case MeshType::Static:
                return SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::Mesh3D>().StaticCast<SR_GTYPES_NS::Mesh>();
            case MeshType::Sprite:
                return SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::Sprite>().StaticCast<SR_GTYPES_NS::Mesh>();
            case MeshType::Skinned:
                return SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::SkinnedMesh>().StaticCast<SR_GTYPES_NS::Mesh>();
            case MeshType::Procedural:
                return SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::ProceduralMesh>().StaticCast<SR_GTYPES_NS::Mesh>();
            case MeshType::Wireframe:
                return SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::DebugWireframeMesh>().StaticCast<SR_GTYPES_NS::Mesh>();
            case MeshType::Unknown:
            default:
                break;
        }

        SRHalt("Unknown mesh type!");

        return nullptr;
    }
}
