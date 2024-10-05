//
// Created by Monika on 14.02.2022.
//

#include <Graphics/Font/ITextComponent.h>
#include <Graphics/Font/Font.h>
#include <Graphics/Font/TextBuilder.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/ComponentManager.h>
#include <Utils/ECS/Transform.h>
#include <Utils/ECS/Transform2D.h>
#include <Utils/Localization/Encoding.h>

namespace SR_GTYPES_NS {
    ITextComponent::ITextComponent()
        : IMeshComponent(this)
    {
        m_entityMessages.AddStandardProperty<SR_MATH_NS::UVector2>("Atlas size", &m_atlasSize)
            .SetDontSave()
            .SetReadOnly();
    }

    bool ITextComponent::InitializeEntity() noexcept {
        GetComponentProperties().AddStandardProperty("Is 3D", &m_is3D)
            .SetSetter([this](void* pValue) { m_is3D = *static_cast<bool*>(pValue); ReRegisterMesh(); });

        GetComponentProperties().AddStandardProperty("Use localization", &m_localization)
            .SetSetter([this](void* pValue) { SetUseLocalization(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Use preprocessor", &m_preprocessor)
            .SetSetter([this](void* pValue) { SetUsePreprocessor(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Use kerning", &m_kerning)
            .SetSetter([this](void* pValue) { SetKerning(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Debug", &m_debug)
            .SetSetter([this](void* pValue) { SetDebug(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Font size", &m_fontSize)
            .SetDrag(1)
            .SetResetValue(512.f)
            .SetWidth(90.f);

        GetComponentProperties().AddStandardProperty("Text", &m_text)
            .SetSetter([this](auto&& text) { SetText(*static_cast<SR_HTYPES_NS::UnicodeString*>(text)); })
            .SetMultiline();

        GetComponentProperties().AddCustomProperty<SR_UTILS_NS::PathProperty>("Font")
            .AddFileFilter("Mesh", SR_GRAPH_NS::SR_SUPPORTED_FONT_FORMATS)
            .SetGetter([this]()-> SR_UTILS_NS::Path {
                return m_font ? m_font->GetResourcePath() : SR_UTILS_NS::Path();
            })
            .SetSetter([this](const SR_UTILS_NS::Path& path) {
                SetFont(path);
            });

        return IMeshComponent::InitializeEntity();
    }

    RenderScene* ITextComponent::GetTextRenderScene() const {
        return TryGetRenderScene();
    }

    int64_t ITextComponent::GetSortingPriority() const {
        if (auto&& pTransform = GetTransform()) {
            if (pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                return static_cast<SR_UTILS_NS::Transform2D*>(pTransform)->GetPriority();
            }
        }

        return -1;
    }

    bool ITextComponent::HasSortingPriority() const {
        if (auto&& pTransform = GetTransform()) {
            return pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
        }
        return false;
    }

    SR_UTILS_NS::StringAtom ITextComponent::GetMeshLayer() const {
        if (!m_sceneObject) {
            return SR_UTILS_NS::StringAtom();
        }

        return m_sceneObject->GetLayer();
    }

    const SR_MATH_NS::Matrix4x4& ITextComponent::GetMatrix() const {
        if (auto&& pTransform = GetTransform()) {
            return pTransform->GetMatrix();
        }

        return Mesh::GetMatrix();
    }
}
