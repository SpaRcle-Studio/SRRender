//
// Created by Monika on 10.10.2022.
//

#include <Graphics/Pass/ColorBufferPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Mesh.h>
#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/ColorBufferPass.generated.hpp>

namespace SR_GRAPH_NS {
    ColorBufferRenderQueue::ColorBufferRenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer)
        : Super(pStrategy, pDrawer)
    {
        m_customMeshDraw = true;
    }

    void ColorBufferRenderQueue::CustomDrawMesh(const MeshInfo& info) {
        auto pColorBuffer = static_cast<ColorBufferPass*>(GetMeshDrawerPass());
        pColorBuffer->IncrementColorIndex();
        pColorBuffer->SetMeshIndex(info.pMesh);
        info.pShader->SetConstVec3(SHADER_PC_COLOR_BUFFER_VALUE, pColorBuffer->GetMeshColor());
        info.pMesh->Draw();
    }

    void ColorBufferPass::UseUniforms(SR_GTYPES_NS::Shader& shader, MeshPtr pMesh) {
        SR_TRACY_ZONE;
        Super::UseUniforms(shader, pMesh);
    }

    MeshDrawerPass::RenderQueuePtr ColorBufferPass::AllocateRenderQueue(uint32_t index) {
        return GetRenderStrategy()->BuildQueue<ColorBufferRenderQueue, RenderQueue>(this);
    }

    const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& ColorBufferPass::GetColorFrameBuffer() const noexcept {
        if (auto&& pParent = GetParent()) {
            return dynamic_cast<FrameBufferPass*>(pParent)->GetFrameBuffer();
        }
        SR_WARN("ColorBufferPass::GetColorFrameBuffer() : parent is not FrameBufferPass!");
        static SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer> nullValue;
        return nullValue;
    }

    void ColorBufferPass::UseConstants(SR_GTYPES_NS::Shader& shader) {
        Super::UseConstants(shader);
    }

    void ColorBufferPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        Super::UseSharedUniforms(shader);
    }

    void ColorBufferPass::OnResize(const SR_MATH_NS::UVector2& size) {
        ResetColorIndex();
        ClearTable();
        DestroyRequests();
        Super::OnResize(size);
    }

    bool ColorBufferPass::Render() {
        ResetColorIndex();
        ClearTable();
        return Super::Render();
    }
}
