//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/Animator.h>

#include <Utils/ECS/ComponentManager.h>
#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/Events/Broadcaster.h>
#include <Utils/Common/SubscriptionMessage.h>

#include <Codegen/Animator.generated.hpp>

namespace SR_ANIMATIONS_NS {
    Animator::~Animator() {
        SetGraph(SR_UTILS_NS::Path());
    }

    void Animator::OnDestroy() {
        Super::OnDestroy();
    }

    void Animator::FixedUpdate() {
        if (m_sync) {
            UpdateInternal(1.f / 60.f);
        }

        Super::FixedUpdate();
    }

    void Animator::Update(float_t dt) {
        SR_TRACY_ZONE;

        if (!m_skeleton) {
            if (auto&& pSkeleton = GetParent()->GetComponent<Skeleton>()) {
                m_skeleton.SetEntityId(pSkeleton->GetEntityId());
            }
        }

        if (!m_sync) {
            UpdateInternal(dt);
        }

        Super::Update(dt);
    }

    void Animator::UpdateInternal(float_t dt) {
        SR_TRACY_ZONE;

        if (!m_skeleton) {
            return;
        }

        if (!m_graph && !m_clip.IsValid()) {
            return;
        }

        if (m_clip.IsValid()) {
            if (!m_graph) {
                m_graph = SRNew<AnimationGraph>();
                m_graph->SetAnimator(this);
            }
            m_graph->SetSimpleClip(m_clip.GetResource());
        }

        m_preparedIKSystems.resize(m_IKSystems.size());
        for (size_t i = 0; i < m_IKSystems.size(); ++i) {
            if (auto&& pSystem = m_IKSystems[i].Get(); pSystem && pSystem->IsActive()) {
                if (!m_preparedIKSystems[i]) {
                    pSystem->UpdateIK(dt);
                    m_preparedIKSystems[i] = true;
                }
            }
        }

        UpdateContext context;

        if (auto&& pSkeleton = m_skeleton.Get()) {
            context.pRig = pSkeleton->GetRig();
            context.pSkeleton = pSkeleton.Get();
        }

        context.tolerance = m_tolerance / 1000.f / 1000.f;
        context.frameRate = SR_MAX(1, m_frameRate);
        context.weight = 1.f;
        context.fpsCompensation = m_fpsCompensation;
        context.dt = dt;

        m_graph->Update(context);

        for (auto&& pIK : m_IKSystems) {
            if (auto&& pSystem = pIK.Get()) {
                if (pSystem->IsActive()) {
                    pSystem->UpdateIK(dt);
                }
            }
        }
    }

    void Animator::OnAttached() {
        Super::OnAttached();
    }

    void Animator::Start() {
        Super::Start();

        for (auto&& pIK : m_IKSystems) {
            if (auto&& pSystem = pIK.Get()) {
                pSystem->SetControlledByAnimator(true);
            }
        }
    }

    void Animator::SetBool(SR_UTILS_NS::StringAtom name, const bool value) {
        if (m_graph) {
            m_graph->SetBool(name, value);
        }
    }

    void Animator::SetInt(SR_UTILS_NS::StringAtom name, const int32_t value) {
        if (m_graph) {
            m_graph->SetInt(name, value);
        }
    }

    void Animator::SetFloat(SR_UTILS_NS::StringAtom name, const float_t value) {
        if (m_graph) {
            m_graph->SetFloat(name, value);
        }
    }

    void Animator::SetString(SR_UTILS_NS::StringAtom name, SR_UTILS_NS::StringView value) {
        if (m_graph) {
            m_graph->SetString(name, value);
        }
    }

    SR_NODISCARD std::optional<bool> Animator::GetBool(const SR_UTILS_NS::StringAtom& name) const {
        return m_graph ? m_graph->GetBool(name) : std::nullopt;
    }

    SR_NODISCARD std::optional<int32_t> Animator::GetInt(const SR_UTILS_NS::StringAtom& name) const {
        return m_graph ? m_graph->GetInt(name) : std::nullopt;
    }

    SR_NODISCARD std::optional<float_t> Animator::GetFloat(const SR_UTILS_NS::StringAtom& name) const {
        return m_graph ? m_graph->GetFloat(name) : std::nullopt;
    }

    SR_NODISCARD std::optional<std::string> Animator::GetString(const SR_UTILS_NS::StringAtom& name) const {
        return m_graph ? m_graph->GetString(name) : std::nullopt;
    }

    void Animator::SetGraph(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;

        m_graph.AutoFree();
        if (path.IsEmpty()) {
            return;
        }

        auto&& loadPath = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
        if (auto&& pAsset = SR_UTILS_NS::Asset::Load<AnimationGraphAsset>(loadPath)) {
            m_graph = new AnimationGraph();
            pAsset->GetData().CloneTo(*m_graph);
            m_graph->SetAnimator(this);
            m_graph->SetAsset(pAsset.Get());
        }
        else {
            SR_ERROR("Animator::SetGraph() : failed to load animation graph asset: {}", loadPath);
        }
    }

    SR_UTILS_NS::Path Animator::GetGraphPath() const noexcept {
        return m_graph ? m_graph->GetPath() : SR_UTILS_NS::Path();
    }

    AnimationGraph* Animator::GetGraph() const noexcept {
        return const_cast<AnimationGraph*>(m_graph.Get());
    }

    SR_HTYPES_NS::SharedPtr<Skeleton> Animator::GetSkeleton() noexcept {
        return m_skeleton.Get();
    }

    void Animator::InspectGraph() {
        SR_UTILS_NS::SubscriptionMessage message;
        message.SetStringAtom("ClassName", Animator::GetClassStaticName());
        message.SetInt("EntityId", GetEntityId());
        SR_UTILS_NS::Broadcaster::Instance().Broadcast(SR_UTILS_NS::Events::EVENT_DO_INSPECT_ENTITY_ID, message);
    }
}
