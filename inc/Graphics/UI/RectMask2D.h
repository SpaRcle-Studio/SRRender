//
// Created by Monika on 27.01.2026.
//

#ifndef SR_ENGINE_COMMON_UI_RECT_MASK_2D_H
#define SR_ENGINE_COMMON_UI_RECT_MASK_2D_H

#include <Graphics/Render/RenderScene.h>
#include <Graphics/UI/Canvas.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/UI/MaskInfo.h>

namespace SR_GRAPH_NS::UI {
    /// @category(UI)
    class RectMask2D final : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
    public:
        RectMask2D() = default;

    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        void OnEnable() override;
        void OnDisable() override;
        void OnDetached() override;
        void OnMatrixDirty() override;

    private:
        void UpdateClipping(bool enable);
        SR_NODISCARD SR_GRAPH_NS::UI::Canvas* FindCanvas();

    private:
        SR_UTILS_NS::EntityRef<SR_GRAPH_NS::UI::Canvas> m_canvas;
        SR_UTILS_NS::UI::MaskInfo m_maskInfo;
        SR_GRAPH_NS::RenderScene::Ptr m_renderScene;

    };
}

#endif //SR_ENGINE_COMMON_UI_RECT_MASK_2D_H
