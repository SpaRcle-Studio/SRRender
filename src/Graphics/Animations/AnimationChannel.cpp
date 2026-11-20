//
// Created by Monika on 01.05.2023.
//

#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/Memory/Allocator.h>

#ifdef SR_UTILS_ASSIMP
    #include <assimp/anim.h>
#endif

#include <Codegen/AnimationChannel.generated.hpp>

namespace SR_ANIMATIONS_NS {
    KeysPool::KeysPool(const uint32_t count) {
        m_count = count;
        if (m_count > 0) {
            m_pKeys = SRMalloc(sizeof(UnionAnimationKey) * count);
        }
    }

    KeysPool::~KeysPool() {
        if (m_pKeys) {
            SRFree(m_pKeys);
        }
    }

    std::span<UnionAnimationKey> KeysPool::AllocateUnionKeys(uint32_t count) {
        const uint32_t offset = m_offset.fetch_add(count, std::memory_order_relaxed);
        if (offset > m_count || count > m_count - offset) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("KeysPool::AllocateUnionKeys() : pool overflow!");
            return {};
        }
        return { reinterpret_cast<UnionAnimationKey*>(static_cast<char*>(m_pKeys) + (offset * sizeof(UnionAnimationKey))), count };
    }

    AnimationChannel::~AnimationChannel() = default;

    uint32_t AnimationChannel::UpdateChannelWithWeight(uint32_t keyIndex, float_t time, ChannelAnimationUpdateContext& context, AnimationGameObjectData& data) const {
        const auto keysCount = static_cast<uint32_t>(m_keys.size());
        const UnionAnimationKey* pData = m_keys.data();

        while (keyIndex < keysCount && time > pData[keyIndex].time) SR_UNLIKELY_ATTRIBUTE {
            if (context.fpsCompensation) SR_UNLIKELY_ATTRIBUTE {
                pData[keyIndex].SetWithWeight(m_type, data, context.weight);
            }

            keyIndex += context.frameRate;
        }

        const uint32_t workingKeyIndex = SR_MIN(keyIndex, keysCount - 1);
        auto&& key = pData[workingKeyIndex];

        if (workingKeyIndex == 0) SR_UNLIKELY_ATTRIBUTE {
            key.SetWithWeight(m_type, data, context.weight);
        }
        else {
            auto&& prevKey = pData[workingKeyIndex - 1];

            const float_t currentTime = time - prevKey.time;
            const float_t keyCurrTime = key.time - prevKey.time;
            float_t progress = 0.f;
            if (keyCurrTime > 0.f) {
                progress = currentTime / keyCurrTime;
            }
            progress = SR_CLAMP01(progress);

            key.UpdateWithWeight(m_type, progress, prevKey, data, context.weight);
        }

        return keyIndex;
    }

    uint32_t AnimationChannel::UpdateChannel(uint32_t keyIndex, float_t time, ChannelAnimationUpdateContext& context, AnimationGameObjectData& data) const {
        const auto keysCount = static_cast<uint32_t>(m_keys.size());
        const UnionAnimationKey* pData = m_keys.data();

        while (keyIndex < keysCount && time > pData[keyIndex].time) {
            if (context.fpsCompensation) SR_UNLIKELY_ATTRIBUTE {
                pData[keyIndex].Set(m_type, data, context.tolerance);
            }

            keyIndex += context.frameRate;
        }

        const uint32_t workingKeyIndex = SR_MIN(keyIndex, keysCount - 1);
        auto&& key = pData[workingKeyIndex];

        if (workingKeyIndex == 0) SR_UNLIKELY_ATTRIBUTE {
            key.Set(m_type, data, context.tolerance);
        }
        else {
            auto&& prevKey = pData[workingKeyIndex - 1];

            const float_t currentTime = time - prevKey.time;
            const float_t keyCurrTime = key.time - prevKey.time;
            float_t progress = 0.f;
            if (keyCurrTime > 0.f) {
                progress = currentTime / keyCurrTime;
            }
            progress = SR_CLAMP01(progress);

            key.Update(m_type, progress, prevKey, data, context.tolerance);
        }

        return keyIndex;
    }

    void AnimationChannel::Load(aiNodeAnim* pChannel, LoadContext& context) {
        SR_TRACY_ZONE;

    #ifdef SR_UTILS_ASSIMP
        auto&& channelName = SR_UTILS_NS::StringAtom(SR_UTILS_NS::StringView(pChannel->mNodeName.C_Str(), pChannel->mNodeName.length));
        const ExcludeBoneTransforms* pExcludeInfo = nullptr;
        if (auto&& pIt = context.excluded.find(channelName); pIt != context.excluded.end()) {
            pExcludeInfo = &pIt->second;
        }

        const auto boneIndex = context.pSkeleton->GetBoneInfo(context.meshId, channelName).boneId;

        AnimationChannel* pTNewChannel = nullptr;
        AnimationChannel* pRNewChannel = nullptr;
        AnimationChannel* pSNewChannel = nullptr;

        const bool hasT = pChannel->mNumPositionKeys > 0 && (!pExcludeInfo || !pExcludeInfo->translation);
        const bool hasR = pChannel->mNumRotationKeys > 0 && (!pExcludeInfo || !pExcludeInfo->rotation);
        const bool hasS = pChannel->mNumScalingKeys > 0 && (!pExcludeInfo || !pExcludeInfo->scale);

        if (hasT || hasR || hasS) {
            SR_GLOBAL_LOCK;
            pTNewChannel = hasT ? &context.channels.emplace_back() : nullptr;
            pRNewChannel = hasR ? &context.channels.emplace_back() : nullptr;
            pSNewChannel = hasS ? &context.channels.emplace_back() : nullptr;
        }

        if (pChannel->mNumPositionKeys > 0 && (!pExcludeInfo || !pExcludeInfo->translation)) {
            auto&& keys = context.pool.AllocateUnionKeys(pChannel->mNumPositionKeys);
            pTNewChannel->SetKeys(keys);
            pTNewChannel->SetName(channelName);
            pTNewChannel->SetType(AnimationChannelType::Translation);
            if (boneIndex.has_value()) {
                pTNewChannel->SetBoneIndex(boneIndex.value());
            }
            for (uint32_t i = 0; i < pChannel->mNumPositionKeys; ++i) {
                static constexpr float_t mul = 0.01;
                auto&& key = pChannel->mPositionKeys[i];
                keys[i].data.translation = SR_MATH_NS::FVector3(key.mValue.x * mul, key.mValue.y * mul, key.mValue.z * mul);
                keys[i].time = static_cast<float_t>(key.mTime) * context.invTicksPerSecond;
            }
        }

        /// --------------------------------------------------------------------------------------------------------

        if (pChannel->mNumRotationKeys > 0 && (!pExcludeInfo || !pExcludeInfo->rotation)) {
            auto&& keys = context.pool.AllocateUnionKeys(pChannel->mNumRotationKeys);
            pRNewChannel->SetKeys(keys);
            pRNewChannel->SetName(channelName);
            pRNewChannel->SetType(AnimationChannelType::Rotation);
            if (boneIndex.has_value()) {
                pRNewChannel->SetBoneIndex(boneIndex.value());
            }
            for (uint32_t i = 0; i < pChannel->mNumRotationKeys; ++i) {
                auto&& key = pChannel->mRotationKeys[i];
                keys[i].data.rotation = SR_MATH_NS::Quaternion(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w);
                keys[i].time = static_cast<float_t>(key.mTime) * context.invTicksPerSecond;
            }
        }

        /// --------------------------------------------------------------------------------------------------------

        if (pChannel->mNumScalingKeys > 0 && (!pExcludeInfo || !pExcludeInfo->scale)) {
            auto&& keys = context.pool.AllocateUnionKeys(pChannel->mNumScalingKeys);
            pSNewChannel->SetKeys(keys);
            pSNewChannel->SetName(channelName);
            pSNewChannel->SetType(AnimationChannelType::Scaling);
            if (boneIndex.has_value()) {
                pSNewChannel->SetBoneIndex(boneIndex.value());
            }
            for (uint32_t i = 0; i < pChannel->mNumScalingKeys; ++i) {
                auto&& key = pChannel->mScalingKeys[i];
                keys[i].data.scaling = SR_MATH_NS::FVector3(key.mValue.x, key.mValue.y, key.mValue.z);
                keys[i].time = static_cast<float_t>(key.mTime) * context.invTicksPerSecond;
            }
        }
    #endif
    }

    void AnimationChannel::SetName(SR_UTILS_NS::StringAtom name) {
        m_name = name;
    }
}