//
// Created by Monika on 18.01.2022.
//

#ifndef SR_ENGINE_LINK_H
#define SR_ENGINE_LINK_H

#include <Graphics/GUI/Icons.h>

#include <Utils/Common/Enumerations.h>

namespace SR_GRAPH_NS::GUI {
    class Pin;

    class Link : private SR_UTILS_NS::NonCopyable {
    public:
        Link() = default;
        Link(Pin* start, Pin* end);
        ~Link() override;

    public:
        SR_NODISCARD uintptr_t GetId() const;
        SR_NODISCARD bool IsLinked(Pin* pPin) const;
        SR_NODISCARD bool IsLinked() const { return m_endPin && m_startPin; }
        SR_NODISCARD Pin* GetStart() const { return m_startPin; }
        SR_NODISCARD Pin* GetEnd() const { return m_endPin; }

        void SetStart(Pin* pPin);
        void SetEnd(Pin* pPin);

        void DrawBezier() const;
        void Broke(Pin* pFrom);

        SR_NODISCARD void* GetUserData() const noexcept { return m_userData; }
        template<typename T> SR_NODISCARD T* GetUserData() const noexcept { return reinterpret_cast<T*>(m_userData); }
        void SetUserData(void* pUserData) noexcept { m_userData = pUserData; }

    private:
        Pin* m_startPin = nullptr;
        Pin* m_endPin = nullptr;
        void* m_userData = nullptr;

    };
}

#endif //SR_ENGINE_LINK_H
