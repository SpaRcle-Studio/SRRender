//
// Created by Monika on 10.10.2022.
//

#ifndef SR_ENGINE_COLOR_BUFFER_PASS_H
#define SR_ENGINE_COLOR_BUFFER_PASS_H

#include <Graphics/Pass/OffScreenMeshDrawerPass.h>
#include <Graphics/Pass/IColorBufferPass.h>

namespace SR_GRAPH_NS {
    class ColorBufferPass : public OffScreenMeshDrawerPass, public IColorBufferPass {
        SR_REGISTER_LOGICAL_NODE(ColorBufferPass, Color Buffer Pass, { "Passes" })
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using FramebufferPtr = SR_GTYPES_NS::Framebuffer*;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using Super = OffScreenMeshDrawerPass;
    public:
        void Update() override;

        SR_NODISCARD SR_GTYPES_NS::Framebuffer* GetColorFrameBuffer() const noexcept override;
        SR_NODISCARD bool IsNeedUseMaterials() const noexcept override { return false; }

    protected:
        void UseUniforms(ShaderPtr pShader, MeshPtr pMesh) override;

    };
}

#endif //SR_ENGINE_COLORBUFFERPASS_H
