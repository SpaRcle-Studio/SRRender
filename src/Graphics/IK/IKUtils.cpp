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
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    ) {
        SR_TRACY_ZONE;

        // Реализация основана на Unity Animation Rigging Package
        // https://github.com/Unity-Technologies/Animation-Rigging

        const float k_SqrEpsilon = 1e-8f;
        //const float k_SqrEpsilon = 0.1;

        // 1. Получаем мировые позиции костей
        SR_MATH_NS::FVector3 aPosition = root.GetMatrix().Orthonormalize().GetTranslate();
        SR_MATH_NS::FVector3 bPosition = mid.GetMatrix().Orthonormalize().GetTranslate();
        SR_MATH_NS::FVector3 cPosition = tip.GetMatrix().Orthonormalize().GetTranslate();

        // 2. Получаем целевую позицию и вращение
        SR_MATH_NS::FVector3 targetPos = target.GetMatrix().Orthonormalize().GetTranslate();
        SR_MATH_NS::Quaternion targetRot = target.GetMatrix().Orthonormalize().GetQuat();

        // 3. Интерполируем целевую позицию с учётом веса
        SR_MATH_NS::FVector3 tPosition = cPosition.Lerp(targetPos, targetPosWeight);

        // 4. Интерполируем целевое вращение с учётом веса
        SR_MATH_NS::Quaternion tipCurrentRot = tip.GetMatrix().Orthonormalize().GetQuat();
        SR_MATH_NS::Quaternion tRotation = tipCurrentRot.Slerp(targetRot, targetRotWeight);


        ///SR_MATH_NS::Quaternion rootQuat = root.GetMatrix().Orthonormalize().GetQuat();
        ///SR_MATH_NS::Quaternion midQuat = mid.GetMatrix().Orthonormalize().GetQuat();
        ///SR_MATH_NS::Quaternion tipQuat = tip.GetMatrix().Orthonormalize().GetQuat();



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
            //axis = hasHint ? (*hintPosition - aPosition).Cross(bc) : SR_MATH_NS::FVector3::Zero();

            axis = ikState.previousBendAxis;
            //if (axis.SqrMagnitude() < k_SqrEpsilon) {
            //    axis = at.Cross(bc);
            //}


            //if (axis.SqrMagnitude() < k_SqrEpsilon) {
            //    axis = SR_MATH_NS::FVector3::Up();
            //}

            //if (axis.SqrMagnitude() < k_SqrEpsilon) {
            //    axis = ikState.previousBendAxis;
            //}
        }
        axis = axis.Normalize();
        ikState.previousBendAxis = axis;

        // 9. Вычисляем дельта-вращение для mid joint
        // Угол поворота равен половине разности между старым и новым углом треугольника
        float a = 0.5f * (oldAbcAngle - newAbcAngle);
        float sin = SR_SIN(a);
        float cos = SR_COS(a);
        SR_MATH_NS::Quaternion deltaR(sin * axis.x, sin * axis.y, sin * axis.z, cos);

        // 10. Применяем дельта-вращение к mid (относительное вращение)
        mid.SetGlobalRotation(deltaR * mid.GetMatrix().Orthonormalize().GetQuat());
        //midQuat = deltaR * midQuat;

        // 11. Обновляем позицию tip после вращения mid
        cPosition = tip.GetMatrix().Orthonormalize().GetTranslate();
        ac = cPosition - aPosition;

        root.SetGlobalRotation(SR_MATH_NS::Quaternion::FromToRotation(ac, at) * root.GetMatrix().Orthonormalize().GetQuat());
        ///rootQuat = SR_MATH_NS::Quaternion::FromToRotation(ac, at) * rootQuat;

        // 13. Применяем hint для дополнительной коррекции
        if (hasHint) {
            float acSqrMag = ac.SqrMagnitude();
            if (acSqrMag > 0.0f) {
                // Обновляем позиции после предыдущих вращений
                bPosition = mid.GetMatrix().Orthonormalize().GetTranslate();
                cPosition = tip.GetMatrix().Orthonormalize().GetTranslate();
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
                    root.SetGlobalRotation(hintR * root.GetMatrix().Orthonormalize().GetQuat());
                    ///rootQuat = hintR * rootQuat;
                }
            }
        }

        // 14. Применяем целевое вращение к tip
        tip.SetGlobalRotation(tRotation);
        //tipQuat = tRotation;
//
        //root.SetGlobalRotation(rootQuat);
        //mid.SetGlobalRotation(midQuat);
        //tip.SetGlobalRotation(tipQuat);
    }

    // Вспомогательная функция: проекция вектора на нормализованный вектор (аналог ProjectOnToNormal из UE)
    static SR_MATH_NS::FVector3 ProjectOnToNormal(const SR_MATH_NS::FVector3& vec, const SR_MATH_NS::FVector3& normal) {
        return normal * vec.Dot(normal);
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