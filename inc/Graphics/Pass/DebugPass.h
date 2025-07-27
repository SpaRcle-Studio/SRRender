//
// Created by Monika on 19.09.2022.
//

#ifndef SR_ENGINE_GRAPHICS_DEBUG_PASS_H
#define SR_ENGINE_GRAPHICS_DEBUG_PASS_H

#include <Graphics/Pass/IMesh3DClusterPass.h>
#include <Graphics/Render/DebugRenderer.h>

namespace SR_GRAPH_NS {
    struct DebugPassShaderInfo : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        DebugPassShaderInfo() = default;
        ~DebugPassShaderInfo() override;

        void SetShader(const SR_UTILS_NS::Path& path);
        void LoadShader();

        struct MemInfo {
            Memory::UBOManager::VirtualUBO virtualUBO;
            DescriptorManager::VirtualDescriptorSet virtualDescriptor;
        };

        SR_GTYPES_NS::Shader::Ptr pShader;
        uint32_t uboUsed = 0;
        std::vector<MemInfo> UBOs;
        std::vector<std::vector<DebugRenderer::DrawInfo>> drawQueues;

        /// @property
        /// @customArgs(pick: enabled, filter name: Shader)
        /// @customArg(filter value: srsl)
        SR_UTILS_NS::Path shaderPath;
    };

    class DebugPass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
    protected:
        void Prepare() override;
        bool Render() override;
        void Update() override;

        bool Init() override;
        void DeInit() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;

        void BuildQueues();
        void DrawQueue(Pipeline& pipeline, const std::vector<DebugRenderer::DrawInfo>& queue, DebugPassShaderInfo& shaderInfo, uint32_t indicesCount);
        void UpdateUBO(DebugPassShaderInfo& shaderInfo, DebugRenderer::DrawType type);

    private:
        std::pair<uint32_t, uint32_t> m_linesCountCache;
        std::vector<std::pair<uint32_t, uint32_t>> m_meshesCountCache;
        bool m_hasRendered = false;
        bool m_isNeedUpdate = false;
        bool m_isValid = false;

        /// @property
        std::map<SR_UTILS_NS::StringAtom, DebugPassShaderInfo> m_shaders;

    };
}

#endif //SR_ENGINE_GRAPHICS_DEBUG_PASS_H
