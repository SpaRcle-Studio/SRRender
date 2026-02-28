//
// AutoExposurePass: GPU auto-exposure for HDR (log luminance reduction + temporal adaptation).
// Integrates after HDR render, before tone mapping. Output: float exposure in SSBO.
//

#ifndef SR_ENGINE_GRAPHICS_AUTO_EXPOSURE_PASS_H
#define SR_ENGINE_GRAPHICS_AUTO_EXPOSURE_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Pass/Data/SamplersPassData.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class AutoExposurePass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AutoExposurePass>;

    public:
        bool Init() override;
        void DeInit() override;

        bool Prepare() override;
        bool Render() override;
        void Update() override;
        void OnMultisampleChanged() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void SetRenderTechnique(IRenderTechnique* pRenderTechnique) override;

        SR_NODISCARD SamplersPassData& GetSamplersData() { return m_samplers; }
        SR_NODISCARD const SamplersPassData& GetSamplersData() const { return m_samplers; }

        void SetSpeed(float speed) { m_speed = speed; }
        SR_NODISCARD float GetSpeed() const { return m_speed; }

        void SetKeyValue(float keyValue) { m_keyValue = keyValue; }
        SR_NODISCARD float GetKeyValue() const { return m_keyValue; }

    public:
        void UseSamplers(SR_GTYPES_NS::Shader& shader) override;
        void UseSSBOFromAnotherPass(SR_GTYPES_NS::Shader& shader) override;

    private:
        /// Buffer creation: allocation sizes and SSBO ids.
        bool AllocateBuffers(uint32_t width, uint32_t height);
        void FreeBuffers();

        bool ReductionFirst(uint32_t groupsX, uint32_t groupsY);
        int32_t ReductionLinear();
        bool Adaptation(int32_t luminanceSSBO);

        bool DispatchReductionAndAdaptation(uint32_t width, uint32_t height);

    private:
        /// @property
        SamplersPassData m_samplers;
        /// @property
        float m_speed = 2.0f;
        /// @property
        float m_keyValue = 0.18f;

    private:
        struct UBOInfo {
            uint32_t elementCount = 0u;
            int32_t uboId = SR_ID_INVALID;
            int32_t descriptorSetId = SR_ID_INVALID;
        };

        SR_MATH_NS::FVector2 m_resolution;
        bool m_multiFrameSSBOResources = false;
        uint64_t m_totalPixelCount = 0u;

        ShaderPtr m_pReductionShader;
        ShaderPtr m_pReductionLinearShader;
        ShaderPtr m_pAdaptationShader;

        std::vector<int32_t> m_exposureSSBO;
        std::vector<int32_t> m_reductionSSBOA;
        std::vector<int32_t> m_reductionSSBOB;

        UBOInfo m_reductionUBOFirst;
        std::vector<UBOInfo> m_reductionUBOLinear;
        UBOInfo m_adaptationUBO;

        uint32_t m_reductionBufferElementCount = 0u;
        uint32_t m_width = 0u;
        uint32_t m_height = 0u;

        bool m_dirtyShader = true;

        uint64_t m_bufferSize = 0;
        std::string m_emptyData;

        static constexpr uint32_t REDUCTION_LINEAR_GROUP_SIZE = 256u;
        static constexpr uint32_t REDUCTION_FIRST_GROUP_X = 16u;
        static constexpr uint32_t REDUCTION_FIRST_GROUP_Y = 16u;
    };
}

#endif // SR_ENGINE_GRAPHICS_AUTO_EXPOSURE_PASS_H
