//
// Created by Monika on 10.10.2022.
//

#ifndef SR_ENGINE_COLOR_BUFFER_PASS_H
#define SR_ENGINE_COLOR_BUFFER_PASS_H

#include <Graphics/Pass/Data/FrameBufferPassData.h>
#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Pass/IColorBufferPass.h>
#include <Graphics/Render/RenderQueue.h>

namespace SR_GRAPH_NS {
    class ColorBufferRenderQueue : public RenderQueue {
        using Super = RenderQueue;
    public:
        ColorBufferRenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer);

        void CustomDrawMesh(const MeshInfo& info) override;

    };

    class ColorBufferPass : public IColorBufferPass, public MeshDrawerPass {
        SR_CLASS()
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using Super = MeshDrawerPass;
        friend class ColorBufferRenderQueue;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<ColorBufferPass>;

    public:
        bool Render() override;

        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& GetColorFrameBuffer() const noexcept override;

        void UseConstants(SR_GTYPES_NS::Shader& shader) override;
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;
        void OnResize(const SR_MATH_NS::UVector2& size) override;

    protected:
        void UseUniforms(SR_GTYPES_NS::Shader& shader, SR_GTYPES_NS::IRenderComponent* pObject) override;

        SR_NODISCARD RenderQueuePtr AllocateRenderQueue(uint32_t index) override;

        /// @virtualProperty(colorMultiplier) @getter(GetColorMultiplier) @setter(SetColorMultiplier)
        SR_VIRTUAL_PROPERTY

    };
}

#endif //SR_ENGINE_COLORBUFFERPASS_H
