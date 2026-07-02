//
// Created by Monika on 07.01.2023.
//

#ifndef SR_ENGINE_ANIMATOR_H
#define SR_ENGINE_ANIMATOR_H

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/AnimationGraph.h>
#include <Graphics/Animations/AnimationClip.h>
#include <Graphics/Animations/AnimationStateMachine.h>
#include <Graphics/IK/IKSystem.h>

#include <Utils/ECS/EntityRef.h>
#include <Utils/ECS/Component.h>
#include <Utils/ECS/ComponentManager.h>

namespace SR_ANIMATIONS_NS {
    class AnimationClip;
    class AnimationChannel;

    /// @category(Animations)
    class Animator : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
    public:
        ~Animator() override;

    public:
        void FixedUpdate() override;
        void Update(float_t dt) override;

        void OnAttached() override;
        void OnDestroy() override;

        void Start() override;

        void SetGraph(const SR_UTILS_NS::Path& path);

        void SetBool(const SR_UTILS_NS::StringAtom& name, bool value);
        void SetInt(const SR_UTILS_NS::StringAtom& name, int32_t value);
        void SetFloat(const SR_UTILS_NS::StringAtom& name, float_t value);
        void SetString(const SR_UTILS_NS::StringAtom& name, const std::string& value);

        SR_NODISCARD std::optional<bool> GetBool(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD std::optional<int32_t> GetInt(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD std::optional<float_t> GetFloat(const SR_UTILS_NS::StringAtom& name) const;
        SR_NODISCARD std::optional<std::string> GetString(const SR_UTILS_NS::StringAtom& name) const;

        SR_NODISCARD SR_UTILS_NS::Path GetGraphPath() const noexcept;
        SR_NODISCARD AnimationGraph* GetGraph() const noexcept;
        SR_NODISCARD SR_HTYPES_NS::SharedPtr<Skeleton> GetSkeleton() noexcept;

    private:
        void UpdateInternal(float_t dt);

    private:
        /// @property
        SR_UTILS_NS::EntityRef<Skeleton> m_skeleton;

        /// @property @group(Advanced)
        uint32_t m_frameRate = 1;
        /// @property @group(Advanced) @drag(0.01f)
        float_t m_tolerance = 1.0f;
        /// @property @group(Advanced)
        bool m_sync = false;
        /// @property @group(Advanced)
        bool m_fpsCompensation = false;
        /// @property @group(Advanced)
        std::vector<SR_UTILS_NS::EntityRef<IKSystem>> m_IKSystems;

        /// @property
        SR_UTILS_NS::ResourceRef<AnimationClip> m_clip;
        /// @virtualProperty(graph) @getter(GetGraphPath) @setter(SetGraph)
        /// @customArgs(pick: enabled, filter name: Animator, relative: resources)
        /// @customArg(filter value: animator)
        /// @condition(!This.m_clip.IsValid())
        SR_VIRTUAL_PROPERTY

        std::vector<bool> m_preparedIKSystems;

        SR_HTYPES_NS::SharedPtr<AnimationGraph> m_graph;

    };
}

#endif //SR_ENGINE_ANIMATOR_H
