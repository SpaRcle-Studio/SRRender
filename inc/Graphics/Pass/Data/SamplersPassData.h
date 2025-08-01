//
// Created by Monika on 07.05.2024.
//

#ifndef SR_ENGINE_GRAPHICS_I_SAMPLERS_PASS_H
#define SR_ENGINE_GRAPHICS_I_SAMPLERS_PASS_H

#include <Utils/Types/StringAtom.h>

#include <Graphics/Types/Texture.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GRAPH_NS {
    class IRenderTechnique;

    struct SamplerData : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        SamplerData() = default;
        ~SamplerData() override;

        SamplerData(SamplerData&& other) noexcept;
        SamplerData(const SamplerData& other);
        SamplerData& operator=(SamplerData&& other) noexcept;
        SamplerData& operator=(const SamplerData& other);

        uint32_t textureId = SR_ID_INVALID;
        uint32_t fboId = SR_ID_INVALID;
        SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Texture> pTexture;

        /// @property
        SR_UTILS_NS::StringAtom id;
        /// @property
        SR_UTILS_NS::StringAtom fboName;
        /// @property
        uint64_t index = 0;
        /// @property
        bool depth = false;
        /// @property
        SR_UTILS_NS::Path texturePath;
    };

    class SamplersPassData final : public SR_UTILS_NS::Serializable {
        SR_CLASS()
    public:
        ~SamplersPassData() override;

    public:
        //void LoadSamplersPass(const SR_XML_NS::Node& passNode);

        void UseSamplers(SR_GTYPES_NS::Shader* pShader);

        SR_NODISCARD bool HasSamplers() const noexcept { return !m_samplers.empty(); }
        SR_NODISCARD bool IsSamplersDirty() const noexcept { return m_dirtySamplers; }

        void MarkSamplersDirty() { m_dirtySamplers = true; }
        void PrepareSamplers();
        void SetRenderTechnique(IRenderTechnique* pTechnique) { m_pTechnique = pTechnique; }

    private:
        /// @property
        std::vector<SamplerData> m_samplers;

    private:
        bool m_dirtySamplers = true;
        IRenderTechnique* m_pTechnique = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_I_SAMPLERS_PASS_H
