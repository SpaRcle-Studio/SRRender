//
// Created by Monika on 13.10.2026.
//

#include <Graphics/Pass/SceneDistancePass.h>

#include <Codegen/SceneDistancePass.generated.hpp>

namespace SR_GRAPH_NS {
    bool SceneDistancePass::Init() {
        SetShader("Engine/Shaders/Common/scene-distance.srsl");
        return Super::Init();
    }
}
