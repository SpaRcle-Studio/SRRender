//
// Created by Monika on 15.08.2024.
//

#include <Graphics/Pass/HTMLDrawerPass.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(HTMLDrawerPass)

    bool HTMLDrawerPass::Load(const SR_XML_NS::Node& passNode) {
        SR_TRACY_ZONE;

        return BasePass::Load(passNode);
    }

    bool HTMLDrawerPass::Render() {
        return true;
    }

    void HTMLDrawerPass::Update() {
        BasePass::Update();
    }

    bool HTMLDrawerPass::Init() {
        const bool result = BasePass::Init();

        return result;
    }
}