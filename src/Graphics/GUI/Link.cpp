//
// Created by Monika on 18.01.2022.
//

#include <Graphics/GUI/Link.h>
#include <Graphics/GUI/Pin.h>
#include <Graphics/GUI/ImmediateGUI.h>

namespace SR_GRAPH_NS::GUI {
    uintptr_t Link::GetId() const {
        return reinterpret_cast<uintptr_t>(this);
    }

    void Link::DrawBezier() const {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        if (m_startPin && m_endPin) {
            SR_GRAPH_GUI_NS::Immediate::Link(GetId(), m_startPin->GetId(), m_endPin->GetId());
        }
    #endif
    }

    Link::Link(Pin* start, Pin* end) {
        SetStart(start);
        SetEnd(end);
    }

    void Link::SetStart(Pin* pPin) {
        if (m_startPin) {
            m_startPin->RemoveLink(this);
        }

        if ((m_startPin = pPin)) {
            pPin->AddLink(this);
        }
    }

    void Link::SetEnd(Pin* pPin) {
        if (m_endPin) {
            m_endPin->RemoveLink(this);
        }

        if ((m_endPin = pPin)) {
            pPin->AddLink(this);
        }
    }

    Link::~Link() {
        if (m_startPin) {
            m_startPin->RemoveLink(this);
            m_startPin = nullptr;
        }

        if (m_endPin) {
            m_endPin->RemoveLink(this);
            m_endPin = nullptr;
        }
    }

    bool Link::IsLinked(Pin* pPin) const {
        return pPin && (m_startPin == pPin || m_endPin == pPin);
    }

    void Link::Broke(Pin* pFrom) {
        if (pFrom != m_startPin && m_startPin) {
            m_startPin->RemoveLink(this);
        }

        if (pFrom != m_endPin && m_endPin) {
            m_endPin->RemoveLink(this);
        }

        m_startPin = nullptr;
        m_endPin = nullptr;
    }
}