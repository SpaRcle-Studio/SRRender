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
    class RenderContext;

    class ISamplersPass {
    private:
        struct Sampler : public SR_UTILS_NS::NonCopyable {
            Sampler() = default;
            ~Sampler() override;

            Sampler(Sampler&& other) noexcept;
            Sampler& operator=(Sampler&& other) noexcept;

            uint32_t textureId = SR_ID_INVALID;
            uint32_t fboId = SR_ID_INVALID;
            SR_UTILS_NS::StringAtom id;
            SR_UTILS_NS::StringAtom fboName;
            SR_GTYPES_NS::Texture* pTexture = nullptr;
            uint64_t index = 0;
            bool depth = false;
        };
        using Samplers = std::vector<Sampler>;

    public:
        virtual ~ISamplersPass();

    public:
        void LoadSamplersPass(const SR_XML_NS::Node& passNode);

        virtual void UseSamplers(ShaderUseInfo info);

        SR_NODISCARD bool HasSamplers() const noexcept { return !m_samplers.empty(); }
        SR_NODISCARD bool IsSamplersDirty() const noexcept { return m_dirtySamplers; }

    protected:
        virtual void OnSamplersChanged() { }
        void MarkSamplersDirty() { m_dirtySamplers = true; }
        void PrepareSamplers();
        void SetISamplerRenderTechnique(IRenderTechnique* pTechnique) { m_pTechnique = pTechnique; }

    private:
        bool m_dirtySamplers = true;
        Samplers m_samplers;

        IRenderTechnique* m_pTechnique = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_I_SAMPLERS_PASS_H
