//
// Created by Monika on 14.01.2023.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/GUI/Pin.h>
#include <Graphics/GUI/Node.h>
#include <Graphics/GUI/NodeBuilder.h>

namespace SR_GRAPH_GUI_NS {
    NodeBuilder::NodeBuilder(SR_GTYPES_NS::Texture *pTexture)
        : m_texture(pTexture)
    {
        if (m_texture) {
            m_texture->AddUsePoint();
        }
    }

    NodeBuilder::~NodeBuilder() {
        if (m_texture) {
            m_texture->RemoveUsePoint();
        }
    }

    void NodeBuilder::Begin(Node* pNode) {

    }

    void NodeBuilder::End() {

    }

    //void NodeBuilder::Header(const ImVec4 &color) {
    //    m_headerColor = ImColor(color);
    //    SetStage(Stage::Header);
    //}

    void NodeBuilder::EndHeader() {
        SetStage(Stage::Content);
    }

    //ImTextureID NodeBuilder::GetTextureId() const {
    //    if (!m_texture) {
    //        return nullptr;
    //    }

    //    auto&& id = m_texture->GetId();
    //    if (id == SR_ID_INVALID) {
    //        return nullptr;
    //    }

    //    if (auto&& pPipeline = m_texture->GetPipeline()) {
    //        return pPipeline->GetOverlayTextureDescriptorSet(id, OverlayType::ImGui);
    //    }

    //    return nullptr;
    //}

    void NodeBuilder::Input(Pin *pPin) {

    }

    void NodeBuilder::EndInput() {

    }

    void NodeBuilder::Middle() {
        if (m_currentStage == Stage::Begin)
            SetStage(Stage::Content);

        SetStage(Stage::Middle);
    }

    void NodeBuilder::Output(Pin *pPin) {

    }

    void NodeBuilder::EndOutput() {

    }

    bool NodeBuilder::SetStage(NodeBuilder::Stage stage) {

        return true;
    }
}

