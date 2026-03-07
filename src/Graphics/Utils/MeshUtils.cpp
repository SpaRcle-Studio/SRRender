//
// Created by Monika on 20.03.2023.
//

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Types/Geometry/Mesh3D.h>
#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Graphics/Types/Geometry/SkinnedMesh.h>
#include <Graphics/Types/Geometry/Sprite.h>
#include <Graphics/Types/Geometry/ProceduralMesh.h>

#include <Utils/TypeTraits/Factory.h>

namespace SR_GRAPH_NS {
    SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Mesh> CreateMeshByType(MeshType type) {
        switch (type) {
        case MeshType::Static: return SR_UTILS_NS::StaticPointerCast<SR_GTYPES_NS::Mesh>(SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::Mesh3D>());
        case MeshType::Sprite: return SR_UTILS_NS::StaticPointerCast<SR_GTYPES_NS::Mesh>(SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::Sprite>());
        case MeshType::Skinned: return SR_UTILS_NS::StaticPointerCast<SR_GTYPES_NS::Mesh>(SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::SkinnedMesh>());
        case MeshType::Procedural: return SR_UTILS_NS::StaticPointerCast<SR_GTYPES_NS::Mesh>(SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::ProceduralMesh>());
        case MeshType::Wireframe: return SR_UTILS_NS::StaticPointerCast<SR_GTYPES_NS::Mesh>(SR_UTILS_NS::Factory::Instance().Create<SR_GTYPES_NS::DebugWireframeMesh>());
            case MeshType::Unknown:
            default:
                break;
        }

        SRHalt("Unknown mesh type!");

        return nullptr;
    }
}
