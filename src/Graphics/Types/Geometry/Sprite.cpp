//
// Created by Monika on 30.07.2022.
//

#include <Graphics/Types/Geometry/Sprite.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Uniforms.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/UI/UINode.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/TransformRect.h>

#include <Codegen/Sprite.generated.hpp>

namespace SR_GTYPES_NS {
    std::string Sprite::GetMeshIdentifier() const {
        static const std::string id = "SpriteFromMemory";
        return id;
    }

    bool Sprite::Calculate() {
        if (IsCalculated()) {
            return true;
        }

        FreeVMemory();

        if (!IsCalculatable()) {
            return false;
        }

        return Super::Calculate();
    }

    void Sprite::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void Sprite::UseModelMatrix() {
        SR_TRACY_ZONE;

        auto&& pCamera = GetPipeline()->GetCurrentCamera();
        if (!pCamera) {
            return;
        }

        if (auto&& pShader = GetPipeline()->GetCurrentShader()) {
            pShader->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());

            if (auto&& pParent = dynamic_cast<SR_UTILS_NS::GameObject*>(GetParent())) SR_LIKELY_ATTRIBUTE {
                if (auto&& pTransform = pParent->GetTransform(); pTransform && pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                    SR_MATH_NS::FRect layout = static_cast<const SR_UTILS_NS::TransformRect*>(pTransform.Get())->GetLayoutRect();
                    //auto&& viewportSize = pCamera->GetViewportSize().CastToFloat();

                    //const float_t left   = (layout.x / viewportSize.x) * 2.0f - 1.0f;
                    //const float_t right  = ((layout.x + layout.w) / viewportSize.x) * 2.0f - 1.0f;
                    //const float_t top    = 1.0f - (layout.y / viewportSize.y) * 2.0f;
                    //const float_t bottom = 1.0f - ((layout.y + layout.h) / viewportSize.y) * 2.0f;
                    //pShader->SetVec4(SHADER_NDC_RECT, SR_MATH_NS::FVector4(left, right, top, bottom));

                    pShader->SetVec4(SHADER_NDC_RECT, layout.vec4);
                }
            }

            //if (auto&& pParent = dynamic_cast<SR_UTILS_NS::GameObject*>(GetParent())) SR_LIKELY_ATTRIBUTE {
            //    if (auto&& pTransform = pParent->GetTransform(); pTransform && pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
            //        SR_MATH_NS::FRect layout = static_cast<const SR_UTILS_NS::TransformRect*>(pTransform.Get())->GetLayoutRect();
            //        auto&& viewportSize = pCamera->GetViewportSize().CastToFloat();

            //        const float_t left   = (layout.x / viewportSize.x) * 2.0f - 1.0f;
            //        const float_t right  = ((layout.x + layout.w) / viewportSize.x) * 2.0f - 1.0f;
            //        const float_t top    = 1.0f - (layout.y / viewportSize.y) * 2.0f;
            //        const float_t bottom = 1.0f - ((layout.y + layout.h) / viewportSize.y) * 2.0f;

            //        pShader->SetVec4(SHADER_NDC_RECT, SR_MATH_NS::FVector4(left, right, top, bottom));
            //    }
            //}

            if (m_sliced) {
                pShader->SetVec2(SHADER_SLICED_TEXTURE_BORDER, m_textureBorder);
                pShader->SetVec2(SHADER_SLICED_WINDOW_BORDER, m_windowBorder);
            }
        }
        else {
            SRHaltOnce("Shader is nullptr!");
        }
        Super::UseModelMatrix();
    }

    bool Sprite::BindMesh() {
        return true;
    }

    bool Sprite::IsSupportVBO() const {
        return false;
    }

    MeshType Sprite::GetMeshType() const noexcept {
        return MeshType::Sprite;
    }

    uint32_t Sprite::GetIndicesCount() const {
        return 4;
    }

    bool Sprite::IsFlatMesh() const noexcept {
        return true;
    }

    SR_MATH_NS::FVector2 Sprite::GetTextureBorder() const {
        return m_textureBorder;
    }

    SR_MATH_NS::FVector2 Sprite::GetWindowBorder() const {
        return m_windowBorder;
    }

    void Sprite::SetTextureBorder(const SR_MATH_NS::FVector2& border) {
        m_textureBorder = border;
        MarkUniformsDirty();
    }

    void Sprite::SetWindowBorder(const SR_MATH_NS::FVector2& border) {
        m_windowBorder = border;
        MarkUniformsDirty();
    }
}
