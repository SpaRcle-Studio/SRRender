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

        // 1. Получаем мировые позиции костей
        const SR_MATH_NS::FVector3 rootPos = root.GetMatrix().GetTranslate();
        const SR_MATH_NS::FVector3 midPos  = mid.GetMatrix().GetTranslate();
        const SR_MATH_NS::FVector3 tipPos  = tip.GetMatrix().GetTranslate();

        // 2. Вычисляем длины сегментов (в мировом пространстве)
        const float len1 = (midPos - rootPos).Length();
        const float len2 = (tipPos - midPos).Length();

        if (len1 < SR_FLOAT_EPSILON || len2 < SR_FLOAT_EPSILON) {
            return; // Некорректные кости
        }

        const float maxReach = len1 + len2;
        const float minReach = fabsf(len1 - len2);

        // 3. Целевая позиция с учётом веса
        SR_MATH_NS::FVector3 targetPos = target.GetMatrix().GetTranslate();
        SR_MATH_NS::FVector3 finalTarget = tipPos.Lerp(targetPos, targetPosWeight);

        // 4. Вектор от root к target и расстояние
        SR_MATH_NS::FVector3 rootToTarget = finalTarget - rootPos;
        float dist = rootToTarget.Length();

        // Ограничиваем расстояние до достижимого диапазона
        if (dist > maxReach) {
            dist = maxReach;
            finalTarget = rootPos + rootToTarget.Normalize() * dist;
        } else if (dist < minReach && minReach > SR_FLOAT_EPSILON) {
            dist = minReach;
            finalTarget = rootPos + rootToTarget.Normalize() * dist;
        }

        // 5. Вычисляем желаемую позицию локтя (mid joint) используя закон косинусов
        float x = (len1 * len1 - len2 * len2 + dist * dist) / (2.0f * dist);
        x = SR_MATH_NS::Clamp(x, 0.0f, len1);

        float h2 = len1 * len1 - x * x;
        float h = (h2 > 0.0f) ? SR_SQRT(h2) : 0.0f;

        // 6. Определяем плоскость сгиба (bend plane) и референсное направление для коррекции перекручивания
        SR_MATH_NS::FVector3 rootToTargetDir = (finalTarget - rootPos).Normalize();
        SR_MATH_NS::FVector3 bendNormal;
        SR_MATH_NS::FVector3 referenceUp; // Референсное направление "вверх" для коррекции перекручивания

        // Получаем текущее вращение root для определения референсного направления
        SR_MATH_NS::Quaternion rootCurrentRot = root.GetMatrix().GetQuat();

        // Референсное направление "вверх" в локальном пространстве root (обычно это Y или Z)
        // Предполагаем, что локальное "вверх" - это Y (0, 1, 0) или можно использовать hint
        SR_MATH_NS::FVector3 localUp = SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f);

        // Преобразуем локальное "вверх" в текущее мировое пространство
        SR_MATH_NS::FVector3 currentWorldUp = localUp.Rotate(rootCurrentRot);

        // Проецируем на плоскость, перпендикулярную направлению кости
        SR_MATH_NS::FVector3 currentBoneDir = (midPos - rootPos).Normalize();
        SR_MATH_NS::FVector3 projectedUp = currentWorldUp - currentBoneDir * currentWorldUp.Dot(currentBoneDir);
        if (projectedUp.LengthSqr() > SR_SQR_EPSILON) {
            referenceUp = projectedUp.Normalize();
        } else {
            // Fallback: используем произвольное перпендикулярное направление
            referenceUp = currentBoneDir.ArbitraryPerpendicular();
        }

        if (hintPosition && hintWeight > 0.0f) {
            // Используем hint для определения плоскости сгиба
            SR_MATH_NS::FVector3 hintVec = (*hintPosition - rootPos);
            SR_MATH_NS::FVector3 hintProj = hintVec - rootToTargetDir * hintVec.Dot(rootToTargetDir);

            if (hintProj.LengthSqr() > SR_SQR_EPSILON) {
                bendNormal = rootToTargetDir.Cross(hintProj).Normalize();
                // Обновляем referenceUp на основе hint
                referenceUp = hintProj.Normalize();
            } else {
                // Fallback: используем текущую плоскость сгиба
                SR_MATH_NS::FVector3 curAB = (midPos - rootPos).Normalize();
                SR_MATH_NS::FVector3 curBC = (tipPos - midPos).Normalize();
                SR_MATH_NS::FVector3 curNormal = curAB.Cross(curBC);
                if (curNormal.LengthSqr() > SR_SQR_EPSILON) {
                    bendNormal = curNormal.Normalize();
                } else {
                    bendNormal = rootToTargetDir.ArbitraryPerpendicular();
                }
            }
        } else {
            // Используем текущую плоскость сгиба
            SR_MATH_NS::FVector3 curAB = (midPos - rootPos).Normalize();
            SR_MATH_NS::FVector3 curBC = (tipPos - midPos).Normalize();
            SR_MATH_NS::FVector3 curNormal = curAB.Cross(curBC);
            if (curNormal.LengthSqr() > SR_SQR_EPSILON) {
                bendNormal = curNormal.Normalize();
            } else {
                bendNormal = rootToTargetDir.ArbitraryPerpendicular();
            }
        }

        // 7. Вычисляем желаемую позицию mid joint
        SR_MATH_NS::FVector3 projPoint = rootPos + rootToTargetDir * x;
        SR_MATH_NS::FVector3 tangent = bendNormal.Cross(rootToTargetDir).Normalize();

        // Сохраняем знак изгиба (в какую сторону сгибается локоть)
        SR_MATH_NS::FVector3 curMidOffset = midPos - projPoint;
        float sign = (curMidOffset.Dot(tangent) >= 0.0f) ? 1.0f : -1.0f;

        SR_MATH_NS::FVector3 desiredMidPos = projPoint + tangent * (h * sign);

        // 8. Вычисляем абсолютные вращения для root и mid
        SR_MATH_NS::FVector3 desiredRootToMid = (desiredMidPos - rootPos);
        if (desiredRootToMid.SqrMagnitude() < SR_SQR_EPSILON) {
            return; // Некорректная позиция
        }
        desiredRootToMid = desiredRootToMid.Normalize();

        // Получаем локальное направление root к mid (в локальном пространстве root)
        SR_MATH_NS::FVector3 localRootToMid = mid.GetTranslation();
        float localRootToMidLen = localRootToMid.Length();
        if (localRootToMidLen < SR_FLOAT_EPSILON) {
            return; // Некорректная структура костей
        }
        SR_MATH_NS::FVector3 localRootToMidDir = localRootToMid / localRootToMidLen;

        // Вычисляем базовое вращение root (без учета перекручивания)
        SR_MATH_NS::Quaternion rootBaseRot = SR_MATH_NS::Quaternion::FromTo(localRootToMidDir, desiredRootToMid);

        // Коррекция перекручивания: минимизируем перекручивание вокруг оси кости
        // Вычисляем желаемое направление "вверх" в новом вращении
        SR_MATH_NS::FVector3 desiredWorldUp = localUp.Rotate(rootBaseRot);
        SR_MATH_NS::FVector3 desiredProjectedUp = desiredWorldUp - desiredRootToMid * desiredWorldUp.Dot(desiredRootToMid);

        if (desiredProjectedUp.LengthSqr() > SR_SQR_EPSILON) {
            desiredProjectedUp = desiredProjectedUp.Normalize();

            // Вычисляем угол перекручивания между текущим и желаемым "вверх"
            float twistAngle = SR_ACOS(SR_MATH_NS::Clamp(referenceUp.Dot(desiredProjectedUp), -1.0f, 1.0f));

            // Ограничиваем перекручивание (можно сделать настраиваемым параметром)
            const float maxTwistAngle = SR_PI * 0.25f; // 45 градусов максимум
            if (twistAngle > maxTwistAngle) {
                // Вычисляем ось перекручивания (вдоль кости)
                SR_MATH_NS::FVector3 twistAxis = desiredRootToMid;

                // Вычисляем коррекцию перекручивания
                float correctionAngle = twistAngle - maxTwistAngle;
                SR_MATH_NS::FVector3 cross = referenceUp.Cross(desiredProjectedUp);
                if (cross.Dot(twistAxis) < 0.0f) {
                    correctionAngle = -correctionAngle;
                }

                SR_MATH_NS::Quaternion twistCorrection(twistAxis, correctionAngle);
                rootBaseRot = twistCorrection * rootBaseRot;
            }
        }

        SR_MATH_NS::Quaternion rootGlobalDesired = rootBaseRot;

        // Получаем текущее вращение для интерполяции (но используем вес 1.0 для полной замены)
        SR_MATH_NS::Quaternion newRootRot = SR_MATH_NS::Quaternion::Slerp(rootCurrentRot, rootGlobalDesired, 1.0f);

        root.SetGlobalRotation(newRootRot);

        // 9. Вычисляем вращение mid
        SR_MATH_NS::FVector3 desiredMidToTip = (finalTarget - desiredMidPos);
        if (desiredMidToTip.SqrMagnitude() < SR_SQR_EPSILON) {
            if (targetRotWeight > 0.0f) {
                SR_MATH_NS::Quaternion targetRot = target.GetMatrix().GetQuat();
                SR_MATH_NS::Quaternion tipCurrentRot = tip.GetMatrix().GetQuat();
                SR_MATH_NS::Quaternion finalTipRot = SR_MATH_NS::Quaternion::Slerp(tipCurrentRot, targetRot, targetRotWeight);
                tip.SetGlobalRotation(finalTipRot);
            }
            return;
        }
        desiredMidToTip = desiredMidToTip.Normalize();

        // Получаем локальное направление mid к tip (в локальном пространстве mid)
        SR_MATH_NS::FVector3 localMidToTip = tip.GetTranslation();
        float localMidToTipLen = localMidToTip.Length();
        if (localMidToTipLen < SR_FLOAT_EPSILON) {
            return; // Некорректная структура костей
        }
        SR_MATH_NS::FVector3 localMidToTipDir = localMidToTip / localMidToTipLen;

        // Вычисляем абсолютное мировое вращение mid
        SR_MATH_NS::Quaternion midGlobalDesired = SR_MATH_NS::Quaternion::FromTo(localMidToTipDir, desiredMidToTip);

        // Получаем текущее вращение для интерполяции (но используем вес 1.0 для полной замены)
        SR_MATH_NS::Quaternion midCurrentRot = mid.GetMatrix().GetQuat();
        SR_MATH_NS::Quaternion newMidRot = SR_MATH_NS::Quaternion::Slerp(midCurrentRot, midGlobalDesired, 1.0f);

        mid.SetGlobalRotation(newMidRot);

        // 10. Применяем вращение tip с учётом targetRotWeight
        if (targetRotWeight > 0.0f) {
            SR_MATH_NS::Quaternion targetRot = target.GetMatrix().GetQuat();
            SR_MATH_NS::Quaternion tipCurrentRot = tip.GetMatrix().GetQuat();
            SR_MATH_NS::Quaternion finalTipRot = SR_MATH_NS::Quaternion::Slerp(tipCurrentRot, targetRot, targetRotWeight);
            tip.SetGlobalRotation(finalTipRot);
        }
    }
}