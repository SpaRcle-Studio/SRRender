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
    protected:
        ~Animator() override;

    public:
        bool InitializeEntity() noexcept override;

        void FixedUpdate() override;
        void Update(float_t dt) override;

        void OnAttached() override;
        void OnDestroy() override;

        void Start() override;

        void SetClipPath(const SR_UTILS_NS::Path& path);
        void SetClipIndex(uint32_t index);

    private:
        void UpdateInternal(float_t dt);

        void ReloadClip();

    private:
        float_t m_weight = 1.f;

        SR_UTILS_NS::Path m_clipPath;
        uint32_t m_clipIndex = 0;

        bool m_sync = false;
        bool m_allowOverride = true;

        AnimationPose* m_workingPose = nullptr;
        AnimationPose* m_staticPose = nullptr;

        AnimationGraph* m_graph = nullptr;

        Skeleton* m_skeleton = nullptr;

    };
}

#endif //SR_ENGINE_ANIMATOR_H
