//
// Created by Monika on 25.04.2023.
//

#ifndef SR_ENGINE_ANIMATIONDATA_H
#define SR_ENGINE_ANIMATIONDATA_H

#include <Graphics/Animations/AnimationCommon.h>

namespace SR_ANIMATIONS_NS {
    /// Ключи меняют данные в рамках одного объекта, смешивая значения
    /// После чего, в конце кадра данные применяются на объект
    struct AnimationGameObjectData {
    public:
        void Reset() noexcept {
            translation.reset();
            rotation.reset();
            scaling.reset();
        }

    public:
        bool dirty = false;

        std::optional<SR_MATH_NS::FVector3> translation;
        std::optional<SR_MATH_NS::Quaternion> rotation;
        std::optional<SR_MATH_NS::FVector3> scaling;

        std::optional<bool> enable;
        std::optional<SR_UTILS_NS::StringAtom> layer;

    };
}

#endif //SR_ENGINE_ANIMATIONDATA_H
