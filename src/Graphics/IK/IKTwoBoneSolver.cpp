//
// Created by Monika on 28.11.2025.
//

#include <Graphics/IK/IKTwoBoneSolver.h>

#include <Utils/ECS/Transform.h>

namespace SR_GRAPH_NS::IK {
    void InitializeTwoBoneIKState(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const SR_UTILS_NS::Transform* pHint,
        IKTwoBoneState& state
    ) {
        if (state.initialized) {
            return;
        }

        state.upperArmLength = SR_MATH_NS::FVector3::Distance(root.GetGlobalTranslation(), mid.GetGlobalTranslation());
        state.lowerArmLength = SR_MATH_NS::FVector3::Distance(mid.GetGlobalTranslation(), tip.GetGlobalTranslation());
        state.armLength = state.upperArmLength + state.lowerArmLength;

        // Сохраняем исходные локальные направления
        state.rootToMidLocal = root.InverseTransformDirection(mid.GetGlobalTranslation() - root.GetGlobalTranslation()).Normalized();
        state.midToTipLocal = mid.InverseTransformDirection(tip.GetGlobalTranslation() - mid.GetGlobalTranslation()).Normalized();

        state.rootInitialRotation = root.GetGlobalRotation();
        state.midInitialRotation = mid.GetGlobalRotation();

        state.lastRootRotation = root.GetGlobalRotation();
        state.lastMidRotation = mid.GetGlobalRotation();

        state.initialized = true;
    }

    void SolveTwoBoneExtendedTarget(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        const SR_UTILS_NS::Transform& target,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params,
        const SR_MATH_NS::FVector3& rootToTargetDir
    ) {
        /// Вытягиваем руку полностью в направлении target
        SR_MATH_NS::Quaternion rootTargetRotation;
        if (params.useInitialRotations) {
            SR_MATH_NS::FVector3 rootForward = state.rootInitialRotation * state.rootToMidLocal;
            rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(rootForward, rootToTargetDir) * state.rootInitialRotation;
        }
        else {
            SR_MATH_NS::FVector3 rootForward = root.GetGlobalRotation() * state.rootToMidLocal;
            rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(rootForward, rootToTargetDir) * root.GetGlobalRotation();
        }

        SR_MATH_NS::FVector3 midPosition = root.GetGlobalTranslation() + rootTargetRotation * state.rootToMidLocal * state.upperArmLength;
        SR_MATH_NS::FVector3 midToTargetDir = (target.GetGlobalTranslation() - midPosition).Normalized();
        SR_MATH_NS::FVector3 midForward = rootTargetRotation * state.midToTipLocal;
        SR_MATH_NS::Quaternion midTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(midForward, midToTargetDir) * rootTargetRotation;

        root.SetGlobalRotation(rootTargetRotation);
        mid.SetGlobalRotation(midTargetRotation);
    }

    void SolveTwoBoneRetractedTarget(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        const SR_UTILS_NS::Transform& target,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params,
        const SR_MATH_NS::FVector3& rootToTargetDir
    ) {
        /// Складываем руку - направляем оба звена к target
        SR_MATH_NS::Quaternion rootTargetRotation;
        if (params.useInitialRotations) {
            SR_MATH_NS::FVector3 rootForward = state.rootInitialRotation * state.rootToMidLocal;
            rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(rootForward, rootToTargetDir) * state.rootInitialRotation;
        }
        else {
            SR_MATH_NS::FVector3 rootForward = root.GetGlobalRotation() * state.rootToMidLocal;
            rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(rootForward, rootToTargetDir) * root.GetGlobalRotation();
        }

        SR_MATH_NS::FVector3 midPosition = root.GetGlobalTranslation() + rootTargetRotation * state.rootToMidLocal * state.upperArmLength;
        SR_MATH_NS::FVector3 midToTargetDir = (target.GetGlobalTranslation() - midPosition).Normalized();
        SR_MATH_NS::FVector3 midForward = rootTargetRotation * state.midToTipLocal;
        SR_MATH_NS::Quaternion midTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(midForward, midToTargetDir) * rootTargetRotation;

        root.SetGlobalRotation(rootTargetRotation);
        mid.SetGlobalRotation(midTargetRotation);
    }

    SR_MATH_NS::FVector3 CalculateTwoBoneBendNormal(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        const SR_UTILS_NS::Transform& target,
        const SR_UTILS_NS::Transform* pHint,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params,
        const SR_MATH_NS::FVector3& rootToTarget
    ) {
        SR_MATH_NS::FVector3 rootToTargetNormalized = rootToTarget.Normalized();
        SR_MATH_NS::FVector3 bendNormal;

        if (pHint) {
            /// Используем pole vector для определения направления изгиба
            SR_MATH_NS::FVector3 rootToHint = pHint->GetGlobalTranslation() - root.GetGlobalTranslation();

            /// Проецируем hint на плоскость, перпендикулярную rootToTarget
            SR_MATH_NS::FVector3 projectedHint = rootToHint - SR_MATH_NS::FVector3::Project(rootToHint, rootToTargetNormalized);

            if (projectedHint.Magnitude() > 0.0001f) {
                bendNormal = projectedHint.Normalized();
            }
            else {
                /// Если проекция слишком мала, используем перпендикулярное направление
                bendNormal = SR_MATH_NS::GetPerpendicularVector(rootToTargetNormalized);
            }
        }
        else {
            /// Без hint используем перпендикулярное направление
            bendNormal = SR_MATH_NS::GetPerpendicularVector(rootToTargetNormalized);
        }

        /// Предотвращаем перекручивание: если есть предыдущий bend normal, используем его для стабильности
        if (state.hasLastBendNormal && params.preventTwist)
        {
            /// Вычисляем угол между текущим и предыдущим bend normal
            const float_t angle = SR_MATH_NS::FVector3::Angle(bendNormal, state.lastBendNormal);

            /// Если угол слишком большой, ограничиваем изменение
            if (angle > params.maxTwistChangePerFrame)
            {
                SR_MATH_NS::FVector3 rotationAxis = SR_MATH_NS::FVector3::Cross(state.lastBendNormal, bendNormal);
                if (rotationAxis.Magnitude() > 0.0001f)
                {
                    rotationAxis = rotationAxis.Normalized();
                    SR_MATH_NS::Quaternion correction = SR_MATH_NS::Quaternion::AngleAxis(params.maxTwistChangePerFrame, rotationAxis);
                    bendNormal = correction * state.lastBendNormal;
                }
            }
        }

        return bendNormal;
    }

    void SolveTwoBoneReachableTarget(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        const SR_UTILS_NS::Transform& target,
        const SR_UTILS_NS::Transform* pHint,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params,
        const SR_MATH_NS::FVector3& rootToTarget,
        float distanceToTarget
    ) {
        SR_MATH_NS::FVector3 rootToTargetDir = rootToTarget.Normalized();

        /// Вычисляем угол между верхним и нижним звеном используя закон косинусов
        float cosMidAngle = ((state.upperArmLength * state.upperArmLength) + (state.lowerArmLength * state.lowerArmLength) - (distanceToTarget * distanceToTarget)) / (2.f * state.upperArmLength * state.lowerArmLength);
        cosMidAngle = SR_MATH_NS::Clamp(cosMidAngle, -1.f, 1.f);

        /// Вычисляем направление изгиба с учетом pole vector и предыдущего состояния
        SR_MATH_NS::FVector3 bendNormal = CalculateTwoBoneBendNormal(root, mid, target, pHint, state, params, rootToTarget);

        /// Вычисляем позицию mid сустава используя правильную геометрию
        /// Используем закон косинусов для вычисления расстояния от root до mid вдоль rootToTarget
        float cosRootAngle = ((state.upperArmLength * state.upperArmLength) + (distanceToTarget * distanceToTarget) - (state.lowerArmLength * state.lowerArmLength)) / (2.f * state.upperArmLength * distanceToTarget);
        cosRootAngle = SR_MATH_NS::Clamp(cosRootAngle, -1.f, 1.f);
        float rootAngle = SR_ACOS(cosRootAngle);

        /// Вычисляем расстояние от root до mid вдоль rootToTarget
        float rootToMidDistanceAlongTarget = state.upperArmLength * SR_COS(rootAngle);

        /// Вычисляем расстояние от root до mid в плоскости изгиба (перпендикулярно rootToTarget)
        float midPlaneDistance = state.upperArmLength * SR_SIN(rootAngle);

        /// Вычисляем позицию mid
        SR_MATH_NS::FVector3 midPosition = root.GetGlobalTranslation() + rootToTargetDir * rootToMidDistanceAlongTarget + bendNormal * midPlaneDistance;

        /// Вычисляем направление от root к mid
        SR_MATH_NS::FVector3 rootToMidDir = (midPosition - root.GetGlobalTranslation()).Normalized();

        /// Вычисляем вращение root для направления к mid
        SR_MATH_NS::Quaternion rootTargetRotation;
        if (params.useInitialRotations)
        {
            /// Используем исходное вращение root как базу для стабильности
            SR_MATH_NS::FVector3 rootForward = state.rootInitialRotation * state.rootToMidLocal;
            rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(rootForward, rootToMidDir) * state.rootInitialRotation;
        }
        else
        {
            /// Используем текущее вращение root
            SR_MATH_NS::FVector3 rootForward = root.GetGlobalRotation() * state.rootToMidLocal;
            rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(rootForward, rootToMidDir) * root.GetGlobalRotation();
        }

        /// Обновляем позицию mid с учетом нового вращения root (для точности)
        midPosition = root.GetGlobalTranslation() + rootTargetRotation * state.rootToMidLocal * state.upperArmLength;

        /// Вычисляем направление от mid к target
        SR_MATH_NS::FVector3 midToTargetDir = (target.GetGlobalTranslation() - midPosition).Normalized();

        /// Вычисляем вращение mid для направления к target
        SR_MATH_NS::Quaternion midTargetRotation;
        if (params.useInitialRotations)
        {
            /// Используем исходное вращение mid как базу, но учитываем вращение root
            SR_MATH_NS::FVector3 midForward = rootTargetRotation * state.midToTipLocal;
            midTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(midForward, midToTargetDir) * rootTargetRotation;
        }
        else
        {
            /// Используем текущее вращение mid
            SR_MATH_NS::FVector3 midForward = rootTargetRotation * state.midToTipLocal;
            midTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(midForward, midToTargetDir) * rootTargetRotation;
        }

        /// Применяем вычисленные вращения
        root.SetGlobalRotation(rootTargetRotation);
        mid.SetGlobalRotation(midTargetRotation);

        /// Сохраняем bend normal для следующего кадра
        state.lastBendNormal = bendNormal;
        state.hasLastBendNormal = true;
    }

    void ApplyTwoBoneRootAngleLimit(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        const SR_UTILS_NS::Transform& target,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params
    ) {
        /// Вычисляем текущее направление от root к mid
        SR_MATH_NS::FVector3 currentRootToMid = (mid.GetGlobalTranslation() - root.GetGlobalTranslation()).Normalized();

        /// Вычисляем желаемое направление от root к target
        SR_MATH_NS::FVector3 desiredRootToTarget = (target.GetGlobalTranslation() - root.GetGlobalTranslation()).Normalized();

        /// Вычисляем угол между текущим и желаемым направлением
        float angle = SR_MATH_NS::FVector3::Angle(currentRootToMid, desiredRootToTarget);

        if (angle > params.rootAngleLimit) {
            /// Ограничиваем угол - находим направление, которое находится на границе ограничения
            SR_MATH_NS::FVector3 rotationAxis = SR_MATH_NS::FVector3::Cross(currentRootToMid, desiredRootToTarget);
            if (rotationAxis.Magnitude() > 0.0001f)
            {
                rotationAxis = rotationAxis.Normalized();
                SR_MATH_NS::Quaternion limitRotation = SR_MATH_NS::Quaternion::AngleAxis(params.rootAngleLimit, rotationAxis);
                SR_MATH_NS::FVector3 limitedDirection = limitRotation * currentRootToMid;

                /// Применяем ограниченное направление к root
                SR_MATH_NS::Quaternion rootTargetRotation = SR_MATH_NS::Quaternion::FromToRotation(root.TransformDirection(state.rootToMidLocal), limitedDirection) * root.GetGlobalRotation();
                root.SetGlobalRotation(rootTargetRotation);
            }
        }
    }

    void ApplyTwoBoneMidAngleLimit(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const IKTwoBoneParams& params
    ) {
        /// Вычисляем угол между верхним и нижним звеном
        SR_MATH_NS::FVector3 upperArm = (mid.GetGlobalTranslation() - root.GetGlobalTranslation()).Normalized();
        SR_MATH_NS::FVector3 lowerArm = (tip.GetGlobalTranslation() - mid.GetGlobalTranslation()).Normalized();

        /// Угол изгиба - это угол между противоположным направлением верхнего звена и нижним звеном
        float angle = SR_MATH_NS::FVector3::Angle(-upperArm, lowerArm);

        if (angle > params.midAngleLimit && params.midAngleLimit > 0.f) {
            /// Ограничиваем угол изгиба
            SR_MATH_NS::FVector3 rotationAxis = SR_MATH_NS::FVector3::Cross(-upperArm, lowerArm);
            if (rotationAxis.Magnitude() > 0.0001f) {
                rotationAxis = rotationAxis.Normalized();

                /// Вычисляем, на сколько нужно повернуть, чтобы достичь ограничения
                float angleDifference = angle - params.midAngleLimit;
                SR_MATH_NS::Quaternion limitRotation = SR_MATH_NS::Quaternion::AngleAxis(-angleDifference, rotationAxis);

                /// Применяем ограничение к mid, сохраняя связь с root
                SR_MATH_NS::Quaternion rootRotation = root.GetGlobalRotation();
                mid.SetGlobalRotation(limitRotation * mid.GetGlobalRotation());

                /// Убеждаемся, что tip все еще направлен к target (насколько возможно)
                SR_MATH_NS::FVector3 newLowerArm = (tip.GetGlobalTranslation() - mid.GetGlobalTranslation()).Normalized();
                SR_MATH_NS::FVector3 midToTarget = (target.GetGlobalTranslation() - mid.GetGlobalTranslation()).Normalized();

                /// Если после ограничения tip слишком далеко от target, корректируем
                float alignment = SR_MATH_NS::FVector3::Dot(newLowerArm, midToTarget);
                if (alignment < 0.9f) {
                    /// Пытаемся улучшить выравнивание в пределах ограничений
                    SR_MATH_NS::Quaternion correction = SR_MATH_NS::Quaternion::FromToRotation(newLowerArm, midToTarget);
                    float correctionAngle = SR_MATH_NS::Quaternion::Angle(SR_MATH_NS::Quaternion::Identity(), correction);

                    if (correctionAngle < angleDifference) {
                        mid.SetGlobalRotation(correction * mid.GetGlobalRotation());
                    }
                }
            }
        }
    }

    void ClampTwoBoneTwist(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params
    ) {
        /// Вычисляем общее изменение вращения root
        SR_MATH_NS::Quaternion rootRotationDelta = root.GetGlobalRotation() * SR_MATH_NS::Quaternion::Inverse(state.lastRootRotation);
        float rootAngleDelta = SR_MATH_NS::Quaternion::Angle(SR_MATH_NS::Quaternion::Identity(), rootRotationDelta);

        if (rootAngleDelta > params.maxTwistChangePerFrame) {
            /// Ограничиваем изменение вращения через интерполяцию
            float t = params.maxTwistChangePerFrame / rootAngleDelta;
            root.SetGlobalRotation(SR_MATH_NS::Quaternion::Slerp(state.lastRootRotation, root.GetGlobalRotation(), t));
        }

        /// Вычисляем общее изменение вращения mid
        SR_MATH_NS::Quaternion midRotationDelta = mid.GetGlobalRotation() * SR_MATH_NS::Quaternion::Inverse(state.lastMidRotation);
        float midAngleDelta = SR_MATH_NS::Quaternion::Angle(SR_MATH_NS::Quaternion::Identity(), midRotationDelta);

        if (midAngleDelta > params.maxTwistChangePerFrame) {
            /// Ограничиваем изменение вращения через интерполяцию
            float t = params.maxTwistChangePerFrame / midAngleDelta;
            mid.SetGlobalRotation(SR_MATH_NS::Quaternion::Slerp(state.lastMidRotation, mid.GetGlobalRotation(), t));
        }

        /// Дополнительная проверка: предотвращаем перекручивание вокруг оси звена
        /// Используем более надежный метод - отслеживаем изменение плоскости изгиба

        /// Для root: проверяем twist вокруг оси от root к mid
        SR_MATH_NS::FVector3 rootToMid = (mid.GetGlobalTranslation() - root.GetGlobalTranslation()).Normalized();
        if (rootToMid.Magnitude() > 0.0001f) {
            /// Используем локальное направление для стабильности
            SR_MATH_NS::FVector3 lastRootLocalDir = state.lastRootRotation * state.rootToMidLocal;
            SR_MATH_NS::FVector3 currentRootLocalDir = root.GetGlobalRotation() * state.rootToMidLocal;

            /// Проецируем на плоскость, перпендикулярную rootToMid
            SR_MATH_NS::FVector3 lastProjected = lastRootLocalDir - SR_MATH_NS::FVector3::Project(lastRootLocalDir, rootToMid);
            SR_MATH_NS::FVector3 currentProjected = currentRootLocalDir - SR_MATH_NS::FVector3::Project(currentRootLocalDir, rootToMid);

            if (lastProjected.Magnitude() > 0.0001f && currentProjected.Magnitude() > 0.0001f)
            {
                float twistAngle = SR_MATH_NS::FVector3::SignedAngle(lastProjected.Normalized(), currentProjected.Normalized(), rootToMid);

                if (SR_ABS(twistAngle) > params.maxTwistChangePerFrame)
                {
                    /// Корректируем вращение для ограничения twist
                    float correctionAngle = SR_MATH_NS::Sign(twistAngle) * params.maxTwistChangePerFrame;
                    SR_MATH_NS::Quaternion correction = SR_MATH_NS::Quaternion::AngleAxis(correctionAngle - twistAngle, rootToMid);
                    root.SetGlobalRotation(correction * root.GetGlobalRotation());
                }
            }
        }

        /// Для mid: проверяем twist вокруг оси от mid к tip
        SR_MATH_NS::FVector3 midToTip = (tip.GetGlobalTranslation() - mid.GetGlobalTranslation()).Normalized();
        if (midToTip.Magnitude() > 0.0001f) {
            /// Используем локальное направление для стабильности
            SR_MATH_NS::FVector3 lastMidLocalDir = state.lastMidRotation * state.midToTipLocal;
            SR_MATH_NS::FVector3 currentMidLocalDir = mid.GetGlobalRotation() * state.midToTipLocal;

            /// Проецируем на плоскость, перпендикулярную midToTip
            SR_MATH_NS::FVector3 lastProjected = lastMidLocalDir - SR_MATH_NS::FVector3::Project(lastMidLocalDir, midToTip);
            SR_MATH_NS::FVector3 currentProjected = currentMidLocalDir - SR_MATH_NS::FVector3::Project(currentMidLocalDir, midToTip);

            if (lastProjected.Magnitude() > 0.0001f && currentProjected.Magnitude() > 0.0001f) {
                float twistAngle = SR_MATH_NS::FVector3::SignedAngle(lastProjected.Normalized(), currentProjected.Normalized(), midToTip);

                if (SR_ABS(twistAngle) > params.maxTwistChangePerFrame) {
                    /// Корректируем вращение для ограничения twist
                    float correctionAngle = SR_MATH_NS::Sign(twistAngle) * params.maxTwistChangePerFrame;
                    SR_MATH_NS::Quaternion correction = SR_MATH_NS::Quaternion::AngleAxis(correctionAngle - twistAngle, midToTip);
                    mid.SetGlobalRotation(correction * mid.GetGlobalRotation());
                }
            }
        }
    }

    void SolveTwoBone(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const SR_UTILS_NS::Transform* pHint,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params
    ) {
        SR_TRACY_ZONE;

        if (params.weight <= 0.0f) {
            return;
        }

        InitializeTwoBoneIKState(root, mid, tip, target, pHint, state);

        /// Сохраняем исходные вращения для интерполяции
        const SR_MATH_NS::Quaternion rootRotationOriginal = root.GetGlobalRotation();
        const SR_MATH_NS::Quaternion midRotationOriginal = mid.GetGlobalRotation();

        /// Вычисляем целевые вращения
        const SR_MATH_NS::FVector3 targetPosition = target.GetGlobalTranslation();

        /// Вычисляем направление от root к target
        const SR_MATH_NS::FVector3 rootToTarget = targetPosition - root.GetGlobalTranslation();
        const float_t distanceToTarget = rootToTarget.Magnitude();

        /// Проверяем достижимость target
        if (distanceToTarget > state.armLength) {
            /// Target слишком далеко - вытягиваем руку полностью
            SolveTwoBoneExtendedTarget(root, mid, target, state, params, rootToTarget.Normalized());
        }
        else if (distanceToTarget < SR_MATH_NS::Abs(state.upperArmLength - state.lowerArmLength)) {
            /// Target слишком близко - складываем руку
            SolveTwoBoneRetractedTarget(root, mid, target, state, params, rootToTarget.Normalized());
        }
        else {
            /// Нормальный случай - решаем треугольник
            SolveTwoBoneReachableTarget(root, mid, target, pHint, state, params, rootToTarget, distanceToTarget);
        }

        /// Применяем ограничения углов
        if (params.rootAngleLimit > 0.f) {
            ApplyTwoBoneRootAngleLimit(root, mid, target, state, params);
        }

        if (params.midAngleLimit > 0.f) {
            ApplyTwoBoneMidAngleLimit(root, mid, tip, target, params);
        }

        /// Предотвращаем перекручивание (до применения веса)
        if (params.preventTwist) {
            ClampTwoBoneTwist(root, mid, tip, state, params);
        }

        /// Применяем вес и сглаживание
        if (params.weight < 1.f) {
            /// Применяем вес
            root.SetGlobalRotation(SR_MATH_NS::Quaternion::Slerp(rootRotationOriginal, root.GetGlobalRotation(), params.weight));
            mid.SetGlobalRotation(SR_MATH_NS::Quaternion::Slerp(midRotationOriginal, mid.GetGlobalRotation(), params.weight));
        }

        /// Применяем сглаживание отдельно для предотвращения рывков
        if (params.smoothing > 0.f) {
            const float_t smoothFactor = SR_MATH_NS::Clamp(params.dt * params.smoothing, 0.f, 1.f);

            root.SetGlobalRotation(SR_MATH_NS::Quaternion::Slerp(state.lastRootRotation, root.GetGlobalRotation(), smoothFactor));
            mid.SetGlobalRotation(SR_MATH_NS::Quaternion::Slerp(state.lastMidRotation, mid.GetGlobalRotation(), smoothFactor));
        }

        /// Сохраняем текущие вращения для следующего кадра
        state.lastRootRotation = root.GetGlobalRotation();
        state.lastMidRotation = mid.GetGlobalRotation();

        if (params.tipRotationFromTarget) {
            tip.SetGlobalRotation(target.GetGlobalRotation());
        }
    }
}
