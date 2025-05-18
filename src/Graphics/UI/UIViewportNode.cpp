//
// Created by Monika on 18.05.2025.
//

#include <Graphics/UI/UIViewportNode.h>

#include <Codegen/UIViewportNode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    void UIViewportNode::Layout(const SR_MATH_NS::FRect&) {
        auto&& pMainCamera = GetCamera();
        if (!pMainCamera) {
            return;
        }

        Super::Layout(SR_MATH_NS::FRect(0.f, 0.f, pMainCamera->GetSize().CastToFloat()));
    }
}