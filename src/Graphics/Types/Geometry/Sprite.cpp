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

#include <Enum/SpriteMode.hpp>

#include <Codegen/Sprite.generated.hpp>

namespace SR_GTYPES_NS {
    bool Sprite::SetTexturePath(const SR_UTILS_NS::Path& path) {
        SR_UTILS_NS::ResourceRef<Texture> texture(path);
        if (!texture.GetResource()) {
            SR_ERROR("Sprite::SetTexturePath() : resource not found: {}", path);
            return false;
        }
        m_texture = std::move(texture);
        OnTextureChanged();
        return true;
    }

    void Sprite::OnTextureChanged() {
        m_textureReloadSubscription.Reset();
        if (auto pTexture = m_texture.GetResource()) {
            m_textureReloadSubscription = pTexture->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](auto&&) {
                SetLayoutDirty();
                MarkMaterialDirty();
            });
        }
        SetLayoutDirty();
        MarkMaterialDirty();
    }

    void Sprite::OnPostLoad() {
        UIRenderComponent::OnPostLoad();
        OnTextureChanged();
    }

    void Sprite::UseSamplers(Shader& shader) {
        UIRenderComponent::UseSamplers(shader);
        if (!m_texture.GetId().empty()) {
            auto pTexture = m_texture.GetResource();
            if (!pTexture) {
                SR_ERROR("Sprite::UseSamplers() : resource not found: {}", m_texture.GetId());
                return;
            }
            shader.SetSampler2D("diffuse", pTexture);
        }
    }

    void Sprite::CalculateLayoutInput(bool horizontal) {
        if (auto pTexture = m_texture.GetResource()) {
            auto&& pCanvas = FindCanvas(GetSceneObject().Get());
            const float_t scale = (pCanvas ? pCanvas->GetReferencePixelsPerUnit() : 100.f) / std::max(0.001f, pTexture->GetPPU());
            m_preferredSize = SR_MATH_NS::FVector2(static_cast<float_t>(pTexture->GetWidth()), static_cast<float_t>(pTexture->GetHeight())) * scale;
        }
    }

    void Sprite::Draw() {
        SR_TRACY_ZONE;

        if (m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        DrawRenderObject(this, 4, m_virtualUBO, m_virtualDescriptor, m_dirtyMaterial, m_hasErrors);
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

        shader.SetInt(SHADER_SPRITE_MODE, static_cast<int32_t>(m_spriteMode));

        if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
            SR_MATH_NS::FRect layout = pTransformRect->GetLayoutRect();
            shader.SetVec4(SHADER_NDC_RECT, layout.vec4);
            shader.SetMat4(SHADER_MODEL_MATRIX, pTransformRect->GetMatrix());
        }

        if (SR_MATH_NS::IsMaskIncludedSubMask(m_spriteMode, SpriteMode::Sliced)) {
            ApplySliceModeParams(&shader);
        }

        if (SR_MATH_NS::IsMaskIncludedSubMask(m_spriteMode, SpriteMode::Filled)) {
            ApplyFillModeParams(&shader);
        }

        Super::UseModelMatrix(shader);
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
        pShader->SetFloat(SHADER_SPRITE_FILL_AMOUNT, m_fillAmount);
        pShader->SetInt(SHADER_SPRITE_FILL_METHOD, static_cast<int>(m_fillMethod));
        pShader->SetInt(SHADER_SPRITE_FILL_ORIGIN, static_cast<int>(m_fillOrigin));
        pShader->SetInt(SHADER_SPRITE_FILL_CLOCKWISE, m_fillClockwise ? 1 : 0);
        pShader->SetInt(SHADER_SPRITE_MODE, static_cast<int>(m_spriteMode));
    }

    void Sprite::ApplySliceModeParams(Shader* pShader) {
        /// Нейтральные значения: слайсинг выключен, UV не искажаются.
        SR_MATH_NS::FVector4 windowBorder;
        SR_MATH_NS::FVector4 textureBorder;
        bool fillCenter = true;

        /// Считаем параметры в лямбде, чтобы при любом выходе гарантированно записать униформы,
        /// иначе в шейдере останутся значения от предыдущего отрисованного спрайта.
        [&] {
            if (m_sliceMode == SliceMode::None) {
                return;
            }

            float_t layoutWidth = 0.f;
            float_t layoutHeight = 0.f;

            if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
                const SR_MATH_NS::FRect layout = pTransformRect->GetLayoutRect();
                layoutWidth = layout.w;
                layoutHeight = layout.h;
            }

            if (layoutWidth <= 0.f || layoutHeight <= 0.f) {
                return;
            }

            static const SR_UTILS_NS::StringAtom diffuseAtom("diffuse");
            auto&& pTexture = GetMaterial()->GetMaterialData()->GetDefaultShaderData().GetSamplerTexture(diffuseAtom);
            if (!pTexture || !pTexture->CanBeUsed()) {
                return;
            }

            const auto textureWidth = static_cast<float_t>(pTexture->GetWidth());
            const auto textureHeight = static_cast<float_t>(pTexture->GetHeight());
            if (textureWidth <= 0.f || textureHeight <= 0.f) {
                return;
            }

            /// Границы в пикселях текстуры: какие её края не растягиваются.
            const SR_MATH_NS::FRect spriteBorder = m_sliceMode == SliceMode::Auto ? pTexture->GetBorder() : m_textureBorder;

            /// Те же границы, но в пикселях UI: сколько места углы занимают на экране.
            float_t leftUI = spriteBorder.left;
            float_t rightUI = spriteBorder.right;
            float_t bottomUI = spriteBorder.bottom;
            float_t topUI = spriteBorder.top;

            if (m_sliceMode == SliceMode::Auto) {
                auto&& pCanvas = FindCanvas(GetSceneObject().Get());
                const float_t referencePixelsPerUnit = pCanvas ? pCanvas->GetReferencePixelsPerUnit() : 100.f;
                const float_t texturePixelsPerUnit = pTexture->GetPPU();

                /// Во сколько раз пиксель текстуры крупнее пикселя UI.
                /// Множитель делит границы: чем он больше, тем тоньше рамка (как в Unity).
                const float_t multiplier = std::max(m_pixelsPerUnitMultiplier, static_cast<float_t>(SR_KINDA_SMALL_NUMBER_EPSILON));
                const float_t pixelScale = texturePixelsPerUnit > 0.f
                    ? referencePixelsPerUnit / (texturePixelsPerUnit * multiplier)
                    : 1.f / multiplier;

                leftUI *= pixelScale;
                rightUI *= pixelScale;
                bottomUI *= pixelScale;
                topUI *= pixelScale;
            }
            else if (m_windowBorder != SR_MATH_NS::FRect()) {
                /// В ручном режиме размер углов на экране можно задать независимо от границ текстуры.
                leftUI = m_windowBorder.left;
                rightUI = m_windowBorder.right;
                bottomUI = m_windowBorder.bottom;
                topUI = m_windowBorder.top;
            }

            leftUI = std::max(leftUI, 0.f);
            rightUI = std::max(rightUI, 0.f);
            bottomUI = std::max(bottomUI, 0.f);
            topUI = std::max(topUI, 0.f);

            /// Углы не должны перекрываться: если суммарно они больше спрайта, ужимаем их пропорционально.
            if (const float_t borderWidth = leftUI + rightUI; borderWidth > layoutWidth) {
                const float_t scale = layoutWidth / borderWidth;
                leftUI *= scale;
                rightUI *= scale;
            }

            if (const float_t borderHeight = bottomUI + topUI; borderHeight > layoutHeight) {
                const float_t scale = layoutHeight / borderHeight;
                bottomUI *= scale;
                topUI *= scale;
            }

            /// Границы окна - доли от размера спрайта.
            windowBorder = SR_MATH_NS::FVector4(
                leftUI / layoutWidth,
                rightUI / layoutWidth,
                bottomUI / layoutHeight,
                topUI / layoutHeight
            );

            /// Границы текстуры - доли от её размера.
            textureBorder = SR_MATH_NS::FVector4(
                SR_CLAMP(spriteBorder.left / textureWidth, 0.f, 1.f),
                SR_CLAMP(spriteBorder.right / textureWidth, 0.f, 1.f),
                SR_CLAMP(spriteBorder.bottom / textureHeight, 0.f, 1.f),
                SR_CLAMP(spriteBorder.top / textureHeight, 0.f, 1.f)
            );

            fillCenter = m_fillCenter;
        }();

        pShader->SetVec4(SHADER_SLICED_WINDOW_BORDER, windowBorder);
        pShader->SetVec4(SHADER_SLICED_TEXTURE_BORDER, textureBorder);
        pShader->SetInt(SHADER_FILL_CENTER, fillCenter ? 1 : 0);
    }

    bool IsSpriteFillOriginApplicable(const Sprite& sprite, SpriteFillOrigin origin) {
        switch (sprite.GetFillMethod()) {
            case SpriteFillMethod::Horizontal:
                return origin == SpriteFillOrigin::Left || origin == SpriteFillOrigin::Right;
            case SpriteFillMethod::Vertical:
                return origin == SpriteFillOrigin::Bottom || origin == SpriteFillOrigin::Top;
            case SpriteFillMethod::Radial90:
                return origin == SpriteFillOrigin::BottomLeft || origin == SpriteFillOrigin::TopLeft || origin == SpriteFillOrigin::TopRight || origin == SpriteFillOrigin::BottomRight;
            case SpriteFillMethod::Radial180:
            case SpriteFillMethod::Radial360:
                return origin == SpriteFillOrigin::Bottom || origin == SpriteFillOrigin::Top || origin == SpriteFillOrigin::Left || origin == SpriteFillOrigin::Right;
            default:
                break;
        }
        SRHaltOnce("Unknown fill method!");
        return false;
    }
}
