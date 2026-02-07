//
// Created by Monika on 16.11.2023.
//

#ifndef SR_ENGINE_GRAPHICS_I_COLOR_BUFFER_PASS_H
#define SR_ENGINE_GRAPHICS_I_COLOR_BUFFER_PASS_H

#include <Graphics/macros.h>

#include <Utils/Math/Vector4.h>
#include <Utils/Types/WeakPtr.h>

namespace SR_GTYPES_NS {
    class Shader;
    class Framebuffer;
    class Mesh;
}

namespace SR_GRAPH_NS {
    class Pipeline;
    class IColorBufferPass;

    class ColorBufferPassRequest final : public SR_HTYPES_NS::SharedPtr<ColorBufferPassRequest> {
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<ColorBufferPassRequest>;
        using WeakPtr = SR_HTYPES_NS::WeakPtr<ColorBufferPassRequest>;

    public:
        ColorBufferPassRequest(IColorBufferPass* pPass, Pipeline* pPipeline, uint64_t requestId, const SR_MATH_NS::FVector2& pos, const SR_MATH_NS::IVector2& imgSize);
        ~ColorBufferPassRequest() override;

        SR_NODISCARD bool IsReady() const;
        SR_NODISCARD SR_MATH_NS::FColor FetchColor(bool release);
        SR_NODISCARD SR_GTYPES_NS::Mesh* GetMesh(bool release);

        bool ChangeRequest(const SR_MATH_NS::FVector2& pos);

    private:
        IColorBufferPass* m_pPass = nullptr;
        Pipeline* m_pPipeline = nullptr;
        uint64_t m_requestId = 0;
        SR_MATH_NS::FVector2 m_pos;
        SR_MATH_NS::IVector2 m_imageSize;
        SR_MATH_NS::FColor m_cachedColor = SR_MATH_NS::FColor(0.f);
        bool m_isResultFetched = false;
        bool m_isReleased = false;

    };

    class IColorBufferPass {
    public:
        virtual ~IColorBufferPass();

        SR_NODISCARD virtual const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& GetColorFrameBuffer() const noexcept = 0;

        SR_NODISCARD ColorBufferPassRequest::Ptr CreateColorRequest(float_t x, float_t y);
        SR_NODISCARD ColorBufferPassRequest::Ptr CreateColorRequest(const SR_MATH_NS::FVector2& pos) { return CreateColorRequest(pos.x, pos.y); }

        SR_NODISCARD SR_GTYPES_NS::Mesh* GetMeshByColor(const SR_MATH_NS::FColor& color) const;

        SR_NODISCARD uint32_t GetColorIndex() const noexcept;
        SR_NODISCARD SR_MATH_NS::FVector3 GetMeshColor() const noexcept;
        SR_NODISCARD uint32_t GetColorMultiplier() const noexcept { return m_multiplier; }

        void SetColorMultiplier(uint32_t multiplier) { m_multiplier = SR_MAX(1, multiplier); }
        void OnRequestDestroyed(const ColorBufferPassRequest* pRequest);

        void DestroyRequests();

    protected:
        void ClearTable();
        void SetMeshIndex(SR_GTYPES_NS::Mesh* pMesh);
        void IncrementColorIndex() noexcept;
        void ResetColorIndex() noexcept { m_colorId = 0; }

    private:
        std::vector<SR_GTYPES_NS::Mesh*> m_table;
        uint32_t m_colorId = 0;
        uint32_t m_multiplier = 1;

        std::vector<ColorBufferPassRequest::WeakPtr> m_activeRequests;
        
    };
}

#endif //SR_ENGINE_GRAPHICS_I_COLOR_BUFFER_PASS_H
