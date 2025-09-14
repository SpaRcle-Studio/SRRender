//
// Created by Monika on 22.07.2022.
//

#ifndef SR_ENGINE_GROUPPASS_H
#define SR_ENGINE_GROUPPASS_H

#include <Graphics/Pass/BasePass.h>

namespace SR_GRAPH_NS {
    class GroupPass : public BasePass {
        using Super = BasePass;
        SR_CLASS()
    public:
        ~GroupPass() override;

    public:
        bool Init() override;
        void DeInit() override;

        bool Overlay() override;

        void Prepare() override;

        bool PreRender() override;
        bool Render() override;
        bool PostRender() override;

        void Update() override;
        void PostUpdate() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void OnMultisampleChanged() override;

        void SetRenderTechnique(IRenderTechnique* pRenderTechnique) override;
        void SetParent(BasePass* pParent) override;

        const std::vector<BasePass::Ptr>& GetPasses() const { return m_passes; }
        std::vector<BasePass::Ptr>& GetPasses() { return m_passes; }

        void SetPasses(const std::vector<BasePass::Ptr>& passes) { m_passes = passes; }

        void ForEachPass(const std::function<void(BasePass&)>& func) override;

        SR_NODISCARD BasePass* FindPass(SR_UTILS_NS::StringAtom name) override;

    protected:
        /// @property
        std::vector<BasePass::Ptr> m_passes;

    };
}

#endif //SR_ENGINE_GROUPPASS_H
