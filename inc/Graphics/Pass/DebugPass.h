//
// Created by Monika on 19.09.2022.
//

#ifndef SR_ENGINE_GRAPHICS_DEBUG_PASS_H
#define SR_ENGINE_GRAPHICS_DEBUG_PASS_H

#include <Graphics/Pass/IMesh3DClusterPass.h>
#include <Graphics/Render/DebugRenderer.h>

namespace SR_GRAPH_NS {
    class DebugPass : public BasePass {
        using Super = BasePass;
        struct ShaderInfo {
            SR_GTYPES_NS::Shader::Ptr pShader;

            struct MemInfo {
                Memory::UBOManager::VirtualUBO virtualUBO;
                DescriptorManager::VirtualDescriptorSet virtualDescriptor;
            };

            uint32_t uboUsed = 0;
            std::vector<MemInfo> UBOs;
            std::vector<std::vector<DebugRenderer::DrawInfo>> drawQueues;
        };
    protected:
        bool Load(const SR_XML_NS::Node& passNode) override;

        void Prepare() override;
        bool Render() override;
        void Update() override;

        bool Init() override;
        void DeInit() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;

        void BuildQueues();
        void DrawQueue(Pipeline& pipeline, const std::vector<DebugRenderer::DrawInfo>& queue, ShaderInfo& shaderInfo, uint32_t indicesCount);
        void UpdateUBO(ShaderInfo& shaderInfo, DebugRenderer::DrawType type);

    private:
        std::pair<uint32_t, uint32_t> m_linesCountCache;
        std::vector<std::pair<uint32_t, uint32_t>> m_meshesCountCache;
        std::map<SR_UTILS_NS::StringAtom, ShaderInfo> m_shaders;
        bool m_hasRendered = false;
        bool m_isNeedUpdate = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_DEBUG_PASS_H
