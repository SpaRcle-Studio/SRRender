//
// Created by Monika on 01.05.2023.
//

#include <Graphics/Animations/AnimationChannel.h>

namespace SR_ANIMATIONS_NS {
    AnimationChannel::~AnimationChannel() {
        m_keys.clear();
    }

    uint32_t AnimationChannel::UpdateChannelWithWeight(uint32_t keyIndex, float_t time, UpdateContext& context, ChannelUpdateContext& channelContext) const {
        if (!channelContext.gameObjectIndex) SR_UNLIKELY_ATTRIBUTE {
            return keyIndex;
        }

        AnimationGameObjectData& data = context.pPose->GetGameObjectData(channelContext.gameObjectIndex.value());

        const auto keysCount = static_cast<uint32_t>(m_keys.size());

    skipKey:
        if (keyIndex >= keysCount) SR_UNLIKELY_ATTRIBUTE {
            return keyIndex;
        }

        auto&& key = m_keys[keyIndex];

        if (time > key.time) SR_UNLIKELY_ATTRIBUTE {
            if (context.fpsCompensation) SR_UNLIKELY_ATTRIBUTE {
                //key.SetWithWeight(data, context.tolerance);
            }

            keyIndex += context.frameRate;

            goto skipKey;
        }

        if (keyIndex == 0) SR_UNLIKELY_ATTRIBUTE {
            //key.SetWithWeight(data, context.weight);
        }
        else {
            auto&& prevKey = m_keys[keyIndex - 1];

            const float_t currentTime = time - prevKey.time;
            const float_t keyCurrTime = key.time - prevKey.time;
            const float_t progress = currentTime / keyCurrTime;

            key.UpdateWithWeight(progress, prevKey, data, context.weight);
        }

        return keyIndex;
    }

    uint32_t AnimationChannel::UpdateChannel(uint32_t keyIndex, float_t time, UpdateContext& context, ChannelUpdateContext& channelContext) const {
        if (!channelContext.gameObjectIndex) SR_UNLIKELY_ATTRIBUTE {
            return keyIndex;
        }

        AnimationGameObjectData& data = context.pPose->GetGameObjectData(channelContext.gameObjectIndex.value());

        const auto keysCount = static_cast<uint32_t>(m_keys.size());

    skipKey:
        if (keyIndex >= keysCount) SR_UNLIKELY_ATTRIBUTE {
            return keyIndex;
        }

        auto&& key = m_keys[keyIndex];

        if (time > key.time) SR_UNLIKELY_ATTRIBUTE {
            if (context.fpsCompensation) SR_UNLIKELY_ATTRIBUTE {
                key.Set(data, context.tolerance);
            }

            keyIndex += context.frameRate;

            goto skipKey;
        }

        if (keyIndex == 0) SR_UNLIKELY_ATTRIBUTE {
            key.Set(data, context.tolerance);
        }
        else {
            auto&& prevKey = m_keys[keyIndex - 1];

            const float_t currentTime = time - prevKey.time;
            const float_t keyCurrTime = key.time - prevKey.time;
            const float_t progress = currentTime / keyCurrTime;

            key.Update(progress, prevKey, data, context.tolerance);
        }

        return keyIndex;
    }

    void AnimationChannel::Load(SR_HTYPES_NS::RawMesh* pRawMesh, aiNodeAnim* pChannel, float_t ticksPerSecond, std::vector<AnimationChannel*>& channels) {
        SR_TRACY_ZONE;

        auto&& boneName = SR_UTILS_NS::StringAtom(pChannel->mNodeName.C_Str());
        auto&& boneIndex = pRawMesh->GetBoneIndex(boneName);
        if (boneIndex == SR_ID_INVALID) {
            return;
        }

        if (pChannel->mNumPositionKeys > 0) {
            static constexpr float_t mul = 0.01;

            auto&& pTranslationChannel = new AnimationChannel();

            pTranslationChannel->SetName(pChannel->mNodeName.C_Str());
            pTranslationChannel->SetBoneIndex(boneIndex);

            for (uint32_t positionKeyIndex = 0; positionKeyIndex < pChannel->mNumPositionKeys; ++positionKeyIndex) {
                auto&& pPositionKey = pChannel->mPositionKeys[positionKeyIndex];

                auto&& translation = AiV3ToFV3(pPositionKey.mValue, mul);

                pTranslationChannel->AddKey(pPositionKey.mTime / ticksPerSecond, TranslationKey(translation));
            }

            channels.emplace_back(pTranslationChannel);
        }

        /// --------------------------------------------------------------------------------------------------------

        if (pChannel->mNumRotationKeys > 0) {
            auto&& pRotationChannel = new AnimationChannel();

            pRotationChannel->SetName(pChannel->mNodeName.C_Str());
            pRotationChannel->SetBoneIndex(boneIndex);

            for (uint32_t rotationKeyIndex = 0; rotationKeyIndex < pChannel->mNumRotationKeys; ++rotationKeyIndex) {
                auto&& pRotationKey = pChannel->mRotationKeys[rotationKeyIndex];

                auto&& q = AiQToQ(pRotationKey.mValue);

                pRotationChannel->AddKey(pRotationKey.mTime / ticksPerSecond, RotationKey(q));
            }

            channels.emplace_back(pRotationChannel);
        }

        /// --------------------------------------------------------------------------------------------------------

        if (pChannel->mNumScalingKeys > 0) {
            auto&& pScalingChannel = new AnimationChannel();

            pScalingChannel->SetName(pChannel->mNodeName.C_Str());
            pScalingChannel->SetBoneIndex(boneIndex);

            for (uint32_t scalingKeyIndex = 0; scalingKeyIndex < pChannel->mNumScalingKeys; ++scalingKeyIndex) {
                auto&& pScalingKey = pChannel->mScalingKeys[scalingKeyIndex];

                auto&& scale = AiV3ToFV3(pScalingKey.mValue, 1.f);

                pScalingChannel->AddKey(pScalingKey.mTime / ticksPerSecond, ScalingKey(scale));
            }

            channels.emplace_back(pScalingChannel);
        }
    }

    void AnimationChannel::SetName(SR_UTILS_NS::StringAtom name) {
        m_name = name;
        SRAssert(!m_name.empty());
    }
}