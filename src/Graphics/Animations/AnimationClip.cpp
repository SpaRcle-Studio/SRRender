//
// Created by Monika on 08.01.2023.
//

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <Graphics/Animations/AnimationClip.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Types/RawMesh.h>

namespace SR_ANIMATIONS_NS {
    AnimationClip::AnimationClip()
        : Super(SR_COMPILE_TIME_CRC32_TYPE_NAME(AnimationClip))
    { }

    AnimationClip::~AnimationClip() {
        for (auto&& pChannel : m_channels) {
            delete pChannel;
        }
        m_channels.clear();
    }

    AnimationClip* AnimationClip::Load(const SR_UTILS_NS::Path &rawPath, SR_UTILS_NS::StringAtom name) {
        SR_GLOBAL_LOCK

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();
        auto&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(resourceManager.GetResPath());

        if (!resourceManager.GetResPath().Concat(path).Exists(SR_UTILS_NS::Path::Type::File)) {
            SR_ERROR("AnimationClip::Load() : animation \"{}\" isn't exists!", rawPath.ToStringRef());
            return nullptr;
        }

        AnimationClip* pAnimationClip = nullptr;

        resourceManager.Execute([&]() {
            auto&& resourceId = path.GetExtensionView() == "animation" ? path.ToString() :
                name.ToStringRef() + SR_UTILS_NS::RESOURCE_ID_SEPARATOR.ToStringRef() + path.ToString();

            if (auto&& pResource = SR_UTILS_NS::ResourceManager::Instance().Find<AnimationClip>(path)) {
                pAnimationClip = pResource;
                return;
            }

            pAnimationClip = new AnimationClip();
            pAnimationClip->SetId(resourceId, false /** auto register */);

            if (!pAnimationClip->Reload()) {
                SR_ERROR("AnimationClip::Load() : failed to load animation clip! \n\tPath: " + path.ToString());
                pAnimationClip->DeleteResource();
                pAnimationClip = nullptr;
                return;
            }

            /// отложенная ручная регистрация
            SR_UTILS_NS::ResourceManager::Instance().RegisterResource(pAnimationClip);
        });

        return pAnimationClip;
    }

    std::vector<AnimationClip*> AnimationClip::Load(const SR_UTILS_NS::Path& rawPath) {
        std::vector<AnimationClip*> animations;

        SR_HTYPES_NS::RawMeshParams params;
        params.animation = true;

        auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(rawPath, params);
        if (!pRawMesh) {
            return animations;
        }

        auto&& animationNames = pRawMesh->GetAnimationNames();
        for (auto&& name : animationNames) {
            auto&& pAnimationClip = Load(rawPath, name);
            animations.emplace_back(pAnimationClip);
        }

        if (animations.empty()) {
            SR_ERROR("AnimationClip::Load() : failed to load animation clips! Path: " + rawPath.ToString());
        }

        return animations;
    }

    bool AnimationClip::LoadChannels(SR_HTYPES_NS::RawMesh* pRawMesh, const std::string& name) {
        const aiAnimation* pAnimation = nullptr;

        for (uint32_t i = 0; i < pRawMesh->GetAssimpScene()->mNumAnimations; ++i) {
            if (pRawMesh->GetAssimpScene()->mAnimations[i]->mName.C_Str() == name) {
                pAnimation = pRawMesh->GetAssimpScene()->mAnimations[i];
                break;
            }
        }

        if (!pAnimation) {
            return false;
        }

        for (uint32_t channelIndex = 0; channelIndex < pAnimation->mNumChannels; ++channelIndex) {
            AnimationChannel::Load(
                pRawMesh,
                pAnimation->mChannels[channelIndex],
                static_cast<float_t>(pAnimation->mTicksPerSecond),
                m_channels
            );
        }

        return true;
    }

    bool AnimationClip::Unload() {
        for (auto&& pChannel : m_channels) {
            delete pChannel;
        }
        m_channels.clear();

        m_maxKeyFrame = 0;
        m_duration = 0.f;

        return Super::Unload();
    }

    bool AnimationClip::Load() {
        SR_TRACY_ZONE;

        auto&& resourceId = GetResourceId();

        if (SR_UTILS_NS::StringUtils::GetExtensionFromFilePath(resourceId) == "animation") {
            SRHalt("TODO!");
        }
        else {
            auto&& [animationName, rawPath] = SR_UTILS_NS::StringUtils::SplitTwo(
                resourceId,
                SR_UTILS_NS::RESOURCE_ID_SEPARATOR.ToStringRef()
            );

            SR_HTYPES_NS::RawMeshParams params;
            params.animation = true;

            auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(rawPath, params);
            if (!pRawMesh) {
                return false;
            }

            if (!LoadChannels(pRawMesh, animationName)) {
                std::string animations;
                for (uint32_t i = 0; i < pRawMesh->GetAssimpScene()->mNumAnimations; ++i) {
                    animations += pRawMesh->GetAssimpScene()->mAnimations[i]->mName.C_Str();
                    if (i < pRawMesh->GetAssimpScene()->mNumAnimations - 1) {
                        animations += ", ";
                    }
                }
                SR_ERROR("AnimationClip::Load() : wrong animation name \"{}\"!\n\tTotal animations: {}", animationName, animations);
                return false;
            }
        }

        for (auto&& pChannel : GetChannels()) {
            m_maxKeyFrame = SR_MAX(m_maxKeyFrame, pChannel->GetKeys().size());
            for (auto&& key : pChannel->GetKeys()) {
                m_duration = SR_MAX(m_duration, key.time);
            }
        }

        return Super::Load();
    }

    SR_UTILS_NS::Path AnimationClip::InitializeResourcePath() const {
        return SR_UTILS_NS::Path(SR_UTILS_NS::StringUtils::SubstringView(
            GetResourceId(),
            SR_UTILS_NS::RESOURCE_ID_SEPARATOR,
            SR_UTILS_NS::RESOURCE_ID_SEPARATOR.size()
        ));
    }

    SR_UTILS_NS::StringAtom AnimationClip::GetClipName() const noexcept {
        auto&& resourceId = GetResourceId();
        if (resourceId.empty()) {
            SR_ERROR("AnimationClip::GetClipName() : resource id is empty!");
            return SR_UTILS_NS::StringAtom();
        }

        auto&& [animationName, rawPath] = SR_UTILS_NS::StringUtils::SplitTwo(
            resourceId,
            SR_UTILS_NS::RESOURCE_ID_SEPARATOR.ToStringRef()
        );
        return animationName;
    }
}
