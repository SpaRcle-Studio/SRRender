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

    AnimationState* AnimationState::Load(const SR_XML_NS::Node& nodeXml) {
        SR_TRACY_ZONE;

        AnimationState* pState = nullptr;

        auto&& type = nodeXml.GetAttribute("Type").ToString();
        if (type == "Clip") {
            pState = AnimationClipState::Load(nodeXml);
        }

        if (pState) {
            pState->SetResetOnPlay(nodeXml.GetAttribute("ResetOnPlay").ToBool(true));
            return pState;
        }

        SR_ERROR("AnimationState::Load() : unknown type \"{}\"!", type);

        return nullptr;
    }

    void AnimationState::OnTransitionBegin(AnimationStateTransition* pTransition) {
        m_activeTransition = pTransition;

        if (m_resetOnPlay) {
            Reset();
        }
    }

    void AnimationState::OnTransitionDone() {
        m_activeTransition = nullptr;

        for (auto&& pTransition : m_transitions) {
            pTransition->Reset();
        }
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

        auto&& channels = m_clip->GetChannels();
        const auto channelsCount = static_cast<uint32_t>(channels.size());

        if (context.weight > 0.f && context.weight < 1.f) SR_UNLIKELY_ATTRIBUTE {
            for (uint32_t i = 0; i < channelsCount; ++i) {
                uint32_t keyFrame = channels[i]->UpdateChannelWithWeight(
                    m_channelPlayState[i],
                    m_time,
                    context,
                    m_channelContexts[i]
                );

                currentKeyFrame = SR_MAX(currentKeyFrame, keyFrame);
            }
        }
        else {
            for (uint32_t i = 0; i < channelsCount; ++i) {
                uint32_t keyFrame = channels[i]->UpdateChannel(
                    m_channelPlayState[i],
                    m_time,
                    context,
                    m_channelContexts[i]
                );

                currentKeyFrame = SR_MAX(currentKeyFrame, keyFrame);
            }
        }

        m_time += context.dt;

        if (currentKeyFrame >= m_maxKeyFrame) {
            m_time = 0.f;
            memset(m_channelPlayState.data(), 0, m_channelPlayState.size() * sizeof(uint32_t));
        }

        Super::Update(context);
    }

    AnimationClipState::~AnimationClipState() {
        SetClip(nullptr);
    }

    AnimationClipState* AnimationClipState::Load(const SR_XML_NS::Node& nodeXml) {
        auto&& path = nodeXml.GetAttribute("Path").ToString();
        auto&& name = nodeXml.GetAttribute("Name").ToString();
        if (path.empty() || name.empty()) {
            SR_ERROR("AnimationClipState::Load() : path or name is empty!");
            return nullptr;
        }

        if (auto&& pClip = AnimationClip::Load(path, name)) {
            auto&& pState = new AnimationClipState();
            pState->SetClip(pClip);
            return pState;
        }

        SR_ERROR("AnimationClipState::Load() : failed to load clip \"{}\" with name \"{}\"!", path, name);
        return nullptr;
    }

    void AnimationClipState::SetClip(AnimationClip* pClip) {
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

    SR_UTILS_NS::StringAtom AnimationClipState::GetName() const noexcept {
        return m_clip ? m_clip->GetClipName() : SR_UTILS_NS::StringAtom();
    }

    bool AnimationClipState::Compile(CompileContext& context) {
        if (!m_clip) {
            SR_ERROR("AnimationClipState::Compile() : clip is nullptr!");
            return false;
        }

        m_channelContexts.clear();

        for (auto&& pChannel : m_clip->GetChannels()) {
            auto&& channelContext = m_channelContexts.emplace_back();

            if (pChannel->HasBoneIndex()) {
                if (!context.pSkeleton) {
                    SR_WARN("AnimationClipState::Compile() : skeleton is nullptr!");
                    continue;
                }

                auto&& pBone = context.pSkeleton->GetBone(pChannel->GetGameObjectName());
                if (!pBone) {
                    SR_WARN("AnimationClipState::Compile() : bone is nullptr!");
                    continue;
                }

                if (!pBone->gameObject) {
                    SR_WARN("AnimationClipState::Compile() : game object is nullptr!");
                    continue;
                }

                auto&& pIt = std::find(context.gameObjects.begin(), context.gameObjects.end(), pBone->gameObject);
                if (pIt != context.gameObjects.end()) {
                    channelContext.gameObjectIndex = static_cast<uint16_t>(std::distance(context.gameObjects.begin(), pIt));
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

        m_channelPlayState.resize(m_clip->GetChannels().size());

        return Super::Compile(context);
    }

    void AnimationClipState::Reset() {
        m_time = 0.f;
        if (!m_channelPlayState.empty()) {
            memset(m_channelPlayState.data(), 0, m_channelPlayState.size() * sizeof(uint32_t));
        }
        Super::Reset();
    }
}
