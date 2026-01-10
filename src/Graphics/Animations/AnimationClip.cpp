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

    bool AnimationClip::LoadChannels(SR_HTYPES_NS::RawMesh* pRawMesh, SR_HTYPES_NS::RawMesh* pSkeleton, const std::string& name) {
    #ifdef SR_UTILS_ASSIMP
        const aiAnimation* pAnimation = nullptr;

        const aiScene* pScene = static_cast<const aiScene*>(pRawMesh->GetAssimpScene());

        for (uint32_t i = 0; i < pScene->mNumAnimations; ++i) {
            if (pScene->mAnimations[i]->mName.C_Str() == name) {
                pAnimation = pScene->mAnimations[i];
                break;
            }
        }

        if (!pAnimation) {
            return false;
        }

        for (uint32_t channelIndex = 0; channelIndex < pAnimation->mNumChannels; ++channelIndex) {
            AnimationChannel::Load(
                pSkeleton,
                pAnimation->mChannels[channelIndex],
                static_cast<float_t>(pAnimation->mTicksPerSecond),
                m_channels
            );
        }

        PostProcess();

        return true;
    #else
        return false;
    #endif
    }

    bool AnimationClip::Unload() {
        m_channels.clear();

        m_maxKeyFrame = 0;
        m_duration = 0.f;

        return Super::Unload();
    }

    void AnimationClip::OnAssetLoaded() {
        SR_TRACY_ZONE;

        SR_HTYPES_NS::RawMeshParams params;
        params.animation = true;

        auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(m_clipPath, params);
        if (!pRawMesh) {
            SR_ERROR("AnimationClip::Load() : failed to load raw mesh from path: {}", m_clipPath);
            return;
        }

        SR_HTYPES_NS::RawMesh::Ptr pSkeletonMesh = nullptr;

        if (m_skeletonPath.empty() || m_skeletonPath == m_clipPath) {
            pSkeletonMesh = pRawMesh;
        }
        else if (!m_skeletonPath.empty()) {
            pSkeletonMesh = SR_HTYPES_NS::RawMesh::Load(m_skeletonPath);
            if (!pSkeletonMesh) {
                pRawMesh->CheckResourceUsage();
                SR_ERROR("AnimationClip::Load() : failed to load skeleton raw mesh from path: {}", m_skeletonPath);
                return;
            }
        }

        if (!LoadChannels(pRawMesh.Get(), pSkeletonMesh.Get(), m_clipName)) {
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
            pSkeletonMesh->CheckResourceUsage();
            SR_ERROR("AnimationClip::Load() : wrong animation name \"{}\"!\n\tTotal animations: {}", m_clipName, animations);
            return;
        }

        pRawMesh->CheckResourceUsage();
        pSkeletonMesh->CheckResourceUsage();

        for (auto&& channel : GetChannels()) {
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
}
