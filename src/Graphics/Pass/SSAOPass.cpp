//
// Created by Monika on 09.02.2023.
//

#include <Graphics/Pass/SSAOPass.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Shader.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(SSAOPass);

    bool SSAOPass::Init() {
        SR_TRACY_ZONE;

        m_kernel = CreateKernel();

        if ((m_noise = CreateNoise())) {
            m_noise->AddUsePoint();
        }

        if (!m_noise || m_noise->GetId() == SR_ID_INVALID) {
            SR_ERROR("SSAOPass::Init() : failed to initialize SSAO pass!");
            return false;
        }

        return PostProcessPass::Init();
    }

    void SSAOPass::DeInit() {
        if (m_noise) {
            m_noise->RemoveUsePoint();
            m_noise = nullptr;
        }

        PostProcessPass::DeInit();
    }

    SSAOPass::SSAOKernel SSAOPass::CreateKernel() const {
        std::vector<SR_MATH_NS::Vector4<float_t>> kernel;
        kernel.resize(64);

        for (uint8_t i = 0; i < kernel.size(); ++i)
        {
            SR_MATH_NS::Vector4<float_t> sample(
                    SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                    SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                    SR_UTILS_NS::Random::Instance().Float(0.0, 1.0),
                    0.f
            );

            sample = sample.Normalize() * SR_UTILS_NS::Random::Instance().Float(0.0, 1.0);

            float_t scale = float_t(i) / static_cast<float_t>(kernel.size());
            scale = SR_MATH_NS::Lerp(0.1, 1.0, scale * scale);

            kernel[i] = sample * scale;
        }

        return kernel;
    }

    SR_GTYPES_NS::Texture* SSAOPass::CreateNoise() const {
        std::vector<SR_MATH_NS::Vector3<float_t>> noise;
        noise.resize(16);

        for (uint8_t i = 0; i < noise.size(); ++i) {
            noise[i] = SR_MATH_NS::Vector3<float_t>(
                    SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                    SR_UTILS_NS::Random::Instance().Float(-1.0, 1.0),
                    0.0f
            );
        }

        auto&& config = Memory::TextureConfig();
        config.m_format = ImageFormat::R32_SFLOAT;
        config.m_filter = TextureFilter::NEAREST;

        return SR_GTYPES_NS::Texture::LoadRaw((uint8_t*)noise.data(), noise.size() * sizeof(float_t), 4, 4, config);
    }

    void SSAOPass::Update() {
        SR_TRACY_ZONE_N("SSAO update");

        if (m_shader) {
            m_shader->SetValue<false>(SHADER_SSAO_SAMPLES, m_kernel.data());
        }

        PostProcessPass::Update();
    }

    bool SSAOPass::Render() {
        SR_TRACY_ZONE_N("SSAO render");

        auto&& pFramebuffer = GetFramebuffer();
        if (!pFramebuffer) {
            return false;
        }

        if (!pFramebuffer->Bind()) {
            return false;
        }

        if (!pFramebuffer->BeginCmdBuffer(GetClearColors(), GetClearDepth())) {
            return false;
        }

        if (pFramebuffer->BeginRender()) {
            pFramebuffer->SetViewportScissor();
            PostProcessPass::Render();
            pFramebuffer->EndRender();
            pFramebuffer->EndCmdBuffer();
        }

        GetPassPipeline()->SetCurrentFrameBuffer(nullptr);

        /// Независимо от того, отрисовали мы что-то в кадровый буффер или нет,
        /// все равно возвращаем false (hasDrawData), так как технически, кадровый буффер
        /// не несет данных для рендера.
        return false;
    }

    void SSAOPass::UseSamplers() {
        if (m_shader && m_noise) {
            m_shader->SetSampler2D(SHADER_SSAO_NOISE, m_noise);
        }

        PostProcessPass::UseSamplers();
    }

    bool SSAOPass::Load(const SR_XML_NS::Node& passNode) {
        LoadFramebufferSettings(passNode.TryGetNode("FramebufferSettings"));
        return PostProcessPass::Load(passNode);
    }

    void SSAOPass::OnResize(const SR_MATH_NS::UVector2 &size) {
        PostProcessPass::OnResize(size);
    }

    std::vector<SR_GTYPES_NS::Framebuffer *> SSAOPass::GetFrameBuffers() const {
        if (!GetFramebuffer()) {
            return std::vector<SR_GTYPES_NS::Framebuffer*>();
        }
        return { GetFramebuffer() };
    }

    IRenderTechnique* SSAOPass::GetFrameBufferRenderTechnique() const {
        return GetTechnique();
    }
}