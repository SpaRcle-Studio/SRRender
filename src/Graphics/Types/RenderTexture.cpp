//
// Created by Monika on 18.07.2022.
//

/*
#include <Graphics/Types/RenderTexture.h>
#include <Graphics/Types/Framebuffer.h>

#include <Utils/Resources/ResourceManager.h>

#include <Codegen/RenderTexture.generated.hpp>

namespace SR_GTYPES_NS {
    RenderTexture::RenderTexture()
        : SR_UTILS_NS::Settings()
    { }

    RenderTexture::~RenderTexture() = default;

    RenderTexture::Ptr RenderTexture::Load(const SR_UTILS_NS::Path &rawPath) {
        SR_GLOBAL_LOCK

        auto&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

        if (auto&& pResource = SR_UTILS_NS::ResourceManager::Instance().Find<Settings>(path)) {
            auto&& pRenderTexture = pResource.DynamicCast<RenderTexture>();

            if (!pRenderTexture) {
                SR_ERROR("RenderTexture::Load() : failed to cast the resource!\n\tPath: " + path.ToString());
            }

            return pRenderTexture;
        }

        auto&& pRenderTexture = RenderTexture::MakeShared<RenderTexture>();

        pRenderTexture->SetId(path.ToStringRef(), false);

        if (!pRenderTexture->Reload()) {
            SR_ERROR("RenderTexture::Load() : failed to load render texture!\n\tPath: " + path.ToString());
            pRenderTexture->DeleteResource();
            pRenderTexture = nullptr;
            return nullptr;
        }

        /// отложенная ручная регистрация
        SR_UTILS_NS::ResourceManager::Instance().RegisterResource(pRenderTexture.StaticCast<SR_UTILS_NS::IResource>());

        return pRenderTexture;
    }

    void RenderTexture::ClearSettings() {
        Settings::ClearSettings();

        if (m_fbo) {
            RemoveDependency(m_fbo);
            /// Ресурс будет автоматически уничтожен либо при убирании use-point'а,
            /// либо синхронно будет освобождена видео память через контекстный класс,
            /// а затем будет уничтожен сам русурс.
            m_fbo->RemoveUsePoint();
            m_fbo = nullptr;
        }
    }

    bool RenderTexture::LoadSettings(const SR_XML_NS::Node &node) {
        //if (auto&& sizeNode = node.GetNode("Size")) {
        //    m_size = sizeNode.GetAttribute<SR_MATH_NS::UVector2>();
        //}

        //if (auto&& preScaleNode = node.GetNode("PreScale")) {
        //    m_preScale = preScaleNode.GetAttribute<SR_MATH_NS::FVector2>();
        //}

        //for (auto&& colorNode : node.GetNodes("Colors")) {
        //    m_colors.emplace_back(ColorLayer {
        //        StringToEnumColorFormat(colorNode.TryGetAttribute("Format").ToStringAtom("Unknown")),
        //        SR_ID_INVALID
        //    });
        //}

        //m_dynamicScaling = node.TryGetAttribute("dynamicScaling").ToBool(true);
        //m_depth.format = StringToEnumDepthFormat(node.GetNode("Depth").TryGetAttribute("Format").ToStringAtom("Unknown"));

        return Settings::LoadSettings(node);
    }
}
*/