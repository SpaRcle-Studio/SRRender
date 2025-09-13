//
// Created by Monika on 06.05.2022.
//

#ifndef SR_ENGINE_FRAMEBUFFER_H
#define SR_ENGINE_FRAMEBUFFER_H

#include <Utils/Debug.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Resources/IResource.h>

#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Pipeline/FrameBufferFeatures.h>

namespace SR_GRAPH_NS {
    class Pipeline;
}

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GTYPES_NS {
    class RenderTexture;

    /**
     * \Usage Bing -> BeginRenderBuffer -> BeginRender -> EndRender -> EndRenderBuffer
     * */
    class Framebuffer : public SR_UTILS_NS::IResource, public Memory::IGraphicsResource {
        SR_CLASS()
        using Super = SR_UTILS_NS::IResource;
    public:
        using ClearColors = std::vector<SR_MATH_NS::FColor>;
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<Pipeline>;
        using Ptr = SR_HTYPES_NS::SharedPtr<Framebuffer>;

    public:
        Framebuffer();
        ~Framebuffer() override;

    public:
        static Ptr Create(uint32_t images, const SR_MATH_NS::IVector2& size);
        static Ptr Create(const std::vector<ImageFormat>& colors, ImageFormat depth);
        static Ptr Create(const std::vector<ImageFormat>& colors, ImageFormat depth, const SR_MATH_NS::IVector2& size);
        static Ptr Create(const std::vector<ImageFormat>& colors, ImageFormat depth, const SR_MATH_NS::IVector2& size, uint8_t samples);
        static Ptr Create(const std::vector<ImageFormat>& colors, ImageFormat depth, const SR_MATH_NS::IVector2& size, uint8_t samples, uint32_t layersCount);
        static Ptr Create(const std::vector<ImageFormat>& colors, ImageFormat depth, const SR_MATH_NS::IVector2& size, uint8_t samples, uint32_t layersCount, ImageAspect depthAspect);

    public:
        bool Update();
        bool Bind();

        bool BeginCmdBuffer(uint32_t frame);
        bool BeginCmdBuffer(uint32_t frame, const ClearColors& clearColors, std::optional<float_t> depth);
        bool BeginCmdBuffer(uint32_t frame, const SR_MATH_NS::FColor& clearColor, float_t depth);

        void SetViewportScissor();
        bool BeginRender();

        void EndCmdBuffer();
        void EndRender();

        void SetDirty();
        void SetSize(const SR_MATH_NS::IVector2& size);
        void SetDepthEnabled(bool depthEnabled);
        void SetSampleCount(uint8_t samples);
        void SetLayersCount(uint32_t layersCount);
        void SetArrayLayersCount(uint32_t arrayLayersCount);
        void SetDepthAspect(ImageAspect depthAspect);
        void SetFeatures(const FrameBufferFeatures& features);
        void SetName(SR_UTILS_NS::StringAtom name) { m_name = name; }
        void SetInstanceForEachFram(bool forEach) { m_forEachSwapchainImage = forEach; }
        void SetRenderAsSingleLayer(bool renderAsSingleLayer) { m_renderAsSingleLayer = renderAsSingleLayer; }

        SR_NODISCARD bool IsFileResource() const noexcept override { return false; }
        SR_NODISCARD bool IsAllowedMultiInstance() const override { return true; }
        SR_NODISCARD uint8_t GetSamplesCount() const;
        SR_NODISCARD uint32_t GetColorLayersCount() const noexcept { return m_colors.size(); }
        SR_NODISCARD uint32_t GetLayersCount() const noexcept { return m_layersCount; }
        SR_NODISCARD uint32_t GetArrayLayersCount() const noexcept { return m_arrayLayersCount; }
        SR_NODISCARD ImageAspect GetDepthAspect() const noexcept { return m_depth.aspect; }
        SR_NODISCARD bool IsDepthEnabled() const { return m_depthEnabled; }
        SR_NODISCARD bool IsDirty() const { return m_dirty; }
        SR_NODISCARD bool IsValid() const;
        SR_NODISCARD bool IsWasRendered() const { return m_wasRendered; }
        SR_NODISCARD bool IsForEachSwapchainImage() const { return m_forEachSwapchainImage; }
        SR_NODISCARD bool IsRenderAsSingleLayer() const { return m_renderAsSingleLayer; }
        SR_NODISCARD const FrameBufferFeatures& GetFeatures() const { return m_features; }

        SR_NODISCARD int32_t GetId() const;
        SR_NODISCARD int32_t GetColorTexture(uint32_t layer, uint8_t frame);
        SR_NODISCARD int32_t GetDepthTexture(int32_t layer, uint8_t frame);

        SR_NODISCARD SR_UTILS_NS::StringAtom GetName() const { return m_name; }
        SR_NODISCARD uint32_t GetWidth() const;
        SR_NODISCARD uint32_t GetHeight() const;
        SR_NODISCARD SR_MATH_NS::IVector2 GetSize() const { return m_size; }

        void FreeVMemory() override;
        uint64_t GetFileHash() const override;

    private:
        FrameBufferFeatures m_features;

        SR_UTILS_NS::StringAtom m_name;
        std::atomic<bool> m_dirty = true;
        std::atomic<bool> m_hasErrors = false;
        bool m_wasRendered = false;
        bool m_renderAsSingleLayer = false;

        std::vector<ColorLayer> m_colors = { };
        DepthLayer m_depth = { };
        std::vector<int32_t> m_frameBuffer;

        SR_MATH_NS::IVector2 m_size = { };

        uint8_t m_layersCount = 1;
        uint32_t m_arrayLayersCount = 1;

        bool m_forEachSwapchainImage = false;

        uint8_t m_sampleCount = 0;
        uint8_t m_currentSampleCount = 0;
        bool m_depthEnabled = true;

    };
}

#endif //SR_ENGINE_FRAMEBUFFER_H
