//
// Created by Monika on 22.07.2022.
//

#include <Graphics/Pass/GroupPass.h>

#include <Codegen/GroupPass.generated.hpp>

namespace SR_GRAPH_NS {
    GroupPass::~GroupPass() {
        for (auto&& pPass : m_passes) {
            pPass.AutoFree();
        }
        m_passes.clear();
    }

    bool GroupPass::Init() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            pPass->Init();
        }

        return BasePass::Init();
    }

    void GroupPass::DeInit() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            pPass->DeInit();
        }

        BasePass::DeInit();
    }

    void GroupPass::Prepare() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            pPass->Prepare();
        }

        BasePass::Prepare();
    }

    bool GroupPass::PreRender() {
        bool hasDrawData = false;

        for (auto&& pPass : m_passes) {
            if (pPass->HasPreRender()) {
                SR_TRACY_ZONE_S(pPass->GetPassName().data());
                pPass->Bind();
                hasDrawData |= pPass->PreRender();
            }
        }

        return hasDrawData;
    }

    bool GroupPass::Render() {
        bool hasDrawData = false;

        for (auto&& pPass : m_passes) {
            if (pPass->HasRender()) {
                SR_TRACY_ZONE_S(pPass->GetPassName().data());
                pPass->Bind();
                hasDrawData |= pPass->Render();
            }
        }

        return hasDrawData;
    }

    bool GroupPass::PostRender() {
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            if (pPass->HasPostRender()) {
                SR_TRACY_ZONE_S(pPass->GetPassName().data());
                pPass->Bind();
                hasDrawData |= pPass->PostRender();
            }
        }
        return hasDrawData;
    }

    void GroupPass::Update() {
        for (auto&& pPass : m_passes) {
            if (pPass->HasUpdate()) {
                SR_TRACY_ZONE_S(pPass->GetPassName().data());
                pPass->Bind();
                pPass->Update();
            }
        }
        Super::Update();
    }

    bool GroupPass::Overlay() {
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            SR_TRACY_ZONE_S(pPass->GetPassName().data());
            hasDrawData |= pPass->Overlay();
        }
        return hasDrawData;
    }

    void GroupPass::OnResize(const SR_MATH_NS::UVector2 &size) {
        for (auto&& pPass : m_passes) {
            SR_TRACY_ZONE_S(pPass->GetPassName().data());
            pPass->OnResize(size);
        }
        Super::OnResize(size);
    }

    void GroupPass::OnMultisampleChanged() {
        for (auto&& pPass : m_passes) {
            SR_TRACY_ZONE_S(pPass->GetPassName().data());
            pPass->OnMultisampleChanged();
        }
        Super::OnMultisampleChanged();
    }

    void GroupPass::SetRenderTechnique(IRenderTechnique* pRenderTechnique) {
        for (auto&& pPass : m_passes) {
            pPass->SetRenderTechnique(pRenderTechnique);
        }
        Super::SetRenderTechnique(pRenderTechnique);
    }

    void GroupPass::SetParent(BasePass* pParent) {
        for (auto&& pPass : m_passes) {
            pPass->SetParent(this);
        }
        Super::SetParent(pParent);
    }

    void GroupPass::PostUpdate() {
        for (auto&& pPass : m_passes) {
            SR_TRACY_ZONE_S(pPass->GetPassName().data());
            pPass->PostUpdate();
        }
        Super::PostUpdate();
    }

    void GroupPass::ForEachPass(const std::function<void(BasePass&)>& func) {
        for (auto&& pPass : m_passes) {
            pPass->ForEachPass(func);
        }
        Super::ForEachPass(func);
    }

    BasePass* GroupPass::FindPass(SR_UTILS_NS::StringAtom name) {
        for (auto&& pPass : m_passes) {
            if (auto&& pFound = pPass->FindPass(name)) {
                return pFound;
            }
        }
        return Super::FindPass(name);
    }
}