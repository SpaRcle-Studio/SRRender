//
// Created by Monika on 18.06.2026.
//

#include <Graphics/Animations/RetargetAnimation.h>
#include <Graphics/Animations/SkeletonRig.h>

namespace SR_ANIMATIONS_NS {
    SR_NODISCARD bool RetargetAnimation::Retarget(
        const SkeletonRig& sourceRig,
        const SkeletonRig& targetRig,
        const Channels& sourceChannels,
        Channels& outTargetChannels
    ) {
        /** formula:
         *  prepare offsets: offset = bindTarget * inverse(bindSource)
         *  and when animating: key = offset * key * inverse(offset)
        */

        /// Иногда риги импортируются в разных глобальных базисах (разные FBX пайплайны),
        /// и тогда локальные оси костей не совпадают. Приводим источник в базис цели
        /// через bind-поворот "Hips" как опорной кости.
        SR_MATH_NS::Quaternion rootBasis = SR_MATH_NS::Quaternion::Identity();
        if (auto&& pSourceHips = sourceRig.GetBoneChain(SR_UTILS_NS::StringAtom("Hips"))) {
            if (auto&& pTargetHips = targetRig.GetBoneChain(SR_UTILS_NS::StringAtom("Hips"))) {
                const auto& srcHipsR = pSourceHips->bones.front().bindRotation;
                const auto& tgtHipsR = pTargetHips->bones.front().bindRotation;
                rootBasis = tgtHipsR * srcHipsR.Inverse();
            }
        }
        const SR_MATH_NS::Quaternion rootBasisInv = rootBasis.Inverse();

        outTargetChannels = sourceChannels;

        for (auto&& channel : outTargetChannels) {
            SR_UTILS_NS::StringAtom sourceName;
            auto&& pSourceChain = sourceRig.RetargetBone(channel.GetChannelName(), sourceName);
            if (!pSourceChain) {
                continue;
            }
            auto&& pTargetChain = targetRig.GetBoneChain(sourceName);
            if (!pTargetChain) {
                continue;
            }

            auto&& sourceBoneInfo = pSourceChain->bones.front();
            auto&& targetBoneInfo = pTargetChain->bones.front();

            channel.SetName(targetBoneInfo.name);
            channel.SetBoneIndex(targetBoneInfo.index);

            const auto& sourceBindT = sourceBoneInfo.bindTranslation;
            const auto& sourceBindR = sourceBoneInfo.bindRotation;
            const auto& sourceBindS = sourceBoneInfo.bindScale;

            const auto& targetBindT = targetBoneInfo.bindTranslation;
            const auto& targetBindR = targetBoneInfo.bindRotation;
            const auto& targetBindS = targetBoneInfo.bindScale;



            //const auto Bs = sourceBoneInfo.bindRotation;
            //const auto Bt = targetBoneInfo.bindRotation;

            //// conversion between spaces
            //const auto C = Bt * Bs.Inverse();


            //const SR_MATH_NS::FVector3 tOffset = targetBoneInfo.bindTranslation - sourceBoneInfo.bindTranslation;
            const SR_MATH_NS::Quaternion sourceBindRAdj = rootBasis * sourceBindR * rootBasisInv;
            const SR_MATH_NS::FVector3 sourceBindTAdj = sourceBindT.Rotate(rootBasis);

            const SR_MATH_NS::Quaternion qOffset = targetBindR * sourceBindRAdj.Inverse();
            //const SR_MATH_NS::FVector3 sOffset = targetBoneInfo.bindScale / sourceBoneInfo.bindScale;

            for (UnionAnimationKey& key : channel.GetKeys()) {
                switch (key.type) {
                    case AnimationKeyType::Rotation: {
                        //auto&& rotation = key.data.rotation.rotation;
                        //rotation = qOffset * rotation * qOffset.Inverse();

                        auto& rotation = key.data.rotation.rotation;
                        /// приводим ключ источника в базис цели (глобально, через hips)
                        const SR_MATH_NS::Quaternion rotationAdj = rootBasis * rotation * rootBasisInv;

                        /// delta относительно bind позы источника
                        SR_MATH_NS::Quaternion delta = sourceBindRAdj.Inverse() * rotationAdj;

                        /// конвертация базиса: source local -> target local
                        /// (иначе при разных осях костей дельта крутится "не туда")
                        const SR_MATH_NS::Quaternion basis = targetBindR.Inverse() * sourceBindRAdj;
                        delta = basis * delta * basis.Inverse();

                        /// применяем дельту к bind позе цели
                        rotation = targetBindR * delta;


                        // animation delta in source space
                        //const auto Rdelta = Bs.Inverse() * rotation;
                        //rotation = Bt * C * Rdelta * C.Inverse();

                        break;
                    }
                    case AnimationKeyType::Translation: {
                        //auto&& translation = key.data.translation.translation;
                        //translation += tOffset;


                        auto& translation = key.data.translation.translation;
                        const SR_MATH_NS::FVector3 translationAdj = translation.Rotate(rootBasis);
                        const SR_MATH_NS::FVector3 delta = translationAdj - sourceBindTAdj;
                        translation = targetBindT + delta.Rotate(qOffset);

                        break;
                    }
                    case AnimationKeyType::Scaling: {
                        //auto&& scale = key.data.scaling.scaling;
                        //scale *= sOffset;

                        auto& scale = key.data.scaling.scaling;
                        const SR_MATH_NS::FVector3 delta = scale / sourceBindS;
                        scale = targetBindS * delta;
                        break;
                    }
                    default: {
                        SRHalt("AnimationClip::RetargetChannels() : unknown key type!");
                        break;
                    }
                }
            }

        }

        outTargetChannels.erase_if([](const AnimationChannel& channel) {
            return !channel.IsValid();
        });

        return true;
    }
}