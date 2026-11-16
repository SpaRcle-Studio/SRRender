//
// Created by Monika on 27.01.2026.
//

#ifndef SR_ENGINE_COMMON_UI_RECT_MASK_2D_H
#define SR_ENGINE_COMMON_UI_RECT_MASK_2D_H

#include <Graphics/Render/RenderScene.h>
#include <Graphics/UI/Canvas.h>

#include <Utils/UI/UIComponent.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/UI/MaskInfo.h>

namespace SR_GRAPH_NS::UI {
    /// @category(UI)
    class RectMask2D final : public SR_UTILS_NS::UIComponent, public IFindCanvasOwner {
        SR_CLASS()
    public:
        RectMask2D() = default;

    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        void OnEnable() override;
        void OnDisable() override;
        void OnDetached() override;
        void OnMatrixDirty() override;
        void AfterLayout() override;

    private:
        void UpdateClipping(bool enable);

    private:
        SR_UTILS_NS::UI::MaskInfo m_maskInfo;
        SR_GRAPH_NS::RenderScene::Ptr m_renderScene;

    };
}

#endif //SR_ENGINE_COMMON_UI_RECT_MASK_2D_H
