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
        //void Reset() noexcept {
        //    translation.reset();
        //    rotation.reset();
        //    scaling.reset();
        //    enable.reset();
        //    layer.reset();
        //    dirty = false;
        //}

    public:
        bool dirty = false;

        std::optional<SR_MATH_NS::FVector3> translation;
        std::optional<SR_MATH_NS::Quaternion> rotation;
        std::optional<SR_MATH_NS::FVector3> scaling;

        std::optional<bool> enable;
        std::optional<SR_UTILS_NS::StringAtom> layer;

    };

    static AnimationGameObjectData Merge(AnimationGameObjectData& from, AnimationGameObjectData& to, float_t weight) {
        SR_TRACY_ZONE;

        AnimationGameObjectData result;
        result.dirty = true;

        //if (from.translation.has_value() && to.translation.has_value()) {
        //    result.translation = from.translation.value().Lerp(to.translation.value(), weight);
        //} else if (from.translation.has_value()) {
        //    result.translation = from.translation.value().
        //} else if (to.translation.has_value()) {
        //    result.translation = to.translation.value();
        //}
        return result;
    }
}

#endif //SR_ENGINE_ANIMATIONDATA_H
