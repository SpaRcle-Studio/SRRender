//
// Created by Monika on 16.11.2023.
//

#include <Graphics/Pass/IColorBufferPass.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SR_MATH_NS::FColor IColorBufferPass::GetColor(float_t x, float_t y) const {
        SR_TRACY_ZONE;

        if (x < 0 || x > 1 || y < 0 || y > 1) {
            return SR_MATH_NS::FColor(0.f);
        }

        auto&& pColorFrameBuffer = GetColorFrameBuffer();
        if (!pColorFrameBuffer) {
            return SR_MATH_NS::FColor(0.f);
        }

        const auto xPos = static_cast<uint32_t>(static_cast<float_t>(pColorFrameBuffer->GetWidth()) * x);
        const auto yPos = static_cast<uint32_t>(static_cast<float_t>(pColorFrameBuffer->GetHeight()) * y);
        if (pColorFrameBuffer->GetSize().x <= xPos || pColorFrameBuffer->GetSize().y <= yPos) {
            return SR_MATH_NS::FColor(0.f);
        }

        auto&& textureId = pColorFrameBuffer->GetColorTexture(0);
        return pColorFrameBuffer->GetPipeline()->GetPixelColor(textureId, xPos, yPos);
    }

    SR_GTYPES_NS::Mesh *IColorBufferPass::GetMesh(float_t x, float_t y) const {
        SR_TRACY_ZONE;

        auto&& color = GetColor(x, y);

        auto&& colorIndex = SR_MATH_NS::BGRToHEX(SR_MATH_NS::IVector3(
            static_cast<int32_t>(color.x),
            static_cast<int32_t>(color.y),
            static_cast<int32_t>(color.z)
        )) / m_multiplier;

        if (colorIndex > m_table.size() || colorIndex == 0) {
            return nullptr;
        }

        return m_table[colorIndex - 1];
    }

    uint32_t IColorBufferPass::GetIndex(float_t x, float_t y) const {
        auto&& color = GetColor(x, y);

        auto&& colorIndex = SR_MATH_NS::BGRToHEX(SR_MATH_NS::IVector3(
            static_cast<int32_t>(color.x),
            static_cast<int32_t>(color.y),
            static_cast<int32_t>(color.z)
        ));

        return colorIndex / m_multiplier;
    }

    void IColorBufferPass::SetMeshIndex(SR_GTYPES_NS::Mesh* pMesh) {
        SR_TRACY_ZONE;

        uint64_t colorIndex = m_colorId / m_multiplier;
        m_meshToColorIndex[pMesh] = m_colorId;

        if (colorIndex - 1 >= m_table.size()) {
            if (m_table.empty()) {
                m_table.resize(32);
            }
            else {
                m_table.resize(m_table.size() * 2);
            }
        }

        m_table[colorIndex - 1] = pMesh;
    }

    SR_MATH_NS::FVector3 IColorBufferPass::GetMeshColor() const noexcept {
        return SR_MATH_NS::HEXToBGR(GetColorIndex()).Cast<SR_MATH_NS::Unit>() / 255.f;
    }

    SR_MATH_NS::FVector3 IColorBufferPass::GetMeshColor(SR_GTYPES_NS::Mesh* pMesh) const noexcept {
        SR_TRACY_ZONE;
        auto&& pIt = m_meshToColorIndex.find(pMesh);
        if (pIt == m_meshToColorIndex.end()) {
            return SR_MATH_NS::FVector3(0.f);
        }
        return SR_MATH_NS::HEXToBGR(pIt->second).Cast<SR_MATH_NS::Unit>() / 255.f;
    }

    SR_GTYPES_NS::Mesh* IColorBufferPass::GetMesh(SR_MATH_NS::FVector2 pos) const {
        return GetMesh(pos.x, pos.y);
    }

    void IColorBufferPass::ClearTable() {
        SR_TRACY_ZONE;

        memset(m_table.data(), 0, m_table.size() * sizeof(SR_GTYPES_NS::Mesh*));
        m_meshToColorIndex.clear();
    }

    void IColorBufferPass::IncrementColorIndex() noexcept {
        m_colorId += m_multiplier;
    }

    uint32_t IColorBufferPass::GetColorIndex() const noexcept {
        return m_colorId;
    }
}