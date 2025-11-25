//
// Created by Monika on 23.11.2025.
//

#include <Graphics/IK/IKUtils.h>

#include <Utils/ECS/Transform.h>

namespace SR_GRAPH_NS::IK {
    void SolveTwoBoneIK(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    ) {
        SR_TRACY_ZONE;

        // Реализация основана на Unity Animation Rigging Package
        // https://github.com/Unity-Technologies/Animation-Rigging

        const float k_SqrEpsilon = 1e-8f;

        // 1. Получаем мировые позиции костей
        SR_MATH_NS::FVector3 aPosition = root.GetMatrix().GetTranslate();
        SR_MATH_NS::FVector3 bPosition = mid.GetMatrix().GetTranslate();
        SR_MATH_NS::FVector3 cPosition = tip.GetMatrix().GetTranslate();

        // 2. Получаем целевую позицию и вращение
        SR_MATH_NS::FVector3 targetPos = target.GetMatrix().GetTranslate();
        SR_MATH_NS::Quaternion targetRot = target.GetMatrix().GetQuat();

        // 3. Интерполируем целевую позицию с учётом веса
        SR_MATH_NS::FVector3 tPosition = cPosition.Lerp(targetPos, targetPosWeight);

        // 4. Интерполируем целевое вращение с учётом веса
        SR_MATH_NS::Quaternion tipCurrentRot = tip.GetMatrix().GetQuat();
        SR_MATH_NS::Quaternion tRotation = tipCurrentRot.Slerp(targetRot, targetRotWeight);

        // 5. Проверяем наличие hint
        bool hasHint = hintPosition.has_value() && hintWeight > 0.0f;

        // 6. Вычисляем векторы и длины сегментов
        SR_MATH_NS::FVector3 ab = bPosition - aPosition;
        SR_MATH_NS::FVector3 bc = cPosition - bPosition;
        SR_MATH_NS::FVector3 ac = cPosition - aPosition;
        SR_MATH_NS::FVector3 at = tPosition - aPosition;

        float abLen = ab.Length();
        float bcLen = bc.Length();
        float acLen = ac.Length();
        float atLen = at.Length();

        // 7. Вычисляем углы треугольников (закон косинусов)
        float oldAbcAngle = SR_MATH_NS::TriangleAngle(acLen, abLen, bcLen);
        float newAbcAngle = SR_MATH_NS::TriangleAngle(atLen, abLen, bcLen);

        // 8. Определяем ось изгиба (bend axis)
        // Стратегия: использовать то, что предоставлено в анимации для минимизации изменений конфигурации
        // Если векторы коллинеарны, пытаемся вычислить ось изгиба по желаемой позиции цели
        // Если это также не удается, используем hint если предоставлен
        SR_MATH_NS::FVector3 axis = ab.Cross(bc);
        if (axis.SqrMagnitude() < k_SqrEpsilon) {
            axis = hasHint ? (*hintPosition - aPosition).Cross(bc) : SR_MATH_NS::FVector3::Zero();

            if (axis.SqrMagnitude() < k_SqrEpsilon) {
                axis = at.Cross(bc);
            }

            if (axis.SqrMagnitude() < k_SqrEpsilon) {
                axis = SR_MATH_NS::FVector3::Up();
            }
        }
        axis = axis.Normalize();

        // 9. Вычисляем дельта-вращение для mid joint
        // Угол поворота равен половине разности между старым и новым углом треугольника
        float a = 0.5f * (oldAbcAngle - newAbcAngle);
        float sin = SR_SIN(a);
        float cos = SR_COS(a);
        SR_MATH_NS::Quaternion deltaR(sin * axis.x, sin * axis.y, sin * axis.z, cos);

        // 10. Применяем дельта-вращение к mid (относительное вращение)
        mid.SetGlobalRotation(deltaR * mid.GetMatrix().GetQuat());

        // 11. Обновляем позицию tip после вращения mid
        cPosition = tip.GetMatrix().GetTranslate();
        ac = cPosition - aPosition;

        // 12. Вычисляем и применяем вращение root (относительное вращение)
        root.SetGlobalRotation(SR_MATH_NS::Quaternion::FromToRotation(ac, at) * root.GetMatrix().GetQuat());

        // 13. Применяем hint для дополнительной коррекции
        if (hasHint) {
            float acSqrMag = ac.SqrMagnitude();
            if (acSqrMag > 0.0f) {
                // Обновляем позиции после предыдущих вращений
                bPosition = mid.GetMatrix().GetTranslate();
                cPosition = tip.GetMatrix().GetTranslate();
                ab = bPosition - aPosition;
                ac = cPosition - aPosition;

                // Нормализуем ac
                SR_MATH_NS::FVector3 acNorm = ac / SR_SQRT(acSqrMag);

                // Вычисляем проекции ab и ah на плоскость, перпендикулярную ac
                SR_MATH_NS::FVector3 ah = *hintPosition - aPosition;
                SR_MATH_NS::FVector3 abProj = ab - acNorm * ab.Dot(acNorm);
                SR_MATH_NS::FVector3 ahProj = ah - acNorm * ah.Dot(acNorm);

                float maxReach = abLen + bcLen;
                // Применяем hint только если проекции достаточно велики
                if (abProj.SqrMagnitude() > (maxReach * maxReach * 0.001f) && ahProj.SqrMagnitude() > 0.0f) {
                    // Вычисляем вращение от abProj к ahProj
                    SR_MATH_NS::Quaternion hintR = SR_MATH_NS::Quaternion::FromToRotation(abProj, ahProj);

                    // Применяем hintWeight к компонентам вращения
                    hintR.x *= hintWeight;
                    hintR.y *= hintWeight;
                    hintR.z *= hintWeight;
                    hintR = hintR.NormalizeSafe();

                    // Применяем hint-вращение к root (относительное вращение)
                    root.SetGlobalRotation(hintR * root.GetMatrix().GetQuat());
                }
            }
        }

        // 14. Применяем целевое вращение к tip
        tip.SetGlobalRotation(tRotation);
    }
}