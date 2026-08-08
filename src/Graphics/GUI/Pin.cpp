//
// Created by Monika on 18.01.2022.
//

#include <Graphics/GUI/Pin.h>
#include <Graphics/GUI/Utils.h>
#include <Graphics/GUI/Link.h>
#include <Graphics/GUI/Node.h>

#include <Utils/Platform/Platform.h>

namespace SR_GRAPH_GUI_NS {
    Pin::Pin()
        : Pin(std::string(), PinKind::None, nullptr)
    { }

    Pin::Pin(const std::string& name)
        : Pin(name, PinKind::None, nullptr)
    { }

    Pin::Pin(const std::string& name, PinKind kind)
        : Pin(name, kind, nullptr)
    { }

    Pin::Pin(const std::string& name, Pin::DataTypePtr pData)
        : Pin(name, PinKind::None, pData)
    { }

    Pin::Pin(std::string name, PinKind kind, DataTypePtr pDataType)
        : m_dataType(pDataType)
        , m_name(std::move(name))
        , m_kind(kind)
    { }

    Pin::~Pin() {
        /// Будет удалено в управляющей ноде
        /// SR_SAFE_DELETE_PTR(m_dataType);

        for (auto&& pLink : m_links) {
            pLink->Broke(this);
        }

        m_node = nullptr;
    }

    void Pin::SetNode(Node* node) {
        if (m_node == node)
            return;

        m_node = node;
    }

    float_t Pin::GetWidth() const {
        return static_cast<float_t>(m_name.size()); //TODO: crash ImGui::CalcTextSize(m_name.c_str()).x;
    }

    void Pin::AddLink(Link *link) {
    #if defined(SR_DEBUG)
        if (m_links.count(link) == 1) {
            SRAssert(false);
            return;
        }
    #endif

        m_links.insert(link);
    }

    void Pin::RemoveLink(Link *link) {
    #if defined(SR_DEBUG)
        if (m_links.count(link) == 0) {
            SRAssert(false);
            return;
        }
    #endif

        m_links.erase(link);
    }

    bool Pin::IsLinked(Pin* pPin) const {
        for (auto&& pLink : m_links) {
            if (pLink->IsLinked(pPin)) {
                return true;
            }
        }

        return false;
    }

    bool Pin::CanLink() const {
        if (!GetNode()) {
            return false;
        }

        /*if (IsLinked()) {
            if (GetType() == PinType::Flow && GetKind() == PinKind::Output) {
                return false;
            }
            else if (GetType() != PinType::Flow && GetKind() == PinKind::Input) {
                return false;
            }
            else if (GetType() == PinType::Flow && GetKind() == PinKind::Input) {
                auto&& pSynchronize = dynamic_cast<SR_SRLM_NS::SynchronizeNode*>(GetNode()->GetLogicalNode());
                if (pSynchronize) {
                    return false;
                }
            }
        }

        if (GetNode()->IsConnector() && IsLinked() && GetType() != SR_SRLM_NS::DataTypeClass::Flow) {
            if (GetKind() == PinKind::Input) {
                return false;
            }
        }*/

        return true;
    }

    bool Pin::IsLinked() const {
        return !m_links.empty();
    }

    void Pin::Begin(PinKind kind) const {
        switch (kind) {
            case PinKind::None:
                break;
            case PinKind::Output:
                break;
            case PinKind::Input:
                break;
            default:
                break;
        }
    }

    void Pin::End() const {
    }

    void Pin::DrawPinIcon(bool connected, uint32_t alpha) {

    }

    void Pin::PostDrawOption() {
        if (m_editEnum) {
            SR_GRAPH_GUI_NS::Immediate::OpenPopup(SR_FORMAT_C("pin_enum_popup{}", (void*)this));
            m_editEnum = false;
        }

    }

    void Pin::DrawOption() {
    }

    uint32_t Pin::GetIndex() const {
        if (auto&& index = m_node->GetPinIndex(this); index >= 0) {
            return index;
        }

        SRHalt("Invalid pin!");
        return SR_UINT32_MAX;
    }
}