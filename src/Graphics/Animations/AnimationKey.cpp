//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/AnimationGraph.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform.h>

namespace SR_ANIMATIONS_NS {
    AnimationKey::AnimationKey(AnimationChannel* pChannel)
        : m_channel(pChannel)
    { }

    void TranslationKey::Update(double_t progress, AnimationKey* pPreviousKey, ChannelUpdateContext& context) noexcept {
        if (!context.gameObjectIndex.has_value()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        AnimationGameObjectData& data = context.pPose->GetGameObjectData(context.gameObjectIndex.value());

        //AnimationGameObjectData* pData = nullptr;
        //for (int i = 0; i < context.pGraph->m_gameObjects.size(); ++i) {
        //    if (context.pGraph->m_gameObjects[i]->GetName() == m_channel->GetGameObjectName()) {
        //        pData = &context.pGraph->m_testGameObjectsData[i];
        //        break;
        //    }
        //}
        //if (!pData) {
        //    return;
        //}
        //AnimationGameObjectData& data = *pData;

        if (!pPreviousKey || SR_EQUALS(progress, 1.0)) SR_UNLIKELY_ATTRIBUTE {
            data.translation = m_delta;
        }
        else {
            data.translation = static_cast<TranslationKey*>(pPreviousKey)->m_delta.Lerp(m_delta, progress);
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

     /*void TranslationKey::Update(double_t progress, float_t weight, AnimationKey* pPreviousKey, AnimationData* pData, AnimationData* pStaticData) noexcept {
       if (!pStaticData->translation.has_value()) {
            return;
        }

        if (!pData->translation.has_value()) {
            pData->translation = SR_MATH_NS::FVector3::Zero();
        }

        if (auto&& pKey = pPreviousKey ? pPreviousKey->GetTranslation() : nullptr) {
            auto&& newValue = (pKey->m_delta + pStaticData->translation.value()).Lerp(pStaticData->translation.value() + m_delta, progress);
            pData->translation = pData->translation->Lerp(newValue, weight);
        }
        else {
            pData->translation = pData->translation.value().Lerp(pStaticData->translation.value() + m_delta, weight);
        }
    }*/

    /*void TranslationKey::Set(float_t weight, AnimationData *pData) noexcept {
        if (!pData->translation.has_value()) {
            pData->translation = SR_MATH_NS::FVector3::Zero();
        }

        pData->translation = pData->translation.value().Lerp(m_translation, weight);
    }*/

    /// ----------------------------------------------------------------------------------------------------------------

    void RotationKey::Update(double_t progress, AnimationKey* pPreviousKey, ChannelUpdateContext& context) noexcept {
        if (!context.gameObjectIndex.has_value()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        AnimationGameObjectData& data = context.pPose->GetGameObjectData(context.gameObjectIndex.value());

        if (!pPreviousKey || SR_EQUALS(progress, 1.0)) SR_UNLIKELY_ATTRIBUTE {
            data.rotation = m_delta;
        }
        else {
            data.rotation = static_cast<RotationKey*>(pPreviousKey)->m_delta.Slerp(m_delta, progress);
        }
    }

    /*ivoid RotationKey::Set(float_t weight, AnimationData *pData) noexcept {
        f (!pData->rotation.has_value()) {
            pData->rotation = SR_MATH_NS::Quaternion::Identity();
        }

        pData->rotation = pData->rotation.value().Slerp(m_rotation, weight);
    }*/

    /// ----------------------------------------------------------------------------------------------------------------

    void ScalingKey::Update(double_t progress, AnimationKey* pPreviousKey, ChannelUpdateContext& context) noexcept {
        if (!context.gameObjectIndex.has_value()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        AnimationGameObjectData& data = context.pPose->GetGameObjectData(context.gameObjectIndex.value());

        if (!pPreviousKey || SR_EQUALS(progress, 1.0)) SR_UNLIKELY_ATTRIBUTE {
            data.scale = m_delta;
        }
        else {
            data.scale = static_cast<ScalingKey*>(pPreviousKey)->m_delta.Lerp(m_delta, progress);
        }
    }

    /*void ScalingKey::Set(float_t weight, AnimationData* pData) noexcept {
        if (!pData->scale.has_value()) {
            pData->scale = SR_MATH_NS::FVector3::One();
        }

        pData->scale = pData->scale->Lerp(m_scaling, weight);
    }*/
}
