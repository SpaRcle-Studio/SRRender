//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/Animator.h>

#include <Utils/ECS/ComponentManager.h>

namespace SR_ANIMATIONS_NS {
    Animator::~Animator() {
        SR_SAFE_DELETE_PTR(m_graph);
    }

    bool Animator::InitializeEntity() noexcept {
        GetComponentProperties().AddCustomProperty<SR_UTILS_NS::PathProperty>("Clip")
            .AddFileFilter("Animation clip", {{ "fbx" }})
            .SetGetter([this]()-> SR_UTILS_NS::Path {
                return m_clipPath;
            })
            .SetSetter([this](const SR_UTILS_NS::Path& path) {
                SetClipPath(path);
            });

        GetComponentProperties().AddStandardProperty("Clip index", &m_clipIndex)
            .SetSetter([this](void* pData) {
                SetClipIndex(*static_cast<uint32_t*>(pData));
            });

        GetComponentProperties().AddStandardProperty("Frame rate", &m_frameRate);

        GetComponentProperties().AddStandardProperty("Sync", &m_sync);

        return Super::InitializeEntity();
    }

    void Animator::OnDestroy() {
        Super::OnDestroy();
        GetThis().AutoFree([](auto&& pData) {
            delete pData;
        });
    }

    void Animator::FixedUpdate() {
        if (m_sync) {
            UpdateInternal(1.f / 60.f);
        }

        Super::FixedUpdate();
    }

    void Animator::Update(float_t dt) {
        SR_TRACY_ZONE;

        m_skeleton = GetParent()->GetComponent<Skeleton>();

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

        if (m_graph) {
            UpdateContext context;

            context.frameRate = SR_MAX(1, m_frameRate);
            context.now = SR_HTYPES_NS::Time::Instance().Now();
            context.weight = 1.f;
            context.dt = dt;

            m_graph->Update(context);
        }
    }

    void Animator::ReloadClip() {
        SR_SAFE_DELETE_PTR(m_graph);

        if (m_clipPath.IsEmpty()) {
            return;
        }

        auto&& pAnimationClip = AnimationClip::Load(m_clipPath, m_clipIndex);
        if (!pAnimationClip) {
            SR_ERROR("Animator::ReloadClip() : failed to load animation clip: {}", m_clipPath.ToStringView());
            return;
        }

        m_graph = new AnimationGraph(this);

        auto&& pStateMachineNode = m_graph->AddNode<AnimationGraphNodeStateMachine>();
        auto&& pStateMachine = pStateMachineNode->GetMachine();

        auto&& pClipState = pStateMachine->AddState<AnimationClipState>(pAnimationClip);

        pStateMachine->GetEntryPoint()->AddTransition(pClipState);

        m_graph->GetFinal()->AddInput(pStateMachineNode, 0, 0);
    }

    void Animator::OnAttached() {
        Super::OnAttached();
    }

    void Animator::Start() {
        Super::Start();
    }

    void Animator::SetClipPath(const SR_UTILS_NS::Path& path) {
        m_clipPath = path;
        ReloadClip();
    }

    void Animator::SetClipIndex(uint32_t index) {
        m_clipIndex = index;
        ReloadClip();
    }
}
