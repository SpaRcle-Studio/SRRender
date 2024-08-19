//
// Created by Monika on 15.08.2024.
//

#ifndef SR_ENGINE_HTML_DRAWER_PASS_H
#define SR_ENGINE_HTML_DRAWER_PASS_H

#include <Graphics/Pass/BasePass.h>

namespace SR_GRAPH_NS {
    class HTMLDrawerPass : public BasePass {
        SR_REGISTER_LOGICAL_NODE(HTMLDrawerPass, HTML Drawer Pass, { "Passes" })
        using Super = BasePass;
    public:
        bool Load(const SR_XML_NS::Node& passNode) override;
        bool Render() override;
        void Update() override;
        bool Init() override;

    };
}

#endif //SR_ENGINE_HTML_DRAWER_PASS_H