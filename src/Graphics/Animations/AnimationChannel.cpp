//
// Created by Monika on 01.05.2023.
//

#include <Graphics/Animations/AnimationChannel.h>

namespace SR_ANIMATIONS_NS {
    AnimationChannel::~AnimationChannel() {
        for (auto&& [time, pKey] : m_keys) {
            delete pKey;
        }
    }

    uint32_t AnimationChannel::UpdateChannel(uint32_t keyIndex, float_t time, UpdateContext& context, ChannelUpdateContext& channelContext) const {
        channelContext.pGraph = context.pGraph;
        channelContext.pPose = context.pPose;

    skipKey:
        if (keyIndex == m_keys.size()) {
            return keyIndex;
        }

        auto&& [keyTime, pKey] = m_keys[keyIndex];

        if (time > keyTime) {
            if (context.fpsCompensation) {
                if (keyIndex == 0) {
                    pKey->Update(0.f, nullptr, channelContext);
                }
                else {
                    pKey->Update(1.f, m_keys[keyIndex - 1].second, channelContext);
                }
            }

            ++keyIndex;

            goto skipKey;
        }

        if (keyIndex == 0) {
            pKey->Update(0.f, nullptr, channelContext);
        }
        else {
            auto&& [prevTime, prevKey] = m_keys[keyIndex - 1];

            const float_t currentTime = time - prevTime;
            const float_t keyCurrTime = keyTime - prevTime;
            const float_t progress = currentTime / keyCurrTime;

            pKey->Update(progress, prevKey, channelContext);
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

            //auto&& offset = pRawMesh->GetBoneOffset(boneName);
            //auto&& translationOffset = offset.GetTranslate();

            //auto&& poseMatrix = pRawMesh->GetBoneTransform(boneIndex);

            auto&& pTranslationChannel = new AnimationChannel();
            auto&& first = AiV3ToFV3(pChannel->mPositionKeys[0].mValue, mul);

            pTranslationChannel->SetName(pChannel->mNodeName.C_Str());
            pTranslationChannel->SetBoneIndex(boneIndex);

            for (uint16_t positionKeyIndex = 0; positionKeyIndex < pChannel->mNumPositionKeys; ++positionKeyIndex) {
                auto&& pPositionKey = pChannel->mPositionKeys[positionKeyIndex];

                auto&& translation = AiV3ToFV3(pPositionKey.mValue, mul);

                pTranslationChannel->AddKey(pPositionKey.mTime / ticksPerSecond,
                    new TranslationKey(
                        pTranslationChannel,
                        SR_MATH_NS::FVector3(),
                        //translation// - poseMatrix.GetTranslate()
                        translation
                        //first - translation
                    )
                );
            }

            channels.emplace_back(pTranslationChannel);
        }

        /// --------------------------------------------------------------------------------------------------------

        if (pChannel->mNumRotationKeys > 0) {
            auto&& pRotationChannel = new AnimationChannel();
            //auto&& first = AiQToQ(pChannel->mRotationKeys[0].mValue).Inverse();

            pRotationChannel->SetName(pChannel->mNodeName.C_Str());
            pRotationChannel->SetBoneIndex(boneIndex);

            for (uint16_t rotationKeyIndex = 0; rotationKeyIndex < pChannel->mNumRotationKeys; ++rotationKeyIndex) {
                auto&& pRotationKey = pChannel->mRotationKeys[rotationKeyIndex];

                auto&& q = AiQToQ(pRotationKey.mValue);
                //auto&& delta = q * first;

                pRotationChannel->AddKey(pRotationKey.mTime / ticksPerSecond,
                     new RotationKey(pRotationChannel, SR_MATH_NS::Quaternion(), q)
                );
            }

            channels.emplace_back(pRotationChannel);
        }

        /// --------------------------------------------------------------------------------------------------------

        if (pChannel->mNumScalingKeys > 0) {
            auto&& pScalingChannel = new AnimationChannel();
            auto&& first = AiV3ToFV3(pChannel->mScalingKeys[0].mValue, 1.f);

            pScalingChannel->SetName(pChannel->mNodeName.C_Str());
            pScalingChannel->SetBoneIndex(boneIndex);

            for (uint16_t scalingKeyIndex = 0; scalingKeyIndex < pChannel->mNumScalingKeys; ++scalingKeyIndex) {
                auto&& pScalingKey = pChannel->mScalingKeys[scalingKeyIndex];

                auto&& scale = AiV3ToFV3(pScalingKey.mValue, 1.f);

                pScalingChannel->AddKey(pScalingKey.mTime / ticksPerSecond,
                    new ScalingKey(
                        pScalingChannel,
                        scale,
                        scale / first
                    )
                );
            }

            channels.emplace_back(pScalingChannel);
        }
    }

    void AnimationChannel::SetName(SR_UTILS_NS::StringAtom name) {
        m_name = name;
        SRAssert(!m_name.empty());
    }

    void AnimationChannel::AddKey(float_t timePoint, AnimationKey* pKey) {
        m_keys.emplace_back(std::make_pair(timePoint, pKey));
    }
}