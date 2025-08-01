//
// Created by Monika on 10.10.2022.
//

#ifndef SR_ENGINE_COLOR_BUFFER_PASS_H
#define SR_ENGINE_COLOR_BUFFER_PASS_H

#include <Graphics/Pass/OffScreenMeshDrawerPass.h>
#include <Graphics/Pass/IColorBufferPass.h>

namespace SR_GRAPH_NS {
    class ColorBufferRenderQueue : public RenderQueue {
        using Super = RenderQueue;
    public:
        ColorBufferRenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer);

        void CustomDrawMesh(const MeshInfo& info) override;

    };

    class ColorBufferPass : public IColorBufferPass, public MeshDrawerPass { /// public OffScreenMeshDrawerPass,
        SR_CLASS()
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using Super = MeshDrawerPass;
        friend class ColorBufferRenderQueue;
    public:
        bool Render() override;

        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>&  GetColorFrameBuffer() const noexcept override;

        void UseConstants(SR_GTYPES_NS::Shader* pShader) override;

    protected:
        void UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh) override;

        SR_NODISCARD RenderQueuePtr AllocateRenderQueue() override;

        /// @virtualProperty(colorMultiplier) @getter(GetColorMultiplier) @setter(SetColorMultiplier)
        SR_VIRTUAL_PROPERTY

    };
}

#endif //SR_ENGINE_COLORBUFFERPASS_H
