//
// Created by Monika on 30.07.2022.
//

#include <Graphics/Types/Geometry/Sprite.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/TransformRect.h>

#include <Codegen/Sprite.generated.hpp>

namespace SR_GTYPES_NS {
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

    void Sprite::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void Sprite::UseModelMatrix(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;

        auto&& pCamera = GetPipeline()->GetCurrentCamera();
        auto&& pMaterial = GetMaterial();
        if (!pCamera || !pMaterial) {
            return;
        }

        shader.SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
        shader.SetInt(SHADER_SPRITE_MODE, static_cast<int32_t>(m_spriteMode));
        switch (m_spriteMode) {
            case SpriteMode::Filled:
                ApplyFillModeParams(&shader);
                break;
            case SpriteMode::Sliced:
                ApplySliceModeParams(&shader);
                break;
            default:
                SRHaltOnce("Unknown sprite mode!");
                break;
        }

        Super::UseModelMatrix(shader);
    }

    bool Sprite::BindMesh() {
        return true;
    }

    bool Sprite::IsSupportVBO() const {
        return false;
    }

    uint32_t Sprite::GetIndicesCount() const {
        return 4;
    }

    SR_MATH_NS::FRect Sprite::GetTextureBorder() const {
        return m_textureBorder;
    }

    SR_MATH_NS::FRect Sprite::GetWindowBorder() const {
        return m_windowBorder;
    }

    void Sprite::SetTextureBorder(const SR_MATH_NS::FRect& border) {
        m_textureBorder = border;
        MarkUniformsDirty();
    }

    void Sprite::SetWindowBorder(const SR_MATH_NS::FRect& border) {
        m_windowBorder = border;
        MarkUniformsDirty();
    }

    void Sprite::SetSliceMode(SliceMode mode) {
        m_sliceMode = mode;
        MarkUniformsDirty();
    }

    void Sprite::SetSpriteMode(SpriteMode mode) {
        m_spriteMode = mode;
        MarkUniformsDirty();
    }

    void Sprite::SetFillCenter(bool fill) {
        m_fillCenter = fill;
        MarkUniformsDirty();
    }

    void Sprite::SetPixelsPerUnitMultiplier(float_t multiplier) {
        m_pixelsPerUnitMultiplier = multiplier;
        MarkUniformsDirty();
    }

    void Sprite::SetFillMethod(SpriteFillMethod method) {
        m_fillMethod = method;
        MarkUniformsDirty();
    }

    void Sprite::SetFillOrigin(SpriteFillOrigin origin) {
        m_fillOrigin = origin;
        MarkUniformsDirty();
    }

    void Sprite::SetFillAmount(float_t amount) {
        m_fillAmount = amount;
        MarkUniformsDirty();
    }

    void Sprite::SetFillClockwise(bool clockwise) {
        m_fillClockwise = clockwise;
        MarkUniformsDirty();
    }

    void Sprite::ApplyFillModeParams(Shader* pShader) {

    }

    void Sprite::ApplySliceModeParams(Shader* pShader) {
        float_t layoutWidth = 0.f;
        float_t layoutHeight = 0.f;

        static const SR_UTILS_NS::StringAtom diffuseAtom("diffuse");
        auto&& pTexture = GetMaterial()->GetMaterialData()->GetDefaultShaderData().GetSamplerTexture(diffuseAtom);
        if (!pTexture || !pTexture->CanBeUsed()) {
            return;
        }

        const float_t effectivePPU = std::max(pTexture->GetPPU() / 100.f * m_pixelsPerUnitMultiplier, static_cast<float_t>(SR_KINDA_SMALL_NUMBER_EPSILON));

        if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
            SR_MATH_NS::FRect layout = pTransformRect->GetLayoutRect();
            layoutWidth = layout.w;
            layoutHeight = layout.h;
            pShader->SetVec4(SHADER_NDC_RECT, layout.vec4);
        }

        if (m_sliceMode == SliceMode::Manual) {
            if (layoutWidth > 0.0f && layoutHeight > 0.0f) {
                const SR_MATH_NS::FVector4 windowBorder = {
                    m_windowBorder.left   / layoutWidth, m_windowBorder.right  / layoutWidth,
                    m_windowBorder.bottom / layoutHeight, m_windowBorder.top    / layoutHeight
                };
                pShader->SetVec4(SHADER_SLICED_WINDOW_BORDER, windowBorder);
                pShader->SetVec4(SHADER_SLICED_TEXTURE_BORDER, m_textureBorder.vec4);
            }
        }
        else if (m_sliceMode == SliceMode::Auto) {
            const SR_MATH_NS::FRect spriteBorder = pTexture->GetBorder();

            if (layoutWidth > 0.0f && layoutHeight > 0.0f && effectivePPU > 0.0f) {
                // Конвертируем границы текстуры из пикселей в единицы UI
                float_t leftUI   = spriteBorder.left   / effectivePPU;
                float_t rightUI  = spriteBorder.right  / effectivePPU;
                float_t bottomUI = spriteBorder.bottom / effectivePPU;
                float_t topUI    = spriteBorder.top    / effectivePPU;

                /// --------------------- добавляем корректировку, чтобы углы не перекрывались ---------------------
                float_t halfWidth  = layoutWidth  * 0.5f;
                float_t halfHeight = layoutHeight * 0.5f;

                leftUI   = std::min(leftUI,   halfWidth);
                rightUI  = std::min(rightUI,  halfWidth);
                bottomUI = std::min(bottomUI, halfHeight);
                topUI    = std::min(topUI,    halfHeight);
                /// ------------------------------------------------------------------------------------------------

                // Границы окна в нормализованных координатах (0-1)
                // Это показывает, где находятся границы в окне относительно его размера
                // Ограничиваем значения до 1.0, чтобы избежать проблем, когда окно меньше границ текстуры
                const SR_MATH_NS::FVector4 windowBorder = {
                    std::min(1.0f, leftUI   / layoutWidth),  // left border в нормализованных координатах
                    std::min(1.0f, rightUI  / layoutWidth),  // right border в нормализованных координатах
                    std::min(1.0f, bottomUI / layoutHeight), // bottom border в нормализованных координатах
                    std::min(1.0f, topUI    / layoutHeight)  // top border в нормализованных координатах
                };

                // Границы текстуры в нормализованных координатах (0-1)
                const SR_MATH_NS::FVector4 textureBorder = {
                    spriteBorder.left   / static_cast<float_t>(pTexture->GetWidth()),   // left в нормализованных координатах
                    spriteBorder.right  / static_cast<float_t>(pTexture->GetWidth()),   // right в нормализованных координатах
                    spriteBorder.bottom / static_cast<float_t>(pTexture->GetHeight()),  // bottom в нормализованных координатах
                    spriteBorder.top    / static_cast<float_t>(pTexture->GetHeight())   // top в нормализованных координатах
                };

                pShader->SetVec4(SHADER_SLICED_WINDOW_BORDER, windowBorder);
                pShader->SetVec4(SHADER_SLICED_TEXTURE_BORDER, textureBorder);
            }
        }
        else {
            pShader->SetVec4(SHADER_SLICED_WINDOW_BORDER, SR_MATH_NS::FVector4());
            pShader->SetVec4(SHADER_SLICED_TEXTURE_BORDER, SR_MATH_NS::FVector4());
        }

        pShader->SetInt(SHADER_FILL_CENTER, m_fillCenter || m_sliceMode == SliceMode::None ? 1 : 0);
    }
}
