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
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/FileSystem/PathDataAccessor.h>

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
        , id(SR_UTILS_NS::Exchange(other.id, { }))
        , fboName(SR_UTILS_NS::Exchange(other.fboName, { }))
        , pTexture(SR_UTILS_NS::Exchange(other.pTexture, { }))
        , index(SR_UTILS_NS::Exchange(other.index, { }))
        , global(SR_UTILS_NS::Exchange(other.global, { }))
        , usageType(SR_UTILS_NS::Exchange(other.usageType, { }))
        , texturePath(SR_UTILS_NS::Exchange(other.texturePath, { }))
    { }

    SamplerData::SamplerData(const SamplerData& other) {
        textureId = other.textureId;
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
        global = other.global;
        usageType = other.usageType;
        texturePath = other.texturePath;
    }

    SamplerData& SamplerData::operator=(SamplerData&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        textureId = SR_UTILS_NS::Exchange(other.textureId, { });
        id = SR_UTILS_NS::Exchange(other.id, { });
        fboName = SR_UTILS_NS::Exchange(other.fboName, { });
        pTexture = SR_UTILS_NS::Exchange(other.pTexture, { });
        index = SR_UTILS_NS::Exchange(other.index, { });
        global = SR_UTILS_NS::Exchange(other.global, { });
        usageType = SR_UTILS_NS::Exchange(other.usageType, { });
        texturePath = SR_UTILS_NS::Exchange(other.texturePath, { });

        return *this;
    }

    SamplerData& SamplerData::operator=(const SamplerData& other) {
        if (this == &other) {
            return *this;
        }

        textureId = other.textureId;
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
        global = other.global;
        usageType = other.usageType;
        texturePath = other.texturePath;

        return *this;
    }

    SR_NODISCARD uint32_t SamplerData::GetTextureId(uint8_t frame) const noexcept {
        if (textureId.empty()) {
            return SR_ID_INVALID;
        }
        return textureId[SR_MIN(frame, static_cast<uint8_t>(textureId.size() - 1))];
    }

    void SamplerData::OnPostLoad() {
        if (pTexture) {
            pTexture->RemoveUsePoint();
            pTexture = nullptr;
        }

        if (IsTextureUsage() && !texturePath.empty()) {
            pTexture = SR_GTYPES_NS::Texture::Load(texturePath);
            if (pTexture) {
                pTexture->AddUsePoint();
            }
            else {
                SR_ERROR("SamplerData::OnPostLoad() : failed to load texture!\n\tPath: " + texturePath.ToString());
            }
        }
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

            const uint32_t textureId = sampler.GetTextureId(m_pTechnique->GetPipeline()->GetCurrentFrameIndex());
            if (textureId == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            pShader->SetSampler2D(sampler.id, static_cast<int32_t>(textureId));
        }
    }

    bool SamplersPassData::PrepareSamplers() {
        SR_TRACY_ZONE;

        if (!m_dirtySamplers) {
            return true;
        }

        m_dirtySamplers = false;

        for (auto&& sampler : m_samplers) {
            std::vector<uint32_t> textureIds;
            textureIds.resize(m_pTechnique->GetPipeline()->GetSwapchainImagesCount());

            if (sampler.IsFrameBufferUsage() && !sampler.fboName.Empty()) {
                auto&& pFrameBufferController = m_pTechnique->GetFrameBufferController(sampler.fboName);
                if (pFrameBufferController) {
                    auto&& pFBO = pFrameBufferController->GetFramebuffer();

                    if (!pFBO->Update()) {
                        SR_ERROR("ISamplersPass::PrepareSamplers() : failed to update frame buffer!\n\tName: " + sampler.fboName.ToStringRef());
                        m_dirtySamplers = true;
                        return false;
                    }

                    if (pFBO->GetId() != SR_ID_INVALID) {
                        uint32_t frame = 0;
                        for (uint32_t& textureId : textureIds) {
                            if (sampler.IsFrameBufferDepthUsage()) {
                                textureId = pFBO->GetDepthTexture(-1, frame);
                            }
                            else {
                                textureId = pFBO->GetColorTexture(sampler.index, frame);
                            }

                            if (textureId == SR_ID_INVALID) {
                                m_dirtySamplers = true;
                            }

                            ++frame;
                        }
                    }
                    else {
                        m_dirtySamplers = true;
                    }
                }
            }

            if (!sampler.IsFrameBufferDepthUsage()) {
                for (uint32_t& textureId : textureIds) {
                    if (textureId == SR_ID_INVALID) {
                        textureId = m_pTechnique->GetRenderContext()->GetDefaultTexture()->GetId();
                    }
                }
            }

            sampler.textureId = textureIds;
        }
        return true;
    }
}