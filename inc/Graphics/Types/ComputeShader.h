//
// Created by Monika on 29.06.2025.
//

#ifndef SR_ENGINE_GRAPHICS_TYPES_COMPUTE_SHADER_H
#define SR_ENGINE_GRAPHICS_TYPES_COMPUTE_SHADER_H

#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GTYPES_NS {
    class SR_RENDERER_DLL_API ComputeShader final : public SR_UTILS_NS::NonCopyable {
    public:
        using Ptr = std::unique_ptr<ComputeShader>;

    private:
        ComputeShader();

    public:
        ~ComputeShader() override;

    public:
        SR_NODISCARD static ComputeShader::Ptr Load(const SR_UTILS_NS::Path& path);

    public:
        SR_NODISCARD const SR_GTYPES_NS::Shader::Ptr& GetShader() const noexcept;
        SR_NODISCARD const SR_GRAPH_NS::Pipeline::Ptr& GetPipeline() const;

        bool BeginCompute();
        void Dispatch(uint32_t x, uint32_t y, uint32_t z);
        void Dispatch();
        void EndCompute();

    private:
        SR_GTYPES_NS::Shader::Ptr m_pShader = nullptr;
        mutable SR_GRAPH_NS::Pipeline::Ptr m_pipeline = nullptr;
        int32_t m_descriptorSet = SR_ID_INVALID;
        bool m_isComputeState = false;
        bool m_isDispatched = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_TYPES_COMPUTE_SHADER_H
