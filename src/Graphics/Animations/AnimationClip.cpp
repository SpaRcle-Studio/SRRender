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

        auto&& pRig = m_rig.GetResource();
        if (auto&& pSkeleton = GetSkeleton()) {
            auto&& meshId = pRig ? pRig->GetSkeleton().GetMeshId() : m_skeleton.GetMeshId();
            for (auto&& channel : m_channels) {
                const auto boneIndex = pSkeleton->GetBoneInfo(meshId, channel.GetChannelName()).boneId;
                if (boneIndex.has_value()) {
                    channel.SetBoneIndex(boneIndex.value());
                }
            }
        }
        else {
            SR_ERROR("AnimationClip::LoadChannels() : failed to get skeleton for animation \"{}\"", name);
        }

        PostProcess();

        return true;
    #else
        return false;
    #endif
    }

    bool AnimationClip::Unload() {
        m_skeletonReloadSubscription.Reset();
        m_clipReloadSubscription.Reset();
        m_rigReloadSubscriptions.clear();
        m_channels.clear();
        m_retargetCache.clear();

        m_maxKeyFrame = 0;
        m_duration = 0.f;

        return Super::Unload();
    }

    void AnimationClip::DoLoadClip() {
        SR_TRACY_ZONE;

        auto&& pRawMesh = m_rawMesh.GetResource();
        if (!pRawMesh->CanBeUsed()) {
            return;
        }

        if (auto&& pSkeleton = GetSkeleton(); !pSkeleton || !pSkeleton->CanBeUsed()) {
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
            SR_ERROR("AnimationClip::Load() : wrong animation name \"{}\"!\n\tTotal animations: {}", m_clipName, animations);
            return;
        }

        for (auto&& channel : m_channels) {
            m_maxKeyFrame = SR_MAX(m_maxKeyFrame, channel.GetKeys().size());
            for (auto&& key : channel.GetKeys()) {
                m_duration = SR_MAX(m_duration, key.time);
            }
        }
    }

    void AnimationClip::OnAssetLoaded() {
        SR_TRACY_ZONE;

        if (m_clipPath.empty()) {
            return;
        }

        SR_HTYPES_NS::RawMeshParams params;
        params.animation = true;

        m_rawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(m_clipPath, &params);
        if (!m_rawMesh) {
            SR_ERROR("AnimationClip::Load() : failed to load raw mesh from path: {}", m_clipPath);
            return;
        }

        m_rigReloadSubscriptions.clear();
        if (m_rig.IsValid()) {
            auto&& pRig = m_rig.GetResource();
            m_rigReloadSubscriptions.emplace_back(pRig->Subscribe(RELOAD_DONE_EVENT, [this](auto&&) {
                m_retargetCache.clear();
            }));
        }

        for (auto&& profile : m_retargetProfiles) {
            if (profile.targetRig.IsValid()) {
                auto&& pTargetRig = profile.targetRig.GetResource();
                m_rigReloadSubscriptions.emplace_back(pTargetRig->Subscribe(RELOAD_DONE_EVENT, [this](auto&&) {
                    m_retargetCache.clear();
                }));
            }
        }

        auto&& pRawMesh = m_rawMesh.GetResource();
        auto&& pSkeleton = GetSkeleton();

        m_clipReloadSubscription = pRawMesh->Subscribe(RELOAD_DONE_EVENT, [this](auto&&) {
            DoLoadClip();
        });

        m_skeletonReloadSubscription = pSkeleton ? pSkeleton->Subscribe(RELOAD_DONE_EVENT, [this](auto&&) {
            DoLoadClip();
        }) : SR_UTILS_NS::Subscription();

        DoLoadClip();

        Super::OnAssetLoaded();
    }

    SR_UTILS_NS::StringAtom AnimationClip::GetClipName() const noexcept {
        return m_clipName;
    }

    void AnimationClip::PostProcess() {
        SR_TRACY_ZONE;

        if (m_startTime) {
            for (auto&& channel : m_channels) {
                for (auto&& key : channel.GetKeys()) {
                    key.time -= m_startTime.value();
                }
            }
            m_duration -= m_startTime.value();
            m_channels.erase_if([&](const AnimationChannel& channel) {
                return !channel.GetKeys().empty() && channel.GetKeys().back().time < 0.f;
            });
        }

        for (auto&& [name, transforms] : m_excludedBones) {
            for (auto&& channel : m_channels) {
                if (channel.GetChannelName() != name) {
                    continue;
                }
                channel.GetKeys().erase_if([&](const UnionAnimationKey& key) {
                    return (transforms.translation && key.type == AnimationKeyType::Translation)
                        || (transforms.rotation && key.type == AnimationKeyType::Rotation)
                        || (transforms.scale && key.type == AnimationKeyType::Scaling);
                });
            }
            m_channels.erase_if([&](const AnimationChannel& channel) {
                return channel.GetKeys().empty() && channel.GetChannelName() == name;
            });
        }

        for (auto&& [name, lock] : m_lockedBones) {
            for (auto&& channel : m_channels) {
                if (channel.GetChannelName() != name) {
                    continue;
                }
                for (auto&& key : channel.GetKeys()) {
                    if (lock.translation != SR_MATH_NS::Axis::None && key.type == AnimationKeyType::Translation) {
                        key.data.translation.translation = key.data.translation.translation.ZeroAxis(lock.translation);
                    }
                    if (lock.rotation != SR_MATH_NS::Axis::None && key.type == AnimationKeyType::Rotation) {
                        key.data.rotation.rotation = key.data.rotation.rotation.EulerAngle().ZeroAxis(lock.rotation).ToQuat();
                    }
                    if (lock.scale != SR_MATH_NS::Axis::None && key.type == AnimationKeyType::Scaling) {
                        key.data.scaling.scaling = key.data.scaling.scaling.ZeroAxis(lock.scale);
                    }
                }
            }
        }
    }

    const AnimationClip::Channels& AnimationClip::GetChannels(const SkeletonRig* pTargetRig) const {
        SR_TRACY_ZONE;

        auto&& pSourceRig = m_rig.GetResource();
        if (!pTargetRig || !pSourceRig) {
            return m_channels;
        }

        const SR_UTILS_NS::StringAtom sourceRigName = pSourceRig->GetResourceId();
        const SR_UTILS_NS::StringAtom targetRigName = pTargetRig->GetResourceId();

        if (sourceRigName == targetRigName) {
            return m_channels;
        }

        if (auto&& pIt = m_retargetCache.find(targetRigName); pIt != m_retargetCache.end()) {
            return pIt->second;
        }

        RetargetAnimationContext context(m_channels, *pSourceRig, *pTargetRig);

        for (auto&& profile : m_retargetProfiles) {
            if (profile.targetRig.GetId() != targetRigName) {
                continue;
            }
            if (profile.sourceRig.GetId() != sourceRigName && profile.sourceRig.IsValid()) {
                continue;
            }
            context.pProfile = &profile;
            break;
        }

        if (RetargetAnimationSystem::Instance().Retarget(context)) {
            SR_LOG("AnimationClip::GetChannels() : retargeted animation \"{}\" from rig \"{}\" to rig \"{}\"", GetResourceId(), sourceRigName, targetRigName);
            m_retargetCache[targetRigName] = std::move(context.targetChannels);
            return m_retargetCache[targetRigName];
        }

        SR_ERROR("AnimationClip::GetChannels() : failed to retarget animation \"{}\" from rig \"{}\" to rig \"{}\"", GetResourceId(), sourceRigName, targetRigName);

        m_retargetCache.emplace(targetRigName, m_channels);
        return m_retargetCache[targetRigName];
    }

    SR_HTYPES_NS::RawMesh* AnimationClip::GetSkeleton() const {
        auto&& pRig = m_rig.GetResource();
        auto&& pSkeleton = pRig ? pRig->GetSkeleton().GetRawMesh() : m_skeleton.GetRawMesh();
        return const_cast<SR_HTYPES_NS::RawMesh*>(pSkeleton.Get());
    }
}
