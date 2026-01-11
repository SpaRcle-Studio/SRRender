//
// Created by Monika on 26.04.2023.
//

#include <Graphics/Animations/AnimationPose.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/AnimationChannel.h>
#include <Graphics/Animations/AnimationClip.h>
#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Animations/Bone.h>

#include <Utils/ECS/Transform.h>

namespace SR_ANIMATIONS_NS {
    AnimationPose::~AnimationPose() {
        //for (auto&& [boneHashName, pData] : m_data) {
        //    delete pData;
        //}
        //m_indices.clear();
        //m_data.clear();
    }

    void AnimationPose::SetGameObjectsCount(uint32_t count) {
        m_gameObjects.resize(count);
    }

    AnimationGameObjectData& AnimationPose::GetGameObjectData(Index index) noexcept {
        if (index < m_gameObjects.size()) SR_LIKELY_ATTRIBUTE {
            return m_gameObjects[index];
        }
        static AnimationGameObjectData empty;
        SRHalt("Invalid index!");
        return empty;
    }

    void AnimationPose::BlendLinear(AnimationPose& first, AnimationPose& second, float_t factor, QuaternionBlendMode quatBlendMode) {
        SR_TRACY_ZONE;

        auto&& firstGameObjects = first.m_gameObjects;
        auto&& secondGameObjects = second.m_gameObjects;

        const uint32_t minSize = SR_MIN(firstGameObjects.size(), secondGameObjects.size());
        const uint32_t maxSize = SR_MAX(firstGameObjects.size(), secondGameObjects.size());

        m_gameObjects.resize(maxSize);

        for (size_t i = 0; i < minSize; ++i) {
            const AnimationGameObjectData& firstData = firstGameObjects[i];
            const AnimationGameObjectData& secondData = secondGameObjects[i];

            m_gameObjects[i].dirty = firstData.dirty || secondGameObjects[i].dirty;
            if (!m_gameObjects[i].dirty) {
                continue;
            }

            if (firstData.translation.has_value() && secondGameObjects[i].translation.has_value()) {
                m_gameObjects[i].translation = SR_MATH_NS::FVector3::Lerp(firstData.translation.value(), secondData.translation.value(), factor);
            }
            else if (firstData.translation.has_value()) {
                m_gameObjects[i].translation = firstData.translation;
            }
            else if (secondData.translation.has_value()) {
                m_gameObjects[i].translation = secondData.translation;
            }
            else {
                m_gameObjects[i].translation.FastReset();
            }

            if (firstData.rotation.has_value() && secondData.rotation.has_value()) {
                if (quatBlendMode == QuaternionBlendMode::Nlerp) {
                    m_gameObjects[i].rotation = SR_MATH_NS::Quaternion::Nlerp(firstData.rotation.value(), secondData.rotation.value(), factor);
                }
                else {
                    m_gameObjects[i].rotation = SR_MATH_NS::Quaternion::Slerp(firstData.rotation.value(), secondData.rotation.value(), factor);
                }
            }
            else if (firstData.rotation.has_value()) {
                m_gameObjects[i].rotation = firstData.rotation;
            }
            else if (secondData.rotation.has_value()) {
                m_gameObjects[i].rotation = secondData.rotation;
            }
            else {
                m_gameObjects[i].rotation.FastReset();
            }

            if (firstData.scaling.has_value() && secondData.scaling.has_value()) {
                m_gameObjects[i].scaling = SR_MATH_NS::FVector3::Lerp(firstData.scaling.value(), secondData.scaling.value(), factor);
            }
            else if (firstData.scaling.has_value()) {
                m_gameObjects[i].scaling = firstData.scaling;
            }
            else if (secondData.scaling.has_value()) {
                m_gameObjects[i].scaling = secondData.scaling;
            }
            else {
                m_gameObjects[i].scaling.FastReset();
            }
        }

        for (size_t i = minSize; i < maxSize; ++i) {
            if (firstGameObjects.size() > secondGameObjects.size()) {
                m_gameObjects[i] = firstGameObjects[i];
            }
            else {
                m_gameObjects[i] = secondGameObjects[i];
            }
        }
    }

    void AnimationPose::BlendAdditive(AnimationPose& base, AnimationPose& additive, float_t factor) {
        SR_TRACY_ZONE;

        auto&& baseGameObjects = base.m_gameObjects;
        auto&& additiveGameObjects = additive.m_gameObjects;

        const uint32_t minSize = SR_MIN(baseGameObjects.size(), additiveGameObjects.size());
        const uint32_t maxSize = SR_MAX(baseGameObjects.size(), additiveGameObjects.size());

        m_gameObjects.resize(maxSize);

        for (size_t i = 0; i < minSize; ++i) {
            const AnimationGameObjectData& baseData = baseGameObjects[i];
            const AnimationGameObjectData& additiveData = additiveGameObjects[i];

            m_gameObjects[i].dirty = baseData.dirty || additiveData.dirty;
            if (!m_gameObjects[i].dirty) {
                continue;
            }

            if (baseData.translation.has_value() && additiveData.translation.has_value()) {
                m_gameObjects[i].translation = baseData.translation.value() + additiveData.translation.value() * factor;
            }
            else if (baseData.translation.has_value()) {
                m_gameObjects[i].translation = baseData.translation;
            }
            else if (additiveData.translation.has_value()) {
                m_gameObjects[i].translation = additiveData.translation;
            }
            else {
                m_gameObjects[i].translation.FastReset();
            }

            if (baseData.rotation.has_value() && additiveData.rotation.has_value()) {
                m_gameObjects[i].rotation = baseData.rotation.value() * SR_MATH_NS::Quaternion::FromEulerAngles(additiveData.rotation.value().EulerAngle() * factor);
            }
            else if (baseData.rotation.has_value()) {
                m_gameObjects[i].rotation = baseData.rotation;
            }
            else if (additiveData.rotation.has_value()) {
                m_gameObjects[i].rotation = additiveData.rotation;
            }
            else {
                m_gameObjects[i].rotation.FastReset();
            }

            if (baseData.scaling.has_value() && additiveData.scaling.has_value()) {
                m_gameObjects[i].scaling = baseData.scaling.value() + additiveData.scaling.value() * factor;
            }
            else if (baseData.scaling.has_value()) {
                m_gameObjects[i].scaling = baseData.scaling;
            }
            else if (additiveData.scaling.has_value()) {
                m_gameObjects[i].scaling = additiveData.scaling;
            }
            else {
                m_gameObjects[i].scaling.FastReset();
            }
        }

        for (size_t i = minSize; i < maxSize; ++i) {
            if (baseGameObjects.size() > additiveGameObjects.size()) {
                m_gameObjects[i] = baseGameObjects[i];
            }
            else {
                m_gameObjects[i] = additiveGameObjects[i];
            }
        }
    }

    /*AnimationData* AnimationPose::GetData(SR_UTILS_NS::StringAtom boneName) const noexcept {
        if (auto&& pIt = m_indices.find(boneName); pIt != m_indices.end()) {
            return pIt->second;
        }

        return nullptr;
    }

    AnimationData* AnimationPose::GetDataByIndex(uint16_t index) const noexcept {
        if (index < m_data.size()) {
            return m_data[index].second;
        }

        return nullptr;
    }

    void AnimationPose::Reset() {
        for (auto&& [boneHashName, pData] : m_data) {
            pData->Reset();
        }
    }

    void AnimationPose::Initialize(const Skeleton* pSkeleton) {
        SR_TRACY_ZONE;

        SRAssert(!m_isInitialized);

        auto&& bones = pSkeleton->GetBones();

        m_indices.reserve(bones.size());
        m_data.reserve(bones.size());

        for (auto&& pBone : bones) {
            auto&& pair = std::make_pair(
                pBone->name,
                new AnimationData()
            );

            m_indices.insert(pair);
            m_data.emplace_back(pair);
        }

        m_isInitialized = true;
    }

    void AnimationPose::Apply(Skeleton* pSkeleton) {
        SR_TRACY_ZONE;

        if (!m_isInitialized) {
            Initialize(pSkeleton);
        }

        for (auto&& [boneHashName, pWorkingData] : m_data) {
            auto&& pBone = pSkeleton->TryGetBone(boneHashName);
            if (!pBone || !pBone->gameObject) {
                continue;
            }

            Apply(pWorkingData, pBone->gameObject);
        }
    }

    void AnimationPose::Apply(const AnimationData* pWorkingData, const SR_UTILS_NS::GameObject::Ptr& pGameObject) {
        auto&& pTransform = pGameObject->GetTransform();

        if (pWorkingData->translation.has_value()) {
            pTransform->SetTranslation(pWorkingData->translation.value());
        }

        if (pWorkingData->rotation.has_value()) {
            pTransform->SetRotation(pWorkingData->rotation.value());
        }

        if (pWorkingData->scale.has_value()) {
            pTransform->SetScale(pWorkingData->scale.value());
        }
    }

    void AnimationPose::Update(Skeleton* pSkeleton, AnimationPose* pWorkingPose) {
        SR_TRACY_ZONE;

        if (!pWorkingPose) {
            SRHalt0();
            return;
        }

        if (!m_isInitialized) {
            Initialize(pSkeleton);
        }

        for (auto&& [boneHashName, pData] : m_data) {
            auto&& pBone = pSkeleton->TryGetBone(boneHashName);
            if (!pBone || !pBone->gameObject) {
                continue;
            }

            auto&& pWorkingData = pWorkingPose->GetData(boneHashName);
            if (!pWorkingData) {
                return;
            }

            Update(pData, pWorkingData, pBone->gameObject);
        }
    }

    void AnimationPose::Update(AnimationData* pStaticData, const AnimationData* pWorkingData, const SR_UTILS_NS::GameObject::Ptr& pGameObject) {
        auto&& pTransform = pGameObject->GetTransform();

        /// ------------------------------------------------------------------------------------------------------------

        if (!pStaticData->translation.has_value() || !pWorkingData->translation.has_value()) {
            pStaticData->translation = pTransform->GetTranslation();
        }
        else {
            auto&& delta = pTransform->GetTranslation() - pWorkingData->translation.value();

            if (!delta.Empty()) {
                pStaticData->translation.value() += delta;
            }
        }

        /// ------------------------------------------------------------------------------------------------------------

        if (!pStaticData->rotation.has_value() || !pWorkingData->rotation.has_value()) {
            pStaticData->rotation = pTransform->GetQuaternion();
        }
        else {
            auto&& delta = pTransform->GetQuaternion() * pWorkingData->rotation.value().Inverse();

            if (!delta.IsIdentity()) {
                pStaticData->rotation.value() *= delta;
            }
        }

        /// ------------------------------------------------------------------------------------------------------------
    }

    void AnimationPose::SetPose(AnimationClip* pClip) {
        auto&& channels = pClip->GetChannels();

        for (auto&& pChannel : channels) {
            for (auto&& [time, pKey] : pChannel->GetKeys()) {
                if (time > 0) {
                    continue;
                }

                auto&& pData = GetData(pChannel->GetChannelName());
                if (!pData) {
                    continue;
                }

                pKey->Set(1.f, pData);
            }
        }
    }*/
}