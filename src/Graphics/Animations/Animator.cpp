//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/Animator.h>

#include <Utils/ECS/ComponentManager.h>

namespace SR_ANIMATIONS_NS {
    Animator::~Animator() {
        SetGraph(SR_UTILS_NS::Path());
    }

    bool Animator::InitializeEntity() noexcept {
        GetComponentProperties().AddCustomProperty<SR_UTILS_NS::PathProperty>("Graph")
            .AddFileFilter("Animation graph", {{ "xml" }})
            .SetGetter([this]()-> SR_UTILS_NS::Path {
                return m_graph ? m_graph->GetPath() : SR_UTILS_NS::Path();
            })
            .SetSetter([this](const SR_UTILS_NS::Path& path) {
                SetGraph(path);
            });

        GetComponentProperties().AddStandardProperty("Frame rate", &m_frameRate);
        GetComponentProperties().AddStandardProperty("Tolerance", &m_tolerance);

        GetComponentProperties().AddStandardProperty("Sync", &m_sync);
        GetComponentProperties().AddStandardProperty("FPS compensation", &m_fpsCompensation);

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

            context.tolerance = m_tolerance;
            context.frameRate = SR_MAX(1, m_frameRate);
            context.now = SR_HTYPES_NS::Time::Instance().Now();
            context.weight = 1.f;
            context.fpsCompensation = m_fpsCompensation;
            context.dt = dt;

            m_graph->Update(context);
        }
    }

    /*void Animator::ReloadClip() {
        SR_SAFE_DELETE_PTR(m_graph);

        if (m_clipPath.empty() || m_clipName.empty()) {
            return;
        }

        auto&& pAnimationClip = AnimationClip::Load(m_clipPath, m_clipName);
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
    }*/

    void Animator::OnAttached() {
        Super::OnAttached();
    }

    void Animator::Start() {
        Super::Start();
    }

    void Animator::SetGraph(const SR_UTILS_NS::Path& path) {
        SR_SAFE_DELETE_PTR(m_graph);
        if (path.IsEmpty()) {
            return;
        }

        if (path.IsAbs()) {
            m_graph = AnimationGraph::Load(this, path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath()));
        }
        else {
            m_graph = AnimationGraph::Load(this, path);
        }
    }

    //void Animator::SetClipPath(const SR_UTILS_NS::Path& path) {
    //    m_clipPath = path.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());
    //    ReloadClip();
    //}

    //void Animator::SetClipName(const std::string& name) {
    //    m_clipName = name;
    //    ReloadClip();
    //}
}
