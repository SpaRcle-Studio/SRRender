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

    class ColorBufferPass : public OffScreenMeshDrawerPass, public IColorBufferPass {
        SR_REGISTER_LOGICAL_NODE(ColorBufferPass, Color Buffer Pass, { "Passes" })
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using FramebufferPtr = SR_GTYPES_NS::Framebuffer*;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using Super = OffScreenMeshDrawerPass;
        friend class ColorBufferRenderQueue;
    public:
        bool Render() override;
        void Update() override;

        bool Load(const SR_XML_NS::Node& passNode) override;

        SR_NODISCARD SR_GTYPES_NS::Framebuffer* GetColorFrameBuffer() const noexcept override;
        SR_NODISCARD bool IsNeedUseMaterials() const noexcept override { return false; }

        void UseConstants(ShaderUseInfo info) override;

    protected:
        void UseUniforms(ShaderUseInfo info, MeshPtr pMesh) override;

        SR_NODISCARD RenderQueuePtr AllocateRenderQueue() override;

    };
}

#endif //SR_ENGINE_COLORBUFFERPASS_H
