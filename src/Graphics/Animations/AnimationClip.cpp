//
// Created by Monika on 08.01.2023.
//

#ifdef SR_UTILS_ASSIMP
    #include <assimp/Importer.hpp>
    #include <assimp/scene.h>
#endif

#include <Graphics/Animations/AnimationClip.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Types/RawMesh.h>

#include <Codegen/AnimationClip.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationClip::AnimationClip() = default;

    AnimationClip::~AnimationClip() {
        for (auto&& pChannel : m_channels) {
            delete pChannel;
        }
        m_channels.clear();
    }

    AnimationClip::Ptr AnimationClip::Load(const SR_UTILS_NS::Path& rawPath, const SR_UTILS_NS::Path& skeleton, SR_UTILS_NS::StringAtom name) {
        SR_GLOBAL_LOCK

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();
        auto&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(resourceManager.GetResPath());

        if (!resourceManager.GetResPath().Concat(path).Exists(SR_UTILS_NS::Path::Type::File)) {
            SR_ERROR("AnimationClip::Load() : animation \"{}\" isn't exists!", rawPath.ToStringRef());
            return nullptr;
        }

        AnimationClip::Ptr pAnimationClip = nullptr;

        resourceManager.Execute([&]() {
            auto&& resourceId = path.GetExtensionView() == "animation" ? path.ToString() :
                name.ToStringRef() + SR_UTILS_NS::RESOURCE_ID_SEPARATOR.ToStringRef() + path.ToString();

            if (auto&& pResource = SR_UTILS_NS::ResourceManager::Instance().Find<AnimationClip>(path)) {
                pAnimationClip = pResource;
                return;
            }

            pAnimationClip = AnimationClip::MakeShared<AnimationClip>();
            pAnimationClip->SetId(resourceId, false /** auto register */);
            pAnimationClip->m_skeletonPath = SR_UTILS_NS::Path(skeleton).RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

            if (!pAnimationClip->Reload()) {
                SR_ERROR("AnimationClip::Load() : failed to load animation clip! \n\tPath: " + path.ToString());
                pAnimationClip->DeleteResource();
                pAnimationClip = nullptr;
                return;
            }

            /// отложенная ручная регистрация
            SR_UTILS_NS::ResourceManager::Instance().RegisterResource(pAnimationClip.StaticCast<SR_UTILS_NS::IResource>());
        });

        return pAnimationClip;
    }

    std::vector<AnimationClip::Ptr> AnimationClip::Load(const SR_UTILS_NS::Path& rawPath, const SR_UTILS_NS::Path& skeleton) {
        std::vector<AnimationClip::Ptr> animations;

        SR_HTYPES_NS::RawMeshParams params;
        params.animation = true;

        auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(rawPath, params);
        if (!pRawMesh) {
            return animations;
        }

        auto&& animationNames = pRawMesh->GetAnimationNames();
        for (auto&& name : animationNames) {
            auto&& pAnimationClip = Load(rawPath, skeleton, name);
            animations.emplace_back(pAnimationClip);
        }

        if (animations.empty()) {
            SR_ERROR("AnimationClip::Load() : failed to load animation clips! Path: " + rawPath.ToString());
        }

        return animations;
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

        return true;
    #else
        return false;
    #endif
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

            SR_HTYPES_NS::RawMesh::Ptr pSkeletonMesh = nullptr;

            if (m_skeletonPath.empty() || m_skeletonPath == rawPath) {
                pSkeletonMesh = pRawMesh;
            }
            else if (!m_skeletonPath.empty()) {
                pSkeletonMesh = SR_HTYPES_NS::RawMesh::Load(m_skeletonPath);
                if (!pSkeletonMesh) {
                    pRawMesh->CheckResourceUsage();
                    return false;
                }
            }

            if (!LoadChannels(pRawMesh.Get(), pSkeletonMesh.Get(), animationName)) {
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
                SR_ERROR("AnimationClip::Load() : wrong animation name \"{}\"!\n\tTotal animations: {}", animationName, animations);
                return false;
            }

            pRawMesh->CheckResourceUsage();
            pSkeletonMesh->CheckResourceUsage();
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
