//
// Created by Monika on 14.02.2022.
//

#ifndef SR_ENGINE_I_TEXT_COMPONENT_H
#define SR_ENGINE_I_TEXT_COMPONENT_H

#include <Graphics/Font/IText.h>
#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Types/Geometry/MeshComponent.h>
#include <Utils/ECS/ComponentManager.h>

namespace SR_GTYPES_NS {
    class ITextComponent : public IMeshComponent, public IText {
    public:
        ITextComponent();

    public:
        bool InitializeEntity() noexcept override;

        SR_NODISCARD RenderScene* GetTextRenderScene() const override;

        SR_NODISCARD SR_FORCE_INLINE bool IsMeshActive() const noexcept override {
            return IMeshComponent::IsActive() && IText::IsMeshActive();
        }

        SR_NODISCARD int64_t GetSortingPriority() const override;
        SR_NODISCARD bool HasSortingPriority() const override;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetMeshLayer() const override;
        const SR_MATH_NS::Matrix4x4& GetMatrix() const override;

    };

    class Text final : public ITextComponent {
        SR_REGISTER_NEW_COMPONENT(Text, 1000)
    };
}

#endif //SR_ENGINE_I_TEXT_COMPONENT_H
