//
// Created by Monika on 08.05.2023.
//

#include <Graphics/Animations/AnimationState.h>
#include <Graphics/Animations/AnimationChannel.h>
#include <Graphics/Animations/AnimationClip.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/Bone.h>
#include <Graphics/Animations/AnimationStateMachine.h>

#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/AnimationState.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationState::~AnimationState() = default;

    void AnimationState::OnTransitionBegin(AnimationStateTransition* pTransition) {
        m_activeTransition = pTransition;

        if (m_resetOnPlay) {
            ResetState();
        }
    }

    void AnimationState::OnTransitionDone() {
        m_activeTransition = nullptr;

        for (auto&& pTransition : m_transitions) {
            pTransition->ResetTransition();
        }
    }

    AnimationState::AnimationState()
        : SR_HTYPES_NS::SharedPtr<AnimationState>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    SR_UTILS_NS::StringAtom AnimationState::GetStateName() const noexcept {
        return m_stateName.empty() ? GetDefaultStateName() : m_stateName;
    }

    SR_UTILS_NS::StringAtom AnimationState::GetDefaultStateName() const noexcept {
        return GetMeta()->GetFactoryName();
    }

    bool AnimationState::Compile(CompileContext& context) {
        SR_TRACY_ZONE;

        if (!m_machine) {
            SRHalt("AnimationState::Compile() : machine is nullptr!");
            return false;
        }

        for (auto&& pTransition : m_transitions) {
            auto&& pDestinationState = m_machine->GetStateOrNull(pTransition->GetTargetIndex());
            if (!pDestinationState) {
                SR_ERROR("AnimationState::Compile() : failed to get destination state! State name: {}, Target index: {}", GetStateName(), pTransition->GetTargetIndex());
                return false;
            }
            pTransition->SetSourceState(this);
            pTransition->SetDestinationState(pDestinationState);
        }

        return true;
    }

    void AnimationClipState::Update(UpdateContext& context) {
        SR_TRACY_ZONE;

        if (!m_clip) {
            Super::Update(context);
            return;
        }

        if (context.weight <= 0.f) {
            Super::Update(context);
            return;
        }

        uint32_t currentKeyFrame = 0;

        auto&& channels = m_clip->GetChannels(context.pRig);
        const auto channelsCount = static_cast<uint32_t>(channels.size());

        for (uint32_t i = 0; i < channelsCount; ++i) {
            ChannelUpdateContext& channelContext = m_channelContexts[i];
            if (!channelContext.gameObjectIndex) SR_UNLIKELY_ATTRIBUTE {
                currentKeyFrame = SR_MAX(currentKeyFrame, m_channelPlayState[i]);
                continue;
            }

            auto&& data = context.pPose->GetGameObjectData(channelContext.gameObjectIndex.value());

            uint32_t keyFrame = 0;
            if (context.weight > 0.f && context.weight < 1.f) SR_UNLIKELY_ATTRIBUTE {
                keyFrame = channels[i].UpdateChannelWithWeight(m_channelPlayState[i], m_time, context, data);
            }
            else {
                keyFrame = channels[i].UpdateChannel(m_channelPlayState[i], m_time, context, data);
            }
            currentKeyFrame = SR_MAX(currentKeyFrame, keyFrame);
        }

        m_time += context.dt;

        if (currentKeyFrame >= m_maxKeyFrame) {
            SR_TRACY_ZONE_N("memset");
            memset(m_channelPlayState.data(), 0, m_channelPlayState.size() * sizeof(uint32_t));
            m_time = 0.f;
        }

        Super::Update(context);
    }

    AnimationClipState::~AnimationClipState() {
        SetClip(nullptr);
    }

    void AnimationClipState::SetClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip) {
        SR_TRACY_ZONE;

        if (m_clip == pClip) {
            return;
        }

        if (m_clip) {
            m_clip->RemoveUsePoint();
        }

        if (pClip) {
            pClip->AddUsePoint();
        }

        if ((m_clip = pClip)) {
            m_maxKeyFrame = m_clip->GetMaxKeyFrame();
            m_duration = m_clip->GetDuration();
        }
        else {
            m_maxKeyFrame = 0;
            m_duration = 0.f;
        }
    }

    float_t AnimationClipState::GetProgress() const noexcept {
        if (m_duration <= 0.f) {
            return 1.f;
        }
        return m_time / m_duration;
    }

    bool AnimationClipState::Compile(CompileContext& context) {
        if (!m_clip) {
            SR_ERROR("AnimationClipState::Compile() : clip is nullptr! Clip name: {}", m_animation);
            return false;
        }

        m_channelContexts.clear();

        const auto& channels = m_clip->GetChannels(context.pRig);

        /// Important: retargeting may bake channels (different key counts than the source clip),
        /// so we must compute these values from the channels actually used at runtime.
        m_maxKeyFrame = 0;
        m_duration = 0.f;

        for (const auto& channel : channels) {
            m_maxKeyFrame = SR_MAX(m_maxKeyFrame, static_cast<uint32_t>(channel.GetKeys().size()));
            for (const auto& key : channel.GetKeys()) {
                m_duration = SR_MAX(m_duration, key.time);
            }
        }

        for (auto&& channel : channels) {
            auto&& channelContext = m_channelContexts.emplace_back();

            if (channel.HasBoneIndex()) {
                if (!context.pSkeleton) {
                    SR_WARN("AnimationClipState::Compile() : skeleton is nullptr!");
                    continue;
                }

                auto&& pBone = context.pSkeleton->GetAnimationBone(channel.GetChannelName());
                if (!pBone) {
                    SR_WARN("AnimationClipState::Compile() : bone \"{}\" not found in skeleton \"{}\"!", channel.GetChannelName(), context.pSkeleton->GetGameObject()->GetName());
                    continue;
                }

                auto&& pBoneGameObject = pBone->GetGameObject();
                if (!pBoneGameObject) {
                    SR_WARN("AnimationClipState::Compile() : game object is nullptr!");
                    continue;
                }

                auto&& pIt = std::find(context.gameObjects.begin(), context.gameObjects.end(), pBoneGameObject);
                if (pIt != context.gameObjects.end()) {
                    channelContext.gameObjectIndex = static_cast<uint16_t>(std::distance(context.gameObjects.begin(), pIt));
                    continue;
                }

                channelContext.gameObjectIndex = static_cast<uint16_t>(context.gameObjects.size());
                context.gameObjects.emplace_back(pBoneGameObject);
            }
            else {
                /// auto&& name = pChannel->GetGameObjectName();
                SR_ERROR("Not implemented!");
            }
        }

        m_channelPlayState.resize(channels.size());

        return Super::Compile(context);
    }

    void AnimationClipState::ResetState() {
        SR_TRACY_ZONE;
        m_time = 0.f;
        if (!m_channelPlayState.empty()) {
            memset(m_channelPlayState.data(), 0, m_channelPlayState.size() * sizeof(uint32_t));
        }
        Super::ResetState();
    }

    void AnimationClipState::OnPostLoad() {
        UpdateClip();
        Super::OnPostLoad();
    }

    void AnimationClipState::UpdateClip() {
        SR_TRACY_ZONE;
        if (!m_animation.empty()) {
            if (auto&& pClip = SR_UTILS_NS::Asset::Load<AnimationClip>(m_animation)) {
                SetClip(pClip);
            }
            else {
                SR_ERROR("AnimationClipState::OnPostLoad() : failed to load clip \"{}\" with name \"{}\"!", m_animation);
            }
        }
    }

    void AnimationClipState::CloneTo(SR_UTILS_NS::SRClass& clone) const {
        Super::CloneTo(clone);
        static_cast<AnimationClipState&>(clone).UpdateClip();
    }

    SR_UTILS_NS::StringAtom AnimationClipState::GetDefaultStateName() const noexcept {
        return m_clip ? m_clip->GetClipName() : Super::GetDefaultStateName();
    }
}
