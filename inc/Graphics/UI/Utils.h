//
// Created by Monika on 01.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_UI_UTILS_H
#define SR_ENGINE_GRAPHICS_UI_UTILS_H

#include <Graphics/Types/Camera.h>

#include <Utils/ECS/TransformRect.h>

namespace SR_GRAPH_NS::UI {
    SR_GRAPHICS_DLL_API SR_MATH_NS::Ray ScreenPointToRay(SR_GTYPES_NS::Camera* pCamera, SR_MATH_NS::FVector2 screenPos);
    SR_GRAPHICS_DLL_API bool ScreenPointToWorldPointInRectangle(SR_UTILS_NS::TransformRect& rect, SR_MATH_NS::FVector2 screenPoint, SR_GTYPES_NS::Camera* pCamera, SR_MATH_NS::FVector3& outWorldPoint);
    SR_GRAPHICS_DLL_API bool ScreenPointToLocalPointInRectangle(SR_UTILS_NS::TransformRect& rect, SR_MATH_NS::FVector2 screenPoint, SR_GTYPES_NS::Camera* pCamera, SR_MATH_NS::FVector2& outLocalPoint);
    SR_GRAPHICS_DLL_API bool RectangleContainsScreenPoint(SR_UTILS_NS::TransformRect& rect, SR_MATH_NS::FVector2 screenPoint, SR_GTYPES_NS::Camera* pCamera);
    SR_GRAPHICS_DLL_API bool RectangleContainsScreenPoint(SR_UTILS_NS::TransformRect& rect, SR_MATH_NS::FVector2 screenPoint);
}

#endif //SR_ENGINE_GRAPHICS_UI_UTILS_H
