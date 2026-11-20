//
// Created by Monika on 08.01.2023.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATION_CHANNEL_H
#define SR_ENGINE_GRAPHICS_ANIMATION_CHANNEL_H

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/AnimationContext.h>

struct aiNodeAnim;

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_ANIMATIONS_NS {
    class AnimationKey;
    class AnimationPose;

    class KeysPool : public SR_UTILS_NS::NonCopyable {
    public:
        KeysPool(uint32_t count);
        ~KeysPool() override;

    public:
        SR_NODISCARD std::span<UnionAnimationKey> AllocateUnionKeys(uint32_t count);

    private:
        uint32_t m_count = 0;
        std::atomic<uint32_t> m_offset = 0;
        void* m_pKeys = nullptr;

    };

    struct ExcludeBoneTransforms : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
        /// @property
        bool translation = true;
        /// @property
        bool rotation = true;
        /// @property
        bool scale = true;

    };

    SR_ENUM_NS_CLASS_T(AnimationChannelType, uint8_t,
        None,
        Translation,
        Rotation,
        Scaling
    );

    class AnimationChannel final {
        using Keys = std::span<UnionAnimationKey>;
        using Excluded = SR_UTILS_NS::Map<SR_UTILS_NS::StringAtom, ExcludeBoneTransforms>;
    public:
        ~AnimationChannel();

    public:
        struct LoadContext {
            LoadContext(const Excluded& ex, SR_UTILS_NS::Vector<AnimationChannel>& chn, KeysPool& pl)
                : excluded(ex)
                , channels(chn)
                , pool(pl)
            { }
            const Excluded& excluded;
            SR_UTILS_NS::Vector<AnimationChannel>& channels;
            KeysPool& pool;
            float_t invTicksPerSecond = 0.f;
            SR_HTYPES_NS::RawMesh* pSkeleton = nullptr;
            uint32_t meshId = SR_ID_INVALID;
        };
        static void Load(aiNodeAnim* pChannel, LoadContext& context);

        void SetName(SR_UTILS_NS::StringAtom name);
        void SetBoneIndex(uint16_t index) { m_boneIndex = index; }
        void SetOffset(const SR_MATH_NS::DecomposedMatrix& offset) { m_offset = offset; }

        SR_NODISCARD uint32_t UpdateChannel(uint32_t keyIndex, float_t time, ChannelAnimationUpdateContext& context, AnimationGameObjectData& data) const;
        SR_NODISCARD uint32_t UpdateChannelWithWeight(uint32_t keyIndex, float_t time, ChannelAnimationUpdateContext& context, AnimationGameObjectData& data) const;

    public:
        SR_NODISCARD const Keys& GetKeys() const { return m_keys; }
        SR_NODISCARD Keys& GetKeys() { return m_keys; }
        SR_NODISCARD AnimationChannelType GetType() const { return m_type; }

        void SetKeys(const Keys keys) { m_keys = keys; }
        void SetType(const AnimationChannelType type) { m_type = type; }

        SR_NODISCARD bool IsValid() const { return !m_keys.empty() && HasBoneIndex() && !m_name.empty(); }

        SR_NODISCARD SR_FORCE_INLINE SR_UTILS_NS::StringAtom GetChannelName() const noexcept { return m_name; }
        SR_NODISCARD SR_FORCE_INLINE uint16_t GetBoneIndex() const noexcept { return m_boneIndex; }
        SR_NODISCARD SR_FORCE_INLINE bool HasBoneIndex() const noexcept { return m_boneIndex != SR_UINT16_MAX; }

    private:
        std::optional<SR_MATH_NS::DecomposedMatrix> m_offset;
        uint16_t m_boneIndex = SR_UINT16_MAX;
        SR_UTILS_NS::StringAtom m_name;
        Keys m_keys;
        AnimationChannelType m_type = AnimationChannelType::None;

    };
}

#endif //SR_ENGINE_GRAPHICS_ANIMATION_CHANNEL_H
