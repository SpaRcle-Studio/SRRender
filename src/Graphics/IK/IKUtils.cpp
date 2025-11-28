//
// Created by Monika on 23.11.2025.
//

#include <Graphics/IK/IKUtils.h>

#include <Utils/ECS/Transform.h>

namespace SR_GRAPH_NS::IK {
    void SolveTwoBoneIK_GLM(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    ) {
        SR_TRACY_ZONE;

       /* glm::vec3 rootPos = root.GetMatrix().Orthonormalize().GetTranslate().ToGLM();
        glm::vec3 middlePos = mid.GetMatrix().Orthonormalize().GetTranslate().ToGLM();
        glm::vec3 effectorPos = tip.GetMatrix().Orthonormalize().GetTranslate().ToGLM();
        glm::vec3 targetPos = target.GetMatrix().Orthonormalize().GetTranslate().ToGLM();

        glm::quat rootGlobalRotation = root.GetMatrix().Orthonormalize().GetQuat().ToGLM();
        glm::quat middleGlobalRotation = mid.GetMatrix().Orthonormalize().GetQuat().ToGLM();
        glm::quat effectorGlobalRotation = tip.GetMatrix().Orthonormalize().GetQuat().ToGLM();

        glm::quat rootLocalRotation = root.GetQuaternion().ToGLM();
        glm::quat middleLocalRotation = mid.GetQuaternion().ToGLM();

        glm::vec3 ab = middlePos - rootPos;
        glm::vec3 ac = effectorPos - rootPos;
        glm::vec3 at = targetPos - rootPos;
        glm::vec3 cb = middlePos - effectorPos;

        // Step1: 旋转关节root和middle, 让dist(root, effector) == dist(root, target)
        float len_ab = glm::length(ab);
        float len_cb = glm::length(cb);
        // 计算可达性
        float len_at = SR_CLAMP(glm::length(at), SR_KINDA_SMALL_NUMBER_EPSILON, len_ab + len_cb + SR_KINDA_SMALL_NUMBER_EPSILON);
        // 计算Step1中, 关节root和middle的旋转角
        float angle_ac_ab_0 = std::acos(std::clamp(glm::dot(glm::normalize(ac), glm::normalize(ab)),
                                                   -1.f, 1.f));
        float angle_ba_bc_0 = std::acos(std::clamp(glm::dot(glm::normalize(-ab), glm::normalize(-cb)),
                                                   -1.f, 1.f));
        float angle_ac_ab_1 = std::acos(std::clamp(static_cast<float>(
                                                           (len_cb * len_cb - len_ab * len_ab - len_at * len_at) / (-2.0 * len_ab * len_at)),
                                                   -1.f, 1.f));
        float angle_ba_bc_1 = std::acos(std::clamp(static_cast<float>(
                                                           (len_at * len_at - len_ab * len_ab - len_cb * len_cb) / (-2.0 * len_ab * len_cb)),
                                                   -1.f, 1.f));
        float angle_ac_ab = angle_ac_ab_1 - angle_ac_ab_0;
        float angle_ba_bc = angle_ba_bc_1 - angle_ba_bc_0;
        // 计算Step1中, 关节root和middle的旋转轴
        //glm::vec3 d = glm::rotate(middleGlobalRotation, glm::vec3(0, 0, 1));
        glm::vec3 d = SR_MATH_NS::FVector3::UnitZ().Rotate(SR_MATH_NS::Quaternion(middleGlobalRotation)).ToGLM();
        glm::vec3 axis0 = glm::normalize(glm::cross(ac, d));
        // 旋转关节root和middle, 注意旋转轴"世界空间->模型本地空间"的转换
        rootLocalRotation = glm::rotate(rootLocalRotation, angle_ac_ab, axis0 * glm::inverse(rootGlobalRotation));
        middleLocalRotation = glm::rotate(middleLocalRotation, angle_ba_bc, axis0 * glm::inverse(rootGlobalRotation));

        // Step2: 旋转root关节, 让Effector到达Target
        float angle_ac_at = std::acos(std::clamp(glm::dot(glm::normalize(ac), glm::normalize(at)),
                                                 -1.f, 1.f));
        glm::vec3 axis1 = glm::normalize(glm::cross(ac, at));
        rootLocalRotation = glm::rotate(rootLocalRotation, angle_ac_at, axis1 * glm::inverse(rootGlobalRotation));


        mid.SetRotation(SR_MATH_NS::Quaternion(middleLocalRotation));
        root.SetRotation(SR_MATH_NS::Quaternion(rootLocalRotation));*/
    }

    // Вспомогательная функция: проекция вектора на нормализованный вектор (аналог ProjectOnToNormal из UE)
    static SR_MATH_NS::FVector3 ProjectOnToNormal(const SR_MATH_NS::FVector3& vec, const SR_MATH_NS::FVector3& normal) {
        return normal * vec.Dot(normal);
    }

    void SolveTwoBoneIK_Twist(
            SR_UTILS_NS::Transform& root,
            SR_UTILS_NS::Transform& mid,
            SR_UTILS_NS::Transform& tip,
            const SR_UTILS_NS::Transform& target,
            const std::optional<SR_MATH_NS::FVector3>& hintPosition,
            IKState& ikState,
            float_t targetPosWeight,
            float_t targetRotWeight,
            float_t hintWeight
    ) {
        SR_TRACY_ZONE;

        if (!ikState.rootBaseRotation) {
            ikState.rootBaseRotation = root.GetGlobalRotation();
            ikState.rootCurrentRotation = *ikState.rootBaseRotation;

            ikState.midBaseRotation = mid.GetGlobalRotation();
            ikState.midCurrentRotation = *ikState.midBaseRotation;
        }

        const float smoothing = 0.5f; // Сглаживание поворота (0 = нет сглаживания, 1 = максимальное)
        const float minAngle = 0.0f; // Минимальный угол для применения поворота (градусы)

        SR_MATH_NS::FVector3 aPosition = root.GetMatrix().GetTranslate();
        //SR_MATH_NS::FVector3 bPosition = mid.GetMatrix().GetTranslate();
        SR_MATH_NS::FVector3 cPosition = tip.GetMatrix().GetTranslate();

        const SR_MATH_NS::FVector3 targetPos = target.GetMatrix().GetTranslate();
        //const SR_MATH_NS::Quaternion targetRot = target.GetGlobalRotation();

        const SR_MATH_NS::FVector3 tPosition = cPosition.Lerp(targetPos, targetPosWeight);
        //const SR_MATH_NS::Quaternion tRotation = tip.GetGlobalRotation().Slerp(targetRot, targetRotWeight);

        //SR_MATH_NS::FVector3 ab = bPosition - aPosition;
        //SR_MATH_NS::FVector3 bc = cPosition - bPosition;
        SR_MATH_NS::FVector3 ac = cPosition - aPosition;
        SR_MATH_NS::FVector3 at = tPosition - aPosition;

        /*const float abLen = ab.Length();
        const float bcLen = bc.Length();
        const float acLen = ac.Length();
        const float atLen = at.Length();

        float oldAbcAngle = SR_MATH_NS::TriangleAngle(acLen, abLen, bcLen);
        float newAbcAngle = SR_MATH_NS::TriangleAngle(atLen, abLen, bcLen);

        SR_MATH_NS::FVector3 axis = ab.Cross(bc);

        if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
        {
            axis = SR_MATH_NS::FVector3();

            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = at.Cross(bc);

            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = SR_MATH_NS::FVector3::Up();
        }
        axis = axis.Normalize();

        float a = 0.5f * (oldAbcAngle - newAbcAngle);
        float sin = SR_SIN(a);
        float cos = SR_COS(a);

        SR_MATH_NS::Quaternion targetMidRotation = SR_MATH_NS::Quaternion(sin * axis.x, sin * axis.y, sin * axis.z, cos) * *ikState.midBaseRotation;
        SR_MATH_NS::Quaternion rootDelta = *ikState.rootBaseRotation * ikState.rootCurrentRotation.Conjugate();

        targetMidRotation = targetMidRotation * rootDelta;

        if (SR_MATH_NS::Quaternion::Angle(ikState.rootCurrentRotation, targetMidRotation) >= minAngle) {
            if (smoothing > 0.01f) {
                ikState.midCurrentRotation = SR_MATH_NS::Quaternion::Slerp(ikState.midCurrentRotation, targetMidRotation, 1.f - smoothing);
            }
            else {
                ikState.midCurrentRotation = targetMidRotation;
            }
            mid.SetGlobalRotation(ikState.midCurrentRotation);
        }

        cPosition = tip.GetMatrix().GetTranslate();
        ac = cPosition - aPosition;*/

        //////

        //if (ac.SqrMagnitude() < SR_SMALL_NUMBER_EPSILON || at.SqrMagnitude() < SR_SMALL_NUMBER_EPSILON) {
        //    return; // Векторы слишком малы, не поворачиваем
        //}

        // Вычисляем поворот от ac к at
        SR_MATH_NS::Quaternion rootDelta = SR_MATH_NS::Quaternion::FromToRotation(ac, at);
        SR_MATH_NS::Quaternion targetRootRotation = rootDelta * *ikState.rootBaseRotation;

        if (SR_MATH_NS::Quaternion::Angle(ikState.rootCurrentRotation, targetRootRotation) >= minAngle) {
            if (smoothing > 0.01f) {
                ikState.rootCurrentRotation = SR_MATH_NS::Quaternion::Slerp(ikState.rootCurrentRotation, targetRootRotation, 1.f - smoothing);
            }
            else {
                ikState.rootCurrentRotation = targetRootRotation;
            }

            root.SetGlobalRotation(rootDelta);
        }


        //ikState.rootBaseRotation = ikState.rootBaseRotation->Slerp(ikState.rootCurrentRotation, 0.1f);
    }

    void SolveTwoBoneIK(
            SR_UTILS_NS::Transform& root,
            SR_UTILS_NS::Transform& mid,
            SR_UTILS_NS::Transform& tip,
            const SR_UTILS_NS::Transform& target,
            const std::optional<SR_MATH_NS::FVector3>& hintPosition,
            IKState& ikState,
            float_t targetPosWeight,
            float_t targetRotWeight,
            float_t hintWeight
    ) {
        SR_TRACY_ZONE;

        // Реализация основана на Unity Animation Rigging Package
        // https://github.com/Unity-Technologies/Animation-Rigging

        SR_MATH_NS::FVector3 aPosition = root.GetMatrix().GetTranslate();
        SR_MATH_NS::FVector3 bPosition = mid.GetMatrix().GetTranslate();
        SR_MATH_NS::FVector3 cPosition = tip.GetMatrix().GetTranslate();

        const SR_MATH_NS::FVector3 targetPos = target.GetMatrix().GetTranslate();
        const SR_MATH_NS::Quaternion targetRot = target.GetMatrix().GetQuat();

        const SR_MATH_NS::FVector3 tPosition = cPosition.Lerp(targetPos, targetPosWeight);
        const SR_MATH_NS::Quaternion tRotation = tip.GetMatrix().GetQuat().Slerp(targetRot, targetRotWeight);

        const bool hasHint = hintPosition.has_value() && hintWeight > 0.0f && true;

        SR_MATH_NS::FVector3 ab = bPosition - aPosition;
        SR_MATH_NS::FVector3 bc = cPosition - bPosition;
        SR_MATH_NS::FVector3 ac = cPosition - aPosition;
        SR_MATH_NS::FVector3 at = tPosition - aPosition;

        const float abLen = ab.Length();
        const float bcLen = bc.Length();
        const float acLen = ac.Length();
        const float atLen = at.Length();

        float oldAbcAngle = SR_MATH_NS::TriangleAngle(acLen, abLen, bcLen);
        float newAbcAngle = SR_MATH_NS::TriangleAngle(atLen, abLen, bcLen);

        SR_MATH_NS::FVector3 axis = ab.Cross(bc);

        if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
        {
            axis = hasHint ? (*hintPosition - aPosition).Cross(bc) : SR_MATH_NS::FVector3();

            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = at.Cross(bc);

            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = SR_MATH_NS::FVector3::Up();
        }
        axis = axis.Normalize();

        float a = 0.5f * (oldAbcAngle - newAbcAngle);
        float sin = SR_SIN(a);
        float cos = SR_COS(a);
        //SR_MATH_NS::Quaternion deltaR(sin * axis.x, sin * axis.y, sin * axis.z, cos);
        //mid.SetGlobalRotation(deltaR * mid.GetMatrix().GetQuat());

        cPosition = tip.GetMatrix().GetTranslate();
        ac = cPosition - aPosition;

        root.SetGlobalRotation(SR_MATH_NS::Quaternion::FromToRotation(ac, at) * root.GetMatrix().GetQuat());

        if (hasHint) {
            float acSqrMag = ac.SqrMagnitude();
            if (acSqrMag > 0.0f) {
                bPosition = mid.GetMatrix().GetTranslate();
                cPosition = tip.GetMatrix().GetTranslate();
                ab = bPosition - aPosition;
                ac = cPosition - aPosition;

                SR_MATH_NS::FVector3 acNorm = ac / SR_SQRT(acSqrMag);
                SR_MATH_NS::FVector3 ah = *hintPosition - aPosition;
                SR_MATH_NS::FVector3 abProj = ab - acNorm * ab.Dot(acNorm);
                SR_MATH_NS::FVector3 ahProj = ah - acNorm * ah.Dot(acNorm);

                float maxReach = abLen + bcLen;
                if (abProj.SqrMagnitude() > (maxReach * maxReach * 0.001f) && ahProj.SqrMagnitude() > 0.0f) {
                    SR_MATH_NS::Quaternion hintR = SR_MATH_NS::Quaternion::FromToRotation(abProj, ahProj); //, ikState.previousBendAxis

                    hintR.x *= hintWeight;
                    hintR.y *= hintWeight;
                    hintR.z *= hintWeight;
                    hintR = hintR.NormalizeSafe();

                    root.SetGlobalRotation(hintR * root.GetMatrix().GetQuat());
                }
            }
        }

        tip.SetGlobalRotation(tRotation);
    }

    void SolveTwoBoneIK_WithFixes(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    ) {
        SR_TRACY_ZONE;

        // Реализация основана на Unity Animation Rigging Package
        // https://github.com/Unity-Technologies/Animation-Rigging

        SR_MATH_NS::FVector3 aPosition = root.GetMatrix().Orthonormalize().GetTranslate();
        SR_MATH_NS::FVector3 bPosition = mid.GetMatrix().Orthonormalize().GetTranslate();
        SR_MATH_NS::FVector3 cPosition = tip.GetMatrix().Orthonormalize().GetTranslate();

        const SR_MATH_NS::FVector3 targetPos = target.GetMatrix().Orthonormalize().GetTranslate();
        const SR_MATH_NS::Quaternion targetRot = target.GetMatrix().Orthonormalize().GetQuat();

        const SR_MATH_NS::FVector3 tPosition = cPosition.Lerp(targetPos, targetPosWeight);
        const SR_MATH_NS::Quaternion tRotation = tip.GetMatrix().Orthonormalize().GetQuat().Slerp(targetRot, targetRotWeight);

        const bool hasHint = hintPosition.has_value() && hintWeight > 0.0f && false;

        SR_MATH_NS::FVector3 ab = bPosition - aPosition;
        SR_MATH_NS::FVector3 bc = cPosition - bPosition;
        SR_MATH_NS::FVector3 ac = cPosition - aPosition;
        SR_MATH_NS::FVector3 at = tPosition - aPosition;

        const float abLen = ab.Length();
        const float bcLen = bc.Length();
        const float acLen = ac.Length();
        const float atLen = at.Length();

        float oldAbcAngle = SR_MATH_NS::TriangleAngle(acLen, abLen, bcLen);
        float newAbcAngle = SR_MATH_NS::TriangleAngle(atLen, abLen, bcLen);

        SR_MATH_NS::FVector3 axis = ab.Cross(bc);

        if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON) {
            // Дегенерация → пробуем hint
            if (hasHint)
                axis = (*hintPosition - aPosition).Cross(bc);

            // всё ещё ноль → пробуем вектор цели
            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = at.Cross(bc);

            // если вообще жопа → fallback на previous ось
            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = ikState.previousBendAxis;

            // если даже предыдущая ось пустая — просто Up
            if (axis.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                axis = SR_MATH_NS::FVector3::Up();
        }

        // НОРМАЛЬНЫЙ СЛУЧАЙ → ось обновляем
        if (axis.SqrMagnitude() >= SR_KINDA_SMALL_NUMBER_EPSILON) {
            axis = axis.Normalize();
            ikState.previousBendAxis = axis;
        }
        else {
            // НЕ НОРМА → ось оставляем прежней
            axis = ikState.previousBendAxis;
        }

        float a = 0.5f * (oldAbcAngle - newAbcAngle);
        float sin = SR_SIN(a);
        float cos = SR_COS(a);
        //SR_MATH_NS::Quaternion deltaR(sin * axis.x, sin * axis.y, sin * axis.z, cos);
        //mid.SetGlobalRotation(deltaR * mid.GetMatrix().Orthonormalize().GetQuat());

        cPosition = tip.GetMatrix().Orthonormalize().GetTranslate();
        ac = cPosition - aPosition;

        //SR_MATH_NS::Quaternion rootR = SR_MATH_NS::Quaternion::FromToRotation(ac, at, ikState.previousBendAxis);
        //rootR = SR_MATH_NS::Quaternion::Slerp(SR_MATH_NS::Quaternion::Identity(), rootR, hintWeight).NormalizeSafe();
        //root.SetGlobalRotation(rootR * root.GetMatrix().Orthonormalize().GetQuat());


        //SR_MATH_NS::FVector3 bendPlaneNormal = (*hintPosition - aPosition).Cross(ac).NormalizeSafe();
        //auto&& acProjected = ac - ProjectOnToNormal(ac, bendPlaneNormal);
        //auto&& projectedAt = at - ProjectOnToNormal(at, bendPlaneNormal);
        //SR_MATH_NS::Quaternion rootR = SR_MATH_NS::Quaternion::FromToRotation(acProjected, projectedAt, ikState.previousBendAxis);

        //SR_MATH_NS::FVector3 desiredBendDir = at.Cross(ac); // нормаль плоскости сгиба
        //if (desiredBendDir.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
        //    desiredBendDir = ikState.previousBendAxis; // fallback
        //SR_MATH_NS::FVector3 bendPlaneNormal = desiredBendDir.NormalizeSafe();

        /*SR_MATH_NS::FVector3 acNorm = ac.NormalizeSafe();
        SR_MATH_NS::FVector3 atNorm = at.NormalizeSafe();

// 1. Swing: поворот, который выравнивает ac с at
        SR_MATH_NS::Quaternion swing;
        float cosTheta = acNorm.Dot(atNorm);
        if (cosTheta > 1.0f - 1e-6f) {
            // почти совпадают
            swing = SR_MATH_NS::Quaternion::Identity();
        } else if (cosTheta < -1.0f + 1e-6f) {
            // противоположные направления — выбираем любую перпендикулярную ось
            SR_MATH_NS::FVector3 ortho = acNorm.Cross(SR_MATH_NS::FVector3::Up());
            if (ortho.SqrMagnitude() < 1e-6f)
                ortho = acNorm.Cross(SR_MATH_NS::FVector3(1,0,0));
            swing = SR_MATH_NS::Quaternion::AngleAxis(SR_DEG(SR_PI), ortho.NormalizeSafe());
        } else {
            SR_MATH_NS::FVector3 rotAxis = acNorm.Cross(atNorm).NormalizeSafe();
            float angle = acosf(cosTheta);
            swing = SR_MATH_NS::Quaternion::AngleAxis(SR_DEG(angle), rotAxis);
        }

        // 2. Twist: вращение вокруг оси ac
        auto ExtractTwist = [](const SR_MATH_NS::Quaternion& q, const SR_MATH_NS::FVector3& axis) -> SR_MATH_NS::Quaternion {
            SR_MATH_NS::FVector3 qAxis;
            float qAngle;
            q.ToAxisAngle(qAxis, qAngle);

            float proj = qAxis.Dot(axis);
            if (fabsf(proj) < 1e-6f) return SR_MATH_NS::Quaternion::Identity();

            // twist должен быть минимальным по модулю
            float twistAngle = qAngle * proj;
            if (twistAngle > SR_PI)
                twistAngle -= 2.0f * SR_PI;
            else if (twistAngle < -SR_PI)
                twistAngle += 2.0f * SR_PI;

            return SR_MATH_NS::Quaternion::AngleAxis(twistAngle, axis);
        };

        SR_MATH_NS::Quaternion twist = ExtractTwist(root.GetMatrix().Orthonormalize().GetQuat(), acNorm);

        // 3. Применяем swing и twist
        //root.SetGlobalRotation(swing * twist);
        root.SetGlobalRotation(SR_MATH_NS::Quaternion::FromToRotation(ac, at, ikState.previousBendAxis) * root.GetMatrix().Orthonormalize().GetQuat());*/


       /* // ПОПЫТКА КОРРЕКЦИИ
         SR_MATH_NS::FVector3 bendPlaneNormal = axis;
        if (bendPlaneNormal.SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON) {
            // fallback: выбираем любую перпендикулярную ось
            bendPlaneNormal = SR_MATH_NS::FVector3::Up();
            if ((bendPlaneNormal.Cross(ac)).SqrMagnitude() < SR_KINDA_SMALL_NUMBER_EPSILON)
                bendPlaneNormal = SR_MATH_NS::FVector3(1,0,0);
        }
        bendPlaneNormal = bendPlaneNormal.NormalizeSafe();

        // проекция на плоскость сгиба
        SR_MATH_NS::FVector3 acProjected = ac - bendPlaneNormal * ac.Dot(bendPlaneNormal);
        SR_MATH_NS::FVector3 atProjected = at - bendPlaneNormal * at.Dot(bendPlaneNormal);

        // нормализация перед FromToRotation
        acProjected = acProjected.NormalizeSafe();
        atProjected = atProjected.NormalizeSafe();

        SR_MATH_NS::Quaternion rootR;
        float cosTheta = ac.Dot(at) / (ac.Length() * at.Length());
        if (cosTheta > 1.0f - 1e-4f) {
            // почти коллинеарно, не вращаем
            rootR = SR_MATH_NS::Quaternion::Identity();
        } else {
            rootR = SR_MATH_NS::Quaternion::FromToRotation(acProjected, atProjected, bendPlaneNormal);
        }

        root.SetGlobalRotation(rootR * root.GetMatrix().Orthonormalize().GetQuat());*/

        root.SetGlobalRotation(SR_MATH_NS::Quaternion::FromToRotation(ac, at) * root.GetMatrix().Orthonormalize().GetQuat());

        if (hasHint && false) {
            float acSqrMag = ac.SqrMagnitude();
            if (acSqrMag > 0.0f) {
                bPosition = mid.GetMatrix().Orthonormalize().GetTranslate();
                cPosition = tip.GetMatrix().Orthonormalize().GetTranslate();
                ab = bPosition - aPosition;
                ac = cPosition - aPosition;

                SR_MATH_NS::FVector3 acNorm = ac / SR_SQRT(acSqrMag);

                SR_MATH_NS::FVector3 ah = *hintPosition - aPosition;
                SR_MATH_NS::FVector3 abProj = ab - acNorm * ab.Dot(acNorm);
                SR_MATH_NS::FVector3 ahProj = ah - acNorm * ah.Dot(acNorm);

                float maxReach = abLen + bcLen;
                if (abProj.SqrMagnitude() > (maxReach * maxReach * 0.001f) && ahProj.SqrMagnitude() > 0.0f) {
                    //SR_MATH_NS::Quaternion hintR = SR_MATH_NS::Quaternion::FromToRotation(abProj, ahProj, ikState.previousBendAxis);

                    //hintR.x *= hintWeight;
                    //hintR.y *= hintWeight;
                    //hintR.z *= hintWeight;
                    //hintR = hintR.NormalizeSafe();

                    //root.SetGlobalRotation(hintR * root.GetMatrix().Orthonormalize().GetQuat());

                    //if (SR_MATH_NS::Quaternion::IsFromToRotationValid(abProj, ahProj)) {
                       // SR_MATH_NS::Quaternion hintR = SR_MATH_NS::Quaternion::FromToRotation(abProj, ahProj, ikState.previousBendAxis);
                       // hintR = SR_MATH_NS::Quaternion::Slerp(SR_MATH_NS::Quaternion::Identity(), hintR, hintWeight).NormalizeSafe();
                       // root.SetGlobalRotation(hintR * root.GetMatrix().Orthonormalize().GetQuat());
                    //}
                }
            }
        }

        tip.SetGlobalRotation(tRotation);
    }

    // Вспомогательная функция: безопасная нормализация с fallback (аналог GetSafeNormal из UE)
    static SR_MATH_NS::FVector3 GetSafeNormal(const SR_MATH_NS::FVector3& vec, float_t tolerance = 1e-6f) {
        float_t sqrMag = vec.SqrMagnitude();
        if (sqrMag > tolerance * tolerance) {
            return vec / SR_SQRT(sqrMag);
        }
        return SR_MATH_NS::FVector3::Zero();
    }

    // Вспомогательная функция: поиск нормали плоскости (аналог FindPlaneNormal из UE)
    static SR_MATH_NS::FVector3 FindPlaneNormal(const std::array<LimbLink, 3>& links, const SR_MATH_NS::FVector3& rootLocation, const SR_MATH_NS::FVector3& targetLocation) {
        const SR_MATH_NS::FVector3 axisX = GetSafeNormal(targetLocation - rootLocation);
        if (axisX.SqrMagnitude() < 1e-6f) {
            return SR_MATH_NS::FVector3::Up();
        }

        for (size_t i = 1; i < links.size(); ++i) {
            const SR_MATH_NS::FVector3 axisY = GetSafeNormal(links[i].Location - rootLocation);
            if (axisY.SqrMagnitude() < 1e-6f) {
                continue;
            }

            const SR_MATH_NS::FVector3 planeNormal = axisX.Cross(axisY);
            const float_t sqrMag = planeNormal.SqrMagnitude();

            // Убеждаемся, что у нас валидная нормаль (оси не коллинеарны)
            if (sqrMag > 1e-6f) {
                return planeNormal / SR_SQRT(sqrMag);
            }
        }

        // Все связи коллинеарны?
        return SR_MATH_NS::FVector3::Up();
    }

    void SolveTwoBoneIK_UE(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_MATH_NS::FVector3& targetLocation,
        const SR_MATH_NS::FVector3& hingeRotationAxis
    ) {
        SR_TRACY_ZONE;

        // Реализация основана на Unreal Engine
        // https://github.com/EpicGames/UnrealEngine

        const float_t k_SmallNumber = 1e-6f;
        const float_t k_KindaSmallNumber = 1e-4f;

        // Инициализируем массив связей (аналог TArray<FLimbLink>)
        std::array<LimbLink, 3> links;

        // Получаем текущие позиции костей и сохраняем старые для вычисления вращений
        const SR_MATH_NS::FVector3 oldRootPos = root.GetMatrix().GetTranslate(); // Hip / Root
        const SR_MATH_NS::FVector3 oldMidPos = mid.GetMatrix().GetTranslate();  // Knee
        const SR_MATH_NS::FVector3 oldTipPos = tip.GetMatrix().GetTranslate();  // Foot

        links[0].Location = oldRootPos; // Hip / Root
        links[1].Location = oldMidPos;  // Knee
        links[2].Location = oldTipPos;  // Foot

        // Вычисляем длины костей
        links[0].Length = (links[1].Location - links[0].Location).Length(); // hip to knee
        links[1].Length = (links[2].Location - links[1].Location).Length();  // knee to foot
        links[2].Length = links[1].Length; // knee to foot (для совместимости с UE)

        // Инициализируем кэшированные направления изгиба
        links[1].RealBendDir = SR_MATH_NS::FVector3::Zero();
        links[1].BaseBendDir = SR_MATH_NS::FVector3::Zero();

        // Сохраняем ссылки для удобства (как в UE)
        SR_MATH_NS::FVector3& pA = links[2].Location; // Foot
        SR_MATH_NS::FVector3& pB = links[1].Location; // Knee
        SR_MATH_NS::FVector3& pC = links[0].Location; // Hip / Root

        // Перемещаем стопу напрямую к цели
        pA = targetLocation;

        const SR_MATH_NS::FVector3 HipToFoot = pA - pC;

        // Используем закон косинусов для решения
        const float_t a = links[0].Length;  // hip to knee
        const float_t b = HipToFoot.Length();  // hip to foot
        const float_t c = links[1].Length;  // knee to foot

        const float_t Two_ab = 2.0f * a * b;
        const float_t CosC = (Two_ab > k_SmallNumber) ? ((a * a + b * b - c * c) / Two_ab) : 0.0f;
        const float_t C = SR_ACOS(SR_CLAMP(CosC, -1.0f, 1.0f));

        // Проецируем колено на линию от бедра до стопы
        const SR_MATH_NS::FVector3 HipToFootDir = (b > k_SmallNumber) ? (HipToFoot / b) : SR_MATH_NS::FVector3::Zero();
        const SR_MATH_NS::FVector3 HipToKnee = pB - pC;
        const SR_MATH_NS::FVector3 ProjKnee = pC + ProjectOnToNormal(HipToKnee, HipToFootDir);

        const SR_MATH_NS::FVector3 ProjKneeToKnee = (pB - ProjKnee);
        SR_MATH_NS::FVector3 BendDir = GetSafeNormal(ProjKneeToKnee, k_KindaSmallNumber);

        // Если у нас определена ось вращения шарнира, можем кэшировать 'BendDir'
        // и использовать его когда не можем определить. (Когда конечность прямая без изгиба).
        if ((hingeRotationAxis.SqrMagnitude() > k_SmallNumber) && 
            (HipToFootDir.SqrMagnitude() > k_SmallNumber) && 
            (a > k_SmallNumber)) {
            
            const SR_MATH_NS::FVector3 HipToKneeDir = HipToKnee / a;
            const float_t KneeBendDot = HipToKneeDir.Dot(HipToFootDir);

            SR_MATH_NS::FVector3& CachedRealBendDir = links[1].RealBendDir;
            SR_MATH_NS::FVector3& CachedBaseBendDir = links[1].BaseBendDir;

            // Валидный 'изгиб', кэшируем 'BendDir'
            if ((BendDir.SqrMagnitude() > k_SmallNumber) && (KneeBendDot < 0.99f)) {
                CachedRealBendDir = BendDir;
                CachedBaseBendDir = hingeRotationAxis.Cross(HipToFootDir);
                CachedBaseBendDir = GetSafeNormal(CachedBaseBendDir, k_SmallNumber);
            }
            // Конечность слишком прямая, не можем точно определить BendDir, используем кэшированное значение если возможно
            else {
                // Если у нас есть кэшированный 'BendDir', переориентируем его на основе 'HingeRotationAxis'
                if (CachedRealBendDir.SqrMagnitude() > k_SmallNumber) {
                    const SR_MATH_NS::FVector3 CurrentBaseBendDir = GetSafeNormal(hingeRotationAxis.Cross(HipToFootDir), k_SmallNumber);
                    if (CachedBaseBendDir.SqrMagnitude() > k_SmallNumber && CurrentBaseBendDir.SqrMagnitude() > k_SmallNumber) {
                        const SR_MATH_NS::Quaternion DeltaCachedToCurrBendDir = SR_MATH_NS::Quaternion::FromToRotation(CachedBaseBendDir, CurrentBaseBendDir);
                        BendDir = DeltaCachedToCurrBendDir * CachedRealBendDir;
                        BendDir = GetSafeNormal(BendDir, k_SmallNumber);
                    }
                }
            }
        }

        // Объединяем обе линии в одну для экономии умножения
        // const FVector NewProjectedKneeLoc = pC + HipToFootDir * a * CosC;
        // const FVector NewKneeLoc = NewProjectedKneeLoc + Dir_LegLineToKnee * a * FMath::Sin(C);
        const SR_MATH_NS::FVector3 NewKneeLoc = pC + (HipToFootDir * CosC + BendDir * SR_SIN(C)) * a;
        pB = NewKneeLoc;

        // Применяем новые позиции к Transform объектам
        // В UE код работает с позициями напрямую, но нам нужно применить это к Transform
        // Root остается на месте (pC не меняется)
        // Mid получает новую позицию (pB = NewKneeLoc)
        // Tip получает целевую позицию (pA = targetLocation)

        // Устанавливаем позиции напрямую (как в UE)
        // Root остается на месте (pC == oldRootPos)
        mid.SetGlobalTranslation(pB); // Устанавливаем новую позицию колена
        tip.SetGlobalTranslation(pA); // Устанавливаем целевую позицию стопы

        // Вычисляем вращения на основе новых позиций для правильной ориентации
        // Root вращается так, чтобы направление от root к mid было правильным
        if (a > k_SmallNumber) {
            const SR_MATH_NS::FVector3 oldRootToMid = (oldMidPos - oldRootPos).NormalizeSafe();
            const SR_MATH_NS::FVector3 newRootToMid = (pB - pC).NormalizeSafe();
            
            if (oldRootToMid.SqrMagnitude() > k_SmallNumber && newRootToMid.SqrMagnitude() > k_SmallNumber) {
                const SR_MATH_NS::Quaternion rootRot = SR_MATH_NS::Quaternion::FromToRotation(oldRootToMid, newRootToMid);
                root.SetGlobalRotation(rootRot * root.GetMatrix().GetQuat());
            }
        }

        // Mid вращается так, чтобы направление от mid к tip было правильным
        if (links[1].Length > k_SmallNumber) {
            const SR_MATH_NS::FVector3 oldMidToTip = (oldTipPos - oldMidPos).NormalizeSafe();
            const SR_MATH_NS::FVector3 newMidToTip = (pA - pB).NormalizeSafe();
            
            if (oldMidToTip.SqrMagnitude() > k_SmallNumber && newMidToTip.SqrMagnitude() > k_SmallNumber) {
                const SR_MATH_NS::Quaternion midRot = SR_MATH_NS::Quaternion::FromToRotation(oldMidToTip, newMidToTip);
                mid.SetGlobalRotation(midRot * mid.GetMatrix().GetQuat());
            }
        }
    }
}