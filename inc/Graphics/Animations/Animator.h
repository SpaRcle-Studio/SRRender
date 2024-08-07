//
// Created by Monika on 07.01.2023.
//

#ifndef SR_ENGINE_ANIMATOR_H
#define SR_ENGINE_ANIMATOR_H

#include <Utils/ECS/Component.h>
#include <Utils/ECS/ComponentManager.h>

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/AnimationGraph.h>
#include <Graphics/Animations/AnimationStateMachine.h>

namespace SR_ANIMATIONS_NS {
    class AnimationClip;
    class AnimationChannel;

    class Animator : public SR_UTILS_NS::Component {
        SR_REGISTER_NEW_COMPONENT(Animator, 1001);
        using Super = SR_UTILS_NS::Component;
    public:
        ~Animator() override;

    public:
        bool InitializeEntity() noexcept override;

        void FixedUpdate() override;
        void Update(float_t dt) override;

        void OnAttached() override;
        void OnDestroy() override;

        void Start() override;

        void SetGraph(const SR_UTILS_NS::Path& path);

        SR_NODISCARD AnimationGraph* GetGraph() const noexcept { return m_graph; }
        SR_NODISCARD SR_HTYPES_NS::SharedPtr<Skeleton>& GetSkeleton() noexcept { return m_skeleton; }

    private:
        void UpdateInternal(float_t dt);

    private:
        uint32_t m_frameRate = 1;
        float_t m_tolerance = 0.001f;

        bool m_sync = false;
        bool m_fpsCompensation = false;

        AnimationGraph* m_graph = nullptr;

        SR_HTYPES_NS::SharedPtr<Skeleton> m_skeleton;

    };
}

#endif //SR_ENGINE_ANIMATOR_H
