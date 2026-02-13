//
// Created by Monika on 04.02.2024.
//

#ifndef SR_ENGINE_FRAME_BUFFER_CONTROLLER_H
#define SR_ENGINE_FRAME_BUFFER_CONTROLLER_H

#include <Graphics/Pipeline/FrameBufferFeatures.h>
#include <Graphics/Pipeline/TextureHelper.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Types/SharedPtr.h>
#include <Utils/Serialization/Serializable.h>

namespace SR_GTYPES_NS {
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class RenderContext;

    class FrameBufferController final : public SR_HTYPES_NS::SharedPtr<FrameBufferController>, public SR_UTILS_NS::Serializable {
        SR_CLASS()
        using Super = SR_HTYPES_NS::SharedPtr<FrameBufferController>;
        using ColorFormats = std::vector<ImageFormat>;
        using ClearColors = std::vector<SR_MATH_NS::FColor>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FrameBufferController>;

    public:
        FrameBufferController();
        ~FrameBufferController() override;

    public:
        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& GetFramebuffer() const noexcept { return m_framebuffer; }
        SR_NODISCARD SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& GetFramebuffer() noexcept { return m_framebuffer; }
        SR_NODISCARD uint8_t GetLayersCount() const noexcept { return m_layersCount; }
        SR_NODISCARD SR_UTILS_NS::StringAtom GetName() const noexcept { return m_name; }
        SR_NODISCARD FrameBufferFeatures& GetFeatures() noexcept { return m_features; }

        bool InitializeFramebuffer(RenderContext* pContext);

        void SetName(const SR_UTILS_NS::StringAtom& name) { m_name = name; }
        void SetDynamicResizing(bool enabled) { m_dynamicResizing = enabled; }
        void SetDepthEnabled(bool enabled) { m_depthEnabled = enabled; }
        void SetInstanceForEachFrame(bool enabled) { m_instanceForEachFrame = enabled; }
        void SetPreScale(const SR_MATH_NS::FVector2& preScale) { m_preScale = preScale; }
        void SetSize(const SR_MATH_NS::IVector2& size) { m_size = size; }
        void SetColorFormats(const ColorFormats& colorFormats) { m_colorFormats = colorFormats; }
        void SetFeatures(const FrameBufferFeatures& features) { m_features = features; }
        void SetSamples(uint8_t samples) { m_samples = samples; }
        void SetDepthFormat(ImageFormat format) { m_depthFormat = format; }
        void SetDepthAspect(ImageAspect aspect) { m_depthAspect = aspect; }

        void OnResize(const SR_MATH_NS::UVector2& size);

        void SetLayersCount(uint32_t count);
        void SetArrayLayersCount(uint32_t count);

    private:
        SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer> m_framebuffer;

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_name;

        /// @property
        bool m_dynamicResizing = true;
        /// @property
        bool m_depthEnabled = true;
        /// @property
        bool m_instanceForEachFrame = false;

        /// @property
        SR_MATH_NS::FVector2 m_preScale = SR_MATH_NS::FVector2(1.f);
        /// @property
        SR_MATH_NS::IVector2 m_size;

        /// @property
        ColorFormats m_colorFormats = { ImageFormat::RGBA8_UNORM };

        /// @property
        FrameBufferFeatures m_features;
        /// @property
        uint8_t m_samples = 0;
        /// @property
        uint32_t m_layersCount = 1;
        /// @property
        uint32_t m_arrayLayersCount = 1;
        /// @property
        ImageFormat m_depthFormat = ImageFormat::Auto;
        /// @property
        ImageAspect m_depthAspect = ImageAspect::DepthStencil;

    };
}

#endif //SR_ENGINE_FRAME_BUFFER_CONTROLLER_H
