//
// Created by Monika on 08.05.2023.
//

#include <Graphics/Animations/AnimationState.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/Bone.h>

namespace SR_ANIMATIONS_NS {
    AnimationState::~AnimationState() {
        for (auto&& pTransition : m_transitions) {
            delete pTransition;
        }
    }

    void AnimationClipState::Update(UpdateContext& context) {
        SR_TRACY_ZONE;

        if (!m_clip) {
            Super::Update(context);
            return;
        }

        uint32_t currentKeyFrame = 0;

        auto&& channels = m_clip->GetChannels();
        const auto channelsCount = static_cast<uint32_t>(channels.size());

        for (uint32_t i = 0; i < channelsCount; ++i) {
            const uint32_t keyFrame = channels[i]->UpdateChannel(
                m_channelPlayState[i],
                m_time,
                context,
                m_channelContexts[i]
            );

            currentKeyFrame = SR_MAX(currentKeyFrame, keyFrame);
        }

        m_time += context.dt;

        if (currentKeyFrame >= m_maxKeyFrame) {
            m_time = 0.f;
            memset(m_channelPlayState.data(), 0, m_channelPlayState.size() * sizeof(uint32_t));
        }

        Super::Update(context);
    }

    bool AnimationClipState::Compile(CompileContext& context) {
        if (!m_clip) {
            SR_ERROR("AnimationClipState::Compile() : clip is nullptr!");
            return false;
        }

        m_channelPlayState.resize(m_clip->GetChannels().size());

        return Super::Compile(context);
    }

    IAnimationClipState::~IAnimationClipState() {
        SetClip(nullptr);
    }

    IAnimationClipState::IAnimationClipState(AnimationStateMachine* pMachine, AnimationClip* pClip)
        : Super(pMachine)
    {
        SetClip(pClip);
    }

    void IAnimationClipState::SetClip(AnimationClip* pClip) {
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
            for (auto&& pChannel : m_clip->GetChannels()) {
                m_maxKeyFrame = SR_MAX(m_maxKeyFrame, pChannel->GetKeys().size());
            }
        }
    }

    bool IAnimationClipState::Compile(CompileContext& context) {
        if (!m_clip) {
            SR_ERROR("IAnimationClipState::Compile() : clip is nullptr!");
            return false;
        }

        m_channelContexts.clear();

        for (auto&& pChannel : m_clip->GetChannels()) {
            auto&& channelContext = m_channelContexts.emplace_back();

            if (pChannel->HasBoneIndex()) {
                if (!context.pSkeleton) {
                    SR_WARN("IAnimationClipState::Compile() : skeleton is nullptr!");
                    continue;
                }

                auto&& pBone = context.pSkeleton->GetBone(pChannel->GetGameObjectName());
                if (!pBone) {
                    SR_WARN("IAnimationClipState::Compile() : bone is nullptr!");
                    continue;
                }

                if (!pBone->gameObject) {
                    SR_WARN("IAnimationClipState::Compile() : game object is nullptr!");
                    continue;
                }

                channelContext.gameObjectIndex = static_cast<uint16_t>(context.gameObjects.size());
                context.gameObjects.emplace_back(pBone->gameObject);
            }
            else {
                /// auto&& name = pChannel->GetGameObjectName();
                SRHalt("Not implemented!");
            }
        }

        return Super::Compile(context);
    }

    void AnimationSetPoseState::OnTransitionBegin(const UpdateContext& context) {
        Super::OnTransitionBegin(context);
    }
}