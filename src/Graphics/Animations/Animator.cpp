//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/Animator.h>

#include <Utils/ECS/ComponentManager.h>

namespace SR_ANIMATIONS_NS {
    Animator::~Animator() {
        SR_SAFE_DELETE_PTR(m_graph);
        SR_SAFE_DELETE_PTR(m_workingPose);
        SR_SAFE_DELETE_PTR(m_staticPose);
    }

    bool Animator::InitializeEntity() noexcept {
        GetComponentProperties().AddCustomProperty<SR_UTILS_NS::PathProperty>("Font")
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

        GetComponentProperties().AddStandardProperty("Sync", &m_sync);
        GetComponentProperties().AddStandardProperty("Allow override", &m_allowOverride);
        GetComponentProperties().AddStandardProperty("Weight", &m_weight);

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

        if (!m_workingPose) {
            m_workingPose = new AnimationPose();
            m_workingPose->Initialize(m_skeleton);
        }

        if (!m_staticPose) {
            m_staticPose = new AnimationPose();
            m_staticPose->Initialize(m_skeleton);
        }
        else if (m_allowOverride) {
            m_staticPose->Update(m_skeleton, m_workingPose);
        }

        if (m_graph) {
            UpdateContext context;

            context.pStaticPose = m_staticPose;
            context.pWorkingPose = m_workingPose;
            context.now = SR_HTYPES_NS::Time::Instance().Now();
            context.weight = 1.f;
            context.dt = dt;

            m_graph->Update(context);
        }

        m_workingPose->Apply(m_skeleton);
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

        m_graph = new AnimationGraph(nullptr);

        auto&& pStateMachineNode = m_graph->AddNode<AnimationGraphNodeStateMachine>();
        auto&& pStateMachine = pStateMachineNode->GetMachine();

        auto&& pSetPoseState = pStateMachine->AddState<AnimationSetPoseState>(pAnimationClip);
        //pSetPoseState->SetClip(pAnimationClip);

        auto&& pClipState = pStateMachine->AddState<AnimationClipState>(pAnimationClip);
        //pClipState->SetClip(pAnimationClip);

        pStateMachine->GetEntryPoint()->AddTransition(pSetPoseState);
        pSetPoseState->AddTransition(pClipState);

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
