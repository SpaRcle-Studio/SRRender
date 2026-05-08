//
// Created by Nariman on 07.05.2026.
//

#include <Graphics/Pass/BlurPass.h>

#include <Codegen/BlurPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool BlurPass::Init() {
        SetShader("Engine/Shaders/BlurPass/BlurPass.srsl");
        return Super::Init();
    }
}