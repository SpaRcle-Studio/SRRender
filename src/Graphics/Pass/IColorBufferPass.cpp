//
// Created by Monika on 16.11.2023.
//

#include <Graphics/Pass/IColorBufferPass.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    ColorBufferPassRequest::ColorBufferPassRequest(IColorBufferPass* pPass, Pipeline* pPipeline, uint64_t requestId, const SR_MATH_NS::FVector2& pos, const SR_MATH_NS::IVector2& imgSize)
        : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_pPass(pPass)
        , m_pPipeline(pPipeline)
        , m_requestId(requestId)
        , m_pos(pos)
        , m_imageSize(imgSize)
    { }

    ColorBufferPassRequest::~ColorBufferPassRequest() {
        SR_TRACY_ZONE;

        if (m_requestId != SR_ID_INVALID) {
            m_pPipeline->ReleasePixelRangeRequest(m_requestId);
        }

        if (SRVerify(m_pPass)) {
            m_pPass->OnRequestDestroyed(this);
        }
    }

    bool ColorBufferPassRequest::IsReady() const {
        SR_TRACY_ZONE;
        if (m_isResultFetched) {
            return true;
        }
        return m_pPipeline && m_requestId != SR_ID_INVALID && m_pPipeline->IsPixelRangeReady(m_requestId);
    }

    SR_MATH_NS::FColor ColorBufferPassRequest::FetchColor(bool release) {
        SR_TRACY_ZONE;

        if (m_isResultFetched) {
            return m_cachedColor;
        }

        SR_MATH_NS::FColor color;
        if (m_pPipeline->GetPixelRangeResult(m_requestId, &color, 1, 1)) {
            m_cachedColor = color;
            m_isResultFetched = true;

            if (release) {
                // Освобождаем ресурсы запроса
                m_pPipeline->ReleasePixelRangeRequest(m_requestId);
                m_requestId = SR_ID_INVALID;
            }

            return m_cachedColor;
        }

        return SR_MATH_NS::FColor();
    }

    SR_GTYPES_NS::Mesh* ColorBufferPassRequest::GetMesh(bool release) {
        SR_TRACY_ZONE;
        return m_pPass ? m_pPass->GetMeshByColor(FetchColor(release)) : nullptr;
    }

    bool ColorBufferPassRequest::ChangeRequest(const SR_MATH_NS::FVector2& pos) {
        SR_TRACY_ZONE;

        if (!m_pPipeline || m_requestId == SR_ID_INVALID || !m_pPass) {
            return false;
        }

        auto&& pColorFrameBuffer = m_pPass->GetColorFrameBuffer();
        if (!pColorFrameBuffer || pos.x < 0 || pos.x > 1 || pos.y < 0 || pos.y > 1) {
            return false;
        }

        const auto xPos = static_cast<uint32_t>(static_cast<float_t>(m_imageSize.x) * pos.x);
        const auto yPos = static_cast<uint32_t>(static_cast<float_t>(m_imageSize.y) * pos.y);
        if (m_imageSize.x <= xPos || m_imageSize.y <= yPos) {
            return false;
        }

        uint64_t workId = m_pPipeline->RequestPixelRange(m_requestId, pColorFrameBuffer->GetColorTexture(0, m_pPipeline->GetCurrentImageIndex()), xPos, yPos, 1, 1);
        if (workId == SR_ID_INVALID) {
            return false;
        }

        m_requestId = workId;
        m_pos = SR_MATH_NS::FVector2(static_cast<float_t>(xPos), static_cast<float_t>(yPos));
        m_isResultFetched = false;
        m_cachedColor = SR_MATH_NS::FColor(0.f);

        return true;
    }

    IColorBufferPass::~IColorBufferPass() {
        DestroyRequests();
    }

    void IColorBufferPass::SetMeshIndex(SR_GTYPES_NS::Mesh* pMesh) {
        SR_TRACY_ZONE;

        uint64_t colorIndex = m_colorId / m_multiplier;

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

    void IColorBufferPass::ClearTable() {
        SR_TRACY_ZONE;

        memset(m_table.data(), 0, m_table.size() * sizeof(SR_GTYPES_NS::Mesh*));
    }

    void IColorBufferPass::IncrementColorIndex() noexcept {
        m_colorId += m_multiplier;
    }

    uint32_t IColorBufferPass::GetColorIndex() const noexcept {
        return m_colorId;
    }

    ColorBufferPassRequest::Ptr IColorBufferPass::CreateColorRequest(float_t x, float_t y) {
        SR_TRACY_ZONE;

        auto&& pColorFrameBuffer = GetColorFrameBuffer();
        if (!pColorFrameBuffer || x < 0 || x > 1 || y < 0 || y > 1) {
            return nullptr;
        }

        const auto xPos = static_cast<uint32_t>(static_cast<float_t>(pColorFrameBuffer->GetWidth()) * x);
        const auto yPos = static_cast<uint32_t>(static_cast<float_t>(pColorFrameBuffer->GetHeight()) * y);
        if (pColorFrameBuffer->GetSize().x <= xPos || pColorFrameBuffer->GetSize().y <= yPos) {
            return nullptr;
        }

        // Используем только асинхронный подход
        auto&& pPipeline = pColorFrameBuffer->GetPipeline();
        if (!pPipeline) {
            return nullptr;
        }

        auto&& textureId = pColorFrameBuffer->GetColorTexture(0, pPipeline->GetCurrentImageIndex());

        uint64_t workId = pPipeline->RequestPixelRange(SR_ID_INVALID, textureId, xPos, yPos, 1, 1);
        if (workId == SR_ID_INVALID) {
            return nullptr;
        }

        auto&& pRequest = ColorBufferPassRequest::MakeShared(
            const_cast<IColorBufferPass*>(this),
            pPipeline.Get(),
            workId,
            SR_MATH_NS::FVector2(static_cast<float_t>(xPos), static_cast<float_t>(yPos)),
            pColorFrameBuffer->GetSize()
        );

        m_activeRequests.emplace_back(pRequest);
        return pRequest;
    }

    void IColorBufferPass::OnRequestDestroyed(const ColorBufferPassRequest* pRequest) {
        SR_TRACY_ZONE;

        auto pIt = std::ranges::find_if(m_activeRequests, [pRequest](const ColorBufferPassRequest::WeakPtr& pWeak) {
            return pWeak.GetUnchecked() == pRequest;
        });

        if (SRVerify(pIt != m_activeRequests.end())) {
            m_activeRequests.erase(pIt);
        }
    }

    SR_GTYPES_NS::Mesh* IColorBufferPass::GetMeshByColor(const SR_MATH_NS::FColor& color) const {
        SR_TRACY_ZONE;

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

    void IColorBufferPass::DestroyRequests() {
        if (m_activeRequests.empty()) {
            return;
        }

        SR_TRACY_ZONE;

        while (!m_activeRequests.empty()) {
            if (auto&& pStrong = m_activeRequests.back().Lock()) {
                pStrong.AutoFree();
            }
            else {
                m_activeRequests.pop_back();
            }
        }
    }
}