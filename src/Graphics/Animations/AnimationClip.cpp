//
// Created by Monika on 08.01.2023.
//

#include <Graphics/Animations/AnimationClip.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/Common/StringUtils.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#ifdef SR_UTILS_ASSIMP
    #include <assimp/Importer.hpp>
    #include <assimp/scene.h>
#endif

#include <Codegen/AnimationClip.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationClip::AnimationClip() = default;

    AnimationClip::~AnimationClip() {
        m_channels.clear();
    }

    bool AnimationClip::LoadChannels(SR_HTYPES_NS::RawMesh* pRawMesh, SR_UTILS_NS::StringAtom name) {
    #ifdef SR_UTILS_ASSIMP
        const aiAnimation* pAnimation = nullptr;

        const aiScene* pScene = static_cast<const aiScene*>(pRawMesh->GetAssimpScene());

        for (uint32_t i = 0; i < pScene->mNumAnimations; ++i) {
            auto&& animationName = pScene->mAnimations[i]->mName;
            if (name == std::string_view(animationName.C_Str(), animationName.length)) {
                pAnimation = pScene->mAnimations[i];
                break;
            }
        }

        if (!pAnimation) {
            return false;
        }

        for (uint32_t channelIndex = 0; channelIndex < pAnimation->mNumChannels; ++channelIndex) {
            auto&& pChannel = pAnimation->mChannels[channelIndex];
            auto&& channelName = SR_UTILS_NS::StringAtom(std::string_view(pChannel->mNodeName.C_Str(), pChannel->mNodeName.length));
            if (channelName.empty()) {
                SR_LOG("AnimationClip::LoadChannels() : channel name is empty! Animation name: {}, Channel index: {}", name, channelIndex);
                continue;
            }

            const uint64_t lastSize = m_channels.size();
            AnimationChannel::Load(pChannel, static_cast<float_t>(pAnimation->mTicksPerSecond), m_channels);
            for (uint64_t i = lastSize; i < m_channels.size(); ++i) {
                m_channels[i].SetName(channelName);
            }
        }

        RetargetChannels();
        PostProcess();

        //auto&& pIt = std::ranges::remove_if(m_channels, [](const AnimationChannel& channel) {
        //    return !channel.IsValid();
        //});
        //m_channels.erase(pIt.begin(), pIt.end());

        return true;
    #else
        return false;
    #endif
    }

    bool AnimationClip::Unload() {
        m_channels.clear();
        m_retargetedChannels.clear();

        m_maxKeyFrame = 0;
        m_duration = 0.f;

        return Super::Unload();
    }

    void AnimationClip::OnAssetLoaded() {
        SR_TRACY_ZONE;

        if (m_clipPath.empty()) {
            SR_ERROR("AnimationClip::Load() : clip path is empty!");
            return;
        }

        SR_HTYPES_NS::RawMeshParams params;
        params.animation = true;

        auto&& pRawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(m_clipPath, &params);
        if (!pRawMesh) {
            SR_ERROR("AnimationClip::Load() : failed to load raw mesh from path: {}", m_clipPath);
            return;
        }

        if (!LoadChannels(pRawMesh.Get(), m_clipName)) {
            std::string animations;
        #ifdef SR_UTILS_ASSIMP
            const aiScene* pScene = static_cast<const aiScene*>(pRawMesh->GetAssimpScene());
            for (uint32_t i = 0; i < pScene->mNumAnimations; ++i) {
                animations += pScene->mAnimations[i]->mName.C_Str();
                if (i < pScene->mNumAnimations - 1) {
                    animations += ", ";
                }
            }
        #endif
            pRawMesh->CheckResourceUsage();
            SR_ERROR("AnimationClip::Load() : wrong animation name \"{}\"!\n\tTotal animations: {}", m_clipName, animations);
            return;
        }

        pRawMesh->CheckResourceUsage();

        for (auto&& channel : m_channels) {
            m_maxKeyFrame = SR_MAX(m_maxKeyFrame, channel.GetKeys().size());
            for (auto&& key : channel.GetKeys()) {
                m_duration = SR_MAX(m_duration, key.time);
            }
        }

        Super::OnAssetLoaded();
    }

    SR_UTILS_NS::StringAtom AnimationClip::GetClipName() const noexcept {
        return m_clipName;
    }

    void AnimationClip::PostProcess() {
        SR_TRACY_ZONE;

        for (SR_UTILS_NS::StringAtom excludedBone : m_excludedBones) {
            auto pIt = std::remove_if(
                m_channels.begin(),
                m_channels.end(),
                [&](const AnimationChannel& channel) {
                    return channel.GetChannelName() == excludedBone;
                }
            );
            m_channels.erase(pIt, m_channels.end());
        }
    }

    void AnimationClip::RetargetChannels() {
        SR_TRACY_ZONE;

        auto&& pRig = m_rig.GetResource();
        if (!pRig) {
            auto&& pSkeleton = m_skeleton.GetRawMesh();
            if (!pSkeleton) {
                SR_ERROR("AnimationClip::RetargetChannels() : both rig and skeleton are not set for clip \"{}\"!", GetResourceId());
                return;
            }

            for (auto&& channel : m_channels) {
                const auto boneIndex= pSkeleton->GetBoneInfo(m_skeleton.GetMeshId(), channel.GetChannelName()).boneId;
                if (boneIndex.has_value()) {
                    channel.SetBoneIndex(boneIndex.value());
                }
            }
            return;
        }

        /*for (auto&& channel : m_channels) {
            SR_UTILS_NS::StringAtom outName;
            auto&& pChain = pRig->RetargetBone(channel.GetChannelName(), outName);
            if (!pChain) {
                continue;
            }

            auto&& boneInfo = pChain->bones.front();

            channel.SetName(outName);
            channel.SetBoneIndex(boneInfo.index);

            /// substract the bind pose from the animation keys
            for (UnionAnimationKey& key : channel.GetKeys()) {
                switch (key.type) {
                    case AnimationKeyType::Translation: {
                        auto&& translation = key.data.translation.translation;
                        translation -= boneInfo.bindTranslation;
                        break;
                    }
                    case AnimationKeyType::Rotation: {
                        auto&& rotation = key.data.rotation.rotation;
                        rotation = boneInfo.bindRotation.Inverse() * rotation;
                        break;
                    }
                    case AnimationKeyType::Scaling: {
                        auto&& scale = key.data.scaling.scaling;
                        scale /= boneInfo.bindScale;
                        break;
                    }
                    default: {
                        SRHalt("AnimationClip::RetargetChannels() : unknown key type!");
                        break;
                    }
                }
            }
        }*/
    }

    const AnimationClip::Channels& AnimationClip::GetChannels(const SkeletonRig* pTargetRig) const {
        SR_TRACY_ZONE;

        auto&& pSourceRig = m_rig.GetResource();
        if (!pTargetRig || !pSourceRig) {
            return m_channels;
        }

        const SR_UTILS_NS::StringAtom targetRigName = pTargetRig->GetResourceId();
        if (auto&& pIt = m_retargetedChannels.find(targetRigName); pIt != m_retargetedChannels.end()) {
            return pIt->second;
        }

        m_retargetedChannels[targetRigName] = m_channels;
        Channels& channels = m_retargetedChannels[targetRigName];

        /** formula:
         *  prepare offsets: offset = bindTarget * inverse(bindSource)
         *  and when animating: key = offset * key * inverse(offset)
        */

        for (auto&& channel : channels) {
            SR_UTILS_NS::StringAtom sourceName;
            auto&& pSourceChain = pSourceRig->RetargetBone(channel.GetChannelName(), sourceName);
            if (!pSourceChain) {
                continue;
            }
            auto&& pTargetChain = pTargetRig->GetBoneChain(sourceName);
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
            const SR_MATH_NS::Quaternion qOffset = targetBoneInfo.bindRotation * sourceBoneInfo.bindRotation.Inverse();
            //const SR_MATH_NS::FVector3 sOffset = targetBoneInfo.bindScale / sourceBoneInfo.bindScale;

            for (UnionAnimationKey& key : channel.GetKeys()) {
                switch (key.type) {
                    case AnimationKeyType::Rotation: {
                        //auto&& rotation = key.data.rotation.rotation;
                        //rotation = qOffset * rotation * qOffset.Inverse();

                        auto& rotation = key.data.rotation.rotation;
                        /// delta относительно bind позы источника
                        const SR_MATH_NS::Quaternion delta = sourceBindR.Inverse() * rotation;
                        /// применяем к bind позе цели
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
                        const SR_MATH_NS::FVector3 delta = translation - sourceBindT;
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

            /*for (UnionAnimationKey& key : channel.GetKeys()) {
                switch (key.type) {
                    case AnimationKeyType::Translation: {
                        auto&& translation = key.data.translation.translation;
                        translation -= sourceBoneInfo.bindTranslation;
                        break;
                    }
                    case AnimationKeyType::Rotation: {
                        auto&& rotation = key.data.rotation.rotation;
                        rotation = sourceBoneInfo.bindRotation.Inverse() * rotation;
                        break;
                    }
                    case AnimationKeyType::Scaling: {
                        auto&& scale = key.data.scaling.scaling;
                        scale /= sourceBoneInfo.bindScale;
                        break;
                    }
                    default: {
                        SRHalt("AnimationClip::RetargetChannels() : unknown key type!");
                        break;
                    }
                }
            }

            for (UnionAnimationKey& key : channel.GetKeys()) {
                switch (key.type) {
                    case AnimationKeyType::Translation: {
                        auto&& translation = key.data.translation.translation;
                        translation += targetBoneInfo.bindTranslation;
                        break;
                    }
                    case AnimationKeyType::Rotation: {
                        auto&& rotation = key.data.rotation.rotation;
                        rotation = targetBoneInfo.bindRotation * rotation;
                        break;
                    }
                    case AnimationKeyType::Scaling: {
                        auto&& scale = key.data.scaling.scaling;
                        scale *= targetBoneInfo.bindScale;
                        break;
                    }
                    default: {
                        SRHalt("AnimationClip::RetargetChannels() : unknown key type!");
                        break;
                    }
                }
            }*/
        }

        channels.erase_if([](const AnimationChannel& channel) {
            return !channel.IsValid();
        });

        return channels;
    }
}
