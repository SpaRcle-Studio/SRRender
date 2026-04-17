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

    bool GroupPass::PreInit() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            if (!pPass->PreInit()) {
                SR_ERROR("GroupPass::PreInit() : failed to pre-initialize pass \"{}\"!", pPass->GetPassName());
                return false;
            }
        }

        return Super::PreInit();
    }

    bool GroupPass::Init() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            if (!pPass->Init()) {
                SR_ERROR("GroupPass::Init() : failed to initialize pass \"{}\"!", pPass->GetPassName());
                return false;
            }
        }

        return Super::Init();
    }

    void GroupPass::DeInit() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            pPass->DeInit();
        }

        BasePass::DeInit();
    }

    bool GroupPass::Prepare() {
        SR_TRACY_ZONE;

        for (auto&& pPass : m_passes) {
            if (!pPass->Prepare()) {
                SR_ERROR("GroupPass::Prepare() : failed to prepare pass \"{}\"!", pPass->GetPassName());
                return false;
            }
        }

        return Super::Prepare();
    }

    bool GroupPass::PreRender() {
        SR_TRACY_ZONE;
        bool hasDrawData = false;

        for (auto&& pPass : m_passes) {
            if (pPass->HasPreRender()) {
                pPass->Bind();
                hasDrawData |= pPass->PreRender();
            }
        }

        return hasDrawData;
    }

    bool GroupPass::Render() {
        SR_TRACY_ZONE;
        bool hasDrawData = false;

        for (auto&& pPass : m_passes) {
            if (pPass->HasRender()) {
                pPass->Bind();
                hasDrawData |= pPass->Render();
            }
        }

        return hasDrawData;
    }

    bool GroupPass::PostRender() {
        SR_TRACY_ZONE;
        bool hasDrawData = false;
        for (auto&& pPass : m_passes) {
            if (pPass->HasPostRender()) {
                pPass->Bind();
                hasDrawData |= pPass->PostRender();
            }
        }
        return hasDrawData;
    }

    void GroupPass::Update() {
        SR_TRACY_ZONE;
        for (auto&& pPass : m_passes) {
            if (pPass->HasUpdate()) {
                pPass->Bind();
                pPass->Update();
            }
        }
        Super::Update();
    }

    bool GroupPass::Overlay() {
        bool hasDrawData = false;
        SR_TRACY_ZONE;
        for (auto&& pPass : m_passes) {
            hasDrawData |= pPass->Overlay();
        }
        return hasDrawData;
    }

    void GroupPass::OnResize(const SR_MATH_NS::UVector2 &size) {
        SR_TRACY_ZONE;
        for (auto&& pPass : m_passes) {
            pPass->OnResize(size);
        }
        Super::OnResize(size);
    }

    void GroupPass::OnMultisampleChanged() {
        SR_TRACY_ZONE;
        for (auto&& pPass : m_passes) {
            pPass->OnMultisampleChanged();
        }
        Super::OnMultisampleChanged();
    }

    void GroupPass::OnCameraParamsChanged() {
        SR_TRACY_ZONE;
        for (auto&& pPass : m_passes) {
            pPass->OnCameraParamsChanged();
        }
        Super::OnCameraParamsChanged();
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
        SR_TRACY_ZONE;
        for (auto&& pPass : m_passes) {
            pPass->PostUpdate();
        }
        Super::PostUpdate();
    }

    bool GroupPass::UpdateFrustum() {
        bool changed = Super::UpdateFrustum();
        for (auto&& pPass : m_passes) {
            changed |= pPass->UpdateFrustum();
        }
        return changed;
    }

    void GroupPass::ForEachPass(const std::function<void(BasePass&)>& func) {
        for (auto&& pPass : m_passes) {
            pPass->ForEachPass(func);
        }
        Super::ForEachPass(func);
    }

    void GroupPass::AddPass(const BasePass::Ptr& pPass) {
        SR_TRACY_ZONE;
        m_passes.emplace_back(pPass);
        pPass->SetParent(this);
    }

    void GroupPass::SetPasses(const std::vector<BasePass::Ptr>& passes) {
        SR_TRACY_ZONE;
        m_passes = passes;
        for (auto&& pPass : m_passes) {
            pPass->SetParent(this);
        }
    }

    BasePass* GroupPass::FindPass(SR_UTILS_NS::StringAtom name) {
        for (auto&& pPass : m_passes) {
            if (auto&& pFound = pPass->FindPass(name)) {
                return pFound;
            }
        }
        return Super::FindPass(name);
    }

    void GroupPass::InsertPass(const BasePass::Ptr& pPass, uint32_t index) {
        if (index > m_passes.size()) {
            index = static_cast<uint32_t>(m_passes.size());
        }
        m_passes.insert(m_passes.begin() + index, pPass);
        pPass->SetParent(this);
    }

    int32_t GroupPass::IndexOfPass(SR_UTILS_NS::StringAtom name) const {
        for (uint32_t i = 0; i < m_passes.size(); ++i) {
            if (m_passes[i]->GetPassName() == name) {
                return i;
            }
        }
        return SR_ID_INVALID;
    }

    void GroupPass::RemovePass(SpaRcle::Utils::StringAtom name) {
        SR_TRACY_ZONE;

        auto pIt = std::find_if(m_passes.begin(), m_passes.end(), [name](const BasePass::Ptr& pPass) {
            return pPass->GetPassName() == name;
        });
        if (pIt != m_passes.end()) {
            (*pIt)->DeInit();
            (*pIt).AutoFree();
            m_passes.erase(pIt);
        }
        else {
            SR_ERROR("GroupPass::RemovePass() : pass with name {} not found!", name);
        }
    }
}