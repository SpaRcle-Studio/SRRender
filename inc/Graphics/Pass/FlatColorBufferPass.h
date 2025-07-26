//
// Created by Monika on 18.11.2023.
//

#ifndef SR_ENGINE_GRAPHICS_FLAT_COLOR_BUFFER_PASS_H
#define SR_ENGINE_GRAPHICS_FLAT_COLOR_BUFFER_PASS_H

#include <Graphics/Pass/IColorBufferPass.h>
#include <Graphics/Pass/FlatClusterPass.h>

namespace SR_GRAPH_NS {
    /*class FlatColorBufferPass : public FlatClusterPass, public IColorBufferPass, public IFramebufferPass {
        using Super = FlatClusterPass;
    public:
        void Update() override;
        bool Render() override;
        bool Init() override;
        bool Load(const SR_XML_NS::Node& passNode) override;
        void OnResize(const SR_MATH_NS::UVector2& size) override;

        SR_NODISCARD IRenderTechnique* GetFrameBufferRenderTechnique() const override { return GetTechnique(); }

    public:
        SR_NODISCARD SR_GTYPES_NS::Framebuffer* GetColorFrameBuffer() const noexcept override;
        SR_NODISCARD std::vector<SR_GTYPES_NS::Framebuffer*> GetFrameBuffers() const override;

        void UseConstants(SR_GTYPES_NS::Shader* pShader) override;
        void UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh) override;

    };*/
}

#endif //SR_ENGINE_GRAPHICS_FLAT_COLOR_BUFFER_PASS_H
