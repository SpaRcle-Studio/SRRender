//
// Created by Monika on 20.03.2023.
//

#include <Graphics/Utils/MeshUtils.h>

#include <Graphics/Types/Geometry/Mesh3D.h>
#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Graphics/Types/Geometry/SkinnedMesh.h>

#include <Graphics/Types/Geometry/Sprite.h>

namespace SR_GRAPH_NS {
    SR_GTYPES_NS::Mesh* CreateMeshByType(MeshType type) {
        auto&& manager = SR_UTILS_NS::ComponentManager::Instance();

        switch (type) {
            case MeshType::Static:
                return manager.CreateComponent<SR_GTYPES_NS::Mesh3D>();
            case MeshType::Sprite:
                return manager.CreateComponent<SR_GTYPES_NS::Sprite>();
            case MeshType::Skinned:
                return manager.CreateComponent<SR_GTYPES_NS::SkinnedMesh>();
            case MeshType::Procedural:
                return manager.CreateComponent<SR_GTYPES_NS::ProceduralMesh>();
            case MeshType::Wireframe:
                return new SR_GTYPES_NS::DebugWireframeMesh();
            case MeshType::Unknown:
            default:
                break;
        }

        SRHalt("Unknown mesh type!");

        return nullptr;
    }

    SR_GTYPES_NS::IMeshComponent* CreateMeshComponentByType(MeshType type) {
        switch (type) {
            case MeshType::Static:
            case MeshType::Sprite:
            case MeshType::Skinned:
            case MeshType::Procedural: {
                if (auto&& pMeshComponent = dynamic_cast<SR_GTYPES_NS::IMeshComponent*>(CreateMeshByType(type))) {
                    return pMeshComponent;
                }
                SRHalt("Mesh is not a component! Memory leak...");
                return nullptr;
            }
            default:
                break;
        }

        SRHalt("Unknown mesh type!");

        return nullptr;
    }

    uint16_t RoundBonesCount(uint16_t count) {
        if (count == 0) {
            SRHalt("Invalid count!");
            return 0;
        }

        if (count > 256) {
            return 384;
        }
        else if (count > SR_HUMANOID_MAX_BONES) {
            return 256;
        }

        return SR_HUMANOID_MAX_BONES;
    }
}
