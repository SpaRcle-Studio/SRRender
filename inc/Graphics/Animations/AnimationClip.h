//
// Created by Monika on 08.01.2023.
//

#ifndef SR_ENGINE_ANIMATIONCLIP_H
#define SR_ENGINE_ANIMATIONCLIP_H

#include <Utils/Resources/IResource.h>

class aiAnimation;

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_ANIMATIONS_NS {
    class AnimationChannel;

    class AnimationClip : public SR_UTILS_NS::IResource {
        using Super = SR_UTILS_NS::IResource;
    public:
        AnimationClip();
        ~AnimationClip() override;

    public:
        static std::vector<AnimationClip*> Load(const SR_UTILS_NS::Path& path);
        static AnimationClip* Load(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom name);

    public:
        SR_NODISCARD const std::vector<AnimationChannel*>& GetChannels() const { return m_channels; }
        SR_NODISCARD bool IsAllowedToRevive() const override { return true; }

        SR_NODISCARD SR_UTILS_NS::Path InitializeResourcePath() const override;

        SR_NODISCARD SR_UTILS_NS::StringAtom GetClipName() const noexcept;

        SR_NODISCARD float_t GetDuration() const noexcept { return m_duration; }
        SR_NODISCARD uint32_t GetMaxKeyFrame() const noexcept { return m_maxKeyFrame; }

    protected:
        bool Unload() override;
        bool Load() override;

    private:
        SR_NODISCARD bool LoadChannels(SR_HTYPES_NS::RawMesh* pRawMesh, const std::string& name);

    private:
        std::vector<AnimationChannel*> m_channels;

        float_t m_duration = 0.f;
        uint32_t m_maxKeyFrame = 0;

    };
}

#endif //SR_ENGINE_ANIMATIONCLIP_H
