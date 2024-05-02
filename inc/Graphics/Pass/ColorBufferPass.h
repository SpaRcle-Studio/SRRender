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
        bool Render() override;
        void Update() override;
        void PostUpdate() override;

        bool Load(const SR_XML_NS::Node& passNode) override;

        SR_NODISCARD SR_GTYPES_NS::Framebuffer* GetColorFrameBuffer() const noexcept override;
        SR_NODISCARD bool IsNeedUseMaterials() const noexcept override { return false; }
        SR_NODISCARD bool IsNeedUpdate() const noexcept override;

        void UseConstants(ShaderUseInfo info) override;

    protected:
        void UseUniforms(ShaderUseInfo info, MeshPtr pMesh) override;

    protected:
        bool m_needUpdateUniforms = true;

    };
}

#endif //SR_ENGINE_COLORBUFFERPASS_H
