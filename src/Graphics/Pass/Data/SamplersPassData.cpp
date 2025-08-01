//
// Created by Monika on 07.05.2024.
//

#include <Graphics/Pass/Data/SamplersPassData.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/FrameBufferController.h>

#include <Codegen/SamplersPassData.generated.hpp>

namespace SR_GRAPH_NS {
    SamplerData::~SamplerData() {
        if (pTexture) {
            pTexture->RemoveUsePoint();
            pTexture = nullptr;
        }
    }

    SamplerData::SamplerData(SamplerData&& other) noexcept
        : textureId(SR_UTILS_NS::Exchange(other.textureId, { }))
        , fboId(SR_UTILS_NS::Exchange(other.fboId, { }))
        , id(SR_UTILS_NS::Exchange(other.id, { }))
        , fboName(SR_UTILS_NS::Exchange(other.fboName, { }))
        , pTexture(SR_UTILS_NS::Exchange(other.pTexture, { }))
        , index(SR_UTILS_NS::Exchange(other.index, { }))
        , depth(SR_UTILS_NS::Exchange(other.depth, { }))
        , texturePath(SR_UTILS_NS::Exchange(other.texturePath, { }))
    { }

    SamplerData::SamplerData(const SamplerData& other) {
        textureId = other.textureId;
        fboId = other.fboId;
        id = other.id;
        fboName = other.fboName;

        if (pTexture) {
            pTexture->RemoveUsePoint();
        }

        if (other.pTexture) {
            pTexture = other.pTexture;
            pTexture->AddUsePoint();
        }
        else {
            pTexture = nullptr;
        }

        index = other.index;
        depth = other.depth;
        texturePath = other.texturePath;
    }

    SamplerData& SamplerData::operator=(SamplerData&& other) noexcept {
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
        texturePath = SR_UTILS_NS::Exchange(other.texturePath, { });

        return *this;
    }

    SamplerData& SamplerData::operator=(const SamplerData& other) {
        if (this == &other) {
            return *this;
        }

        textureId = other.textureId;
        fboId = other.fboId;
        id = other.id;
        fboName = other.fboName;

        if (pTexture) {
            pTexture->RemoveUsePoint();
        }

        if (other.pTexture) {
            pTexture = other.pTexture;
            pTexture->AddUsePoint();
        }
        else {
            pTexture = nullptr;
        }

        index = other.index;
        depth = other.depth;
        texturePath = other.texturePath;

        return *this;
    }

    SamplersPassData::~SamplersPassData() {
        m_samplers.clear();
    }

    void SamplersPassData::UseSamplers(SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;

        for (auto&& sampler : m_samplers) {
            if (sampler.pTexture) {
                const uint32_t id = sampler.pTexture->GetId();
                if (id == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                    continue;
                }
                pShader->SetSampler2D(sampler.id, static_cast<int32_t>(id));
                continue;
            }

            if (sampler.textureId == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            pShader->SetSampler2D(sampler.id, static_cast<int32_t>(sampler.textureId));
        }
    }

    void SamplersPassData::PrepareSamplers() {
        SR_TRACY_ZONE;

        if (!m_dirtySamplers) {
            return;
        }

        m_dirtySamplers = false;

        for (auto&& sampler : m_samplers) {
            int32_t textureId = SR_ID_INVALID;

            sampler.fboId = SR_ID_INVALID;

            if (!sampler.fboName.Empty()) {
                auto&& pFrameBufferController = m_pTechnique->GetFrameBufferController(sampler.fboName);
                if (pFrameBufferController) {
                    auto&& pFBO = pFrameBufferController->GetFramebuffer();

                    if (!pFBO->Update()) {
                        SR_ERROR("ISamplersPass::PrepareSamplers() : failed to update frame buffer!\n\tName: " + sampler.fboName.ToStringRef());
                        continue;
                    }

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
                sampler.textureId = textureId;
            }
        }
    }

    /*void SamplersPassData::LoadSamplersPass(const SR_XML_NS::Node& passNode) {
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
    }*/
}