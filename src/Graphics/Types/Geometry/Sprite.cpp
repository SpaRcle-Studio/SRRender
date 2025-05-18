//
// Created by Monika on 30.07.2022.
//

#include <Graphics/Types/Geometry/Sprite.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Uniforms.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Utils/MeshUtils.h>

#include <Codegen/Sprite.generated.hpp>

namespace SR_GTYPES_NS {
    std::string Sprite::GetMeshIdentifier() const {
        static const std::string id = "SpriteFromMemory";
        return id;
    }

    bool Sprite::InitializeEntity() noexcept {
        return Super::InitializeEntity();
    }

    bool Sprite::Calculate() {
        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

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
        if (auto&& pShader = GetRenderContext()->GetCurrentShader()) {
            pShader->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());

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
