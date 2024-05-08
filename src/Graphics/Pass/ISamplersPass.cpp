//
// Created by Monika on 07.05.2024.
//

#include <Graphics/Pass/ISamplersPass.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/FrameBufferController.h>

namespace SR_GRAPH_NS {
    ISamplersPass::Sampler::~Sampler() {
        if (pTexture) {
            pTexture->RemoveUsePoint();
            pTexture = nullptr;
        }
    }

    ISamplersPass::Sampler::Sampler(ISamplersPass::Sampler&& other) noexcept
        : textureId(SR_UTILS_NS::Exchange(other.textureId, { }))
        , fboId(SR_UTILS_NS::Exchange(other.fboId, { }))
        , id(SR_UTILS_NS::Exchange(other.id, { }))
        , fboName(SR_UTILS_NS::Exchange(other.fboName, { }))
        , pTexture(SR_UTILS_NS::Exchange(other.pTexture, { }))
        , index(SR_UTILS_NS::Exchange(other.index, { }))
        , depth(SR_UTILS_NS::Exchange(other.depth, { }))
    { }

    ISamplersPass::Sampler& ISamplersPass::Sampler::operator=(ISamplersPass::Sampler&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        textureId = SR_UTILS_NS::Exchange(other.textureId, { });
        fboId = SR_UTILS_NS::Exchange(other.fboId, { });
        id = SR_UTILS_NS::Exchange(other.id, { });
        fboName = SR_UTILS_NS::Exchange(other.fboName, { });
        pTexture = SR_UTILS_NS::Exchange(other.pTexture, { });
        index = SR_UTILS_NS::Exchange(other.index, { });
        depth = SR_UTILS_NS::Exchange(other.depth, { });

        return *this;
    }

    ISamplersPass::~ISamplersPass() {
        m_samplers.clear();
    }

    void ISamplersPass::UseSamplers(ShaderUseInfo info) {
        for (auto&& sampler : m_samplers) {
            if (sampler.pTexture) {
                const uint32_t id = sampler.pTexture->GetId();
                if (id == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                    continue;
                }
                info.pShader->SetSampler2D(sampler.id, static_cast<int32_t>(id));
                continue;
            }

            if (sampler.textureId == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            info.pShader->SetSampler2D(sampler.id, static_cast<int32_t>(sampler.textureId));
        }
    }

    void ISamplersPass::PrepareSamplers() {
        SR_TRACY_ZONE;

        if (!m_dirtySamplers) {
            return;
        }

        m_dirtySamplers = false;

        bool needUpdate = false;

        for (auto&& sampler : m_samplers) {
            int32_t textureId = SR_ID_INVALID;

            sampler.fboId = SR_ID_INVALID;

            if (!sampler.fboName.Empty()) {
                auto&& pFrameBufferController = m_pTechnique->GetFrameBufferController(sampler.fboName);
                if (pFrameBufferController) {
                    auto&& pFBO = pFrameBufferController->GetFramebuffer();

                    sampler.fboId = pFBO->GetId();

                    if (sampler.fboId != SR_ID_INVALID) {
                        if (sampler.depth) {
                            textureId = pFBO->GetDepthTexture();
                        }
                        else {
                            textureId = pFBO->GetColorTexture(sampler.index);
                        }

                        if (textureId == SR_ID_INVALID) {
                            m_dirtySamplers = true;
                        }
                    }
                    else {
                        m_dirtySamplers = true;
                    }
                }
            }

            if (textureId == SR_ID_INVALID && !sampler.depth) {
                textureId = m_pTechnique->GetRenderContext()->GetDefaultTexture()->GetId();
            }

            if (textureId != sampler.textureId) {
                needUpdate = true;
                sampler.textureId = textureId;
            }
        }

        if (needUpdate) {
            OnSamplersChanged();
        }
    }

    void ISamplersPass::LoadSamplersPass(const SR_XML_NS::Node& passNode) {
        m_samplers.clear();

        for (auto&& samplerNode : passNode.TryGetNodes("Sampler")) {
            Sampler sampler = Sampler();

            if (auto&& idNode = samplerNode.TryGetAttribute("Id")) {
                sampler.id = idNode.ToString();
            }
            else {
                continue;
            }

            if (auto&& textureNode = samplerNode.TryGetAttribute("Texture")) {
                auto&& pTexture = SR_GTYPES_NS::Texture::Load(textureNode.ToString());
                if (!pTexture) {
                    SR_ERROR("ISamplersPass::LoadSamplersPass() : failed to load texture!\n\tPath: " + textureNode.ToString());
                    continue;
                }
                pTexture->AddUsePoint();
                sampler.pTexture = pTexture;
            }
            else if (auto&& fboNameNode = samplerNode.TryGetAttribute("FBO")) {
                sampler.fboName = fboNameNode.ToString();

                auto&& pFrameBufferController = m_pTechnique->GetFrameBufferController(sampler.fboName);
                if (!pFrameBufferController) {
                    if (!samplerNode.TryGetAttribute("Optional").ToBool(false)) {
                        SR_ERROR("MeshDrawerPass::Load() : failed to find frame buffer controller!\n\tName: " + sampler.fboName.ToStringRef());
                    }
                    continue;
                }

                if (auto&& depthAttribute = samplerNode.TryGetAttribute("Depth")) {
                    sampler.depth = depthAttribute.ToBool();
                }

                if (!sampler.depth) {
                    sampler.index = samplerNode.TryGetAttribute("Index").ToUInt64(-1);
                }
            }

            m_samplers.emplace_back(std::move(sampler));
        }
    }
}