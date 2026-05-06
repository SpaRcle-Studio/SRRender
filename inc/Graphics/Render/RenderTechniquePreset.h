//
// Created by Monika on 08.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_PRESET_H
#define SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_PRESET_H

#include <Graphics/Render/RenderTechnique.h>
#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    class FileRenderTechniquePresetResource;

    /// @abstract
    class RenderTechniqueLayerBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RenderTechniqueLayerBase> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RenderTechniqueLayerBase>;

        RenderTechniqueLayerBase()
            : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        SR_NODISCARD SR_UTILS_NS::StringAtom GetLayerName() const noexcept { return layer; }

    public:
        /// @property
        SR_UTILS_NS::StringAtom layer;
        /// @property
        bool editorOnly = false;

    };

    /// @abstract
    class RenderTechniquePresetIntegrationBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RenderTechniquePresetIntegrationBase> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RenderTechniquePresetIntegrationBase>;
        using Params = RenderTechniqueLoadParams;
        using Technique = FileRenderTechniquePresetResource;

        RenderTechniquePresetIntegrationBase()
            : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }
    public:
        virtual void Integrate(const Technique& technique, const Params& params) const { }

    };

    class RenderTechniqueLayerMesh : public RenderTechniqueLayerBase {
        SR_CLASS()
    public:
        /// @property
        std::set<SR_UTILS_NS::StringAtom> allowedLayers;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> disallowedLayers;
        /// @property
        bool mainRenderer = true;
        /// @property
        bool castShadows = false;
        /// @property
        bool applyShadows = false;
        /// @property
        bool colorBuffer = false;
        /// @property
        bool frustumCulling = true;

    };

    class RenderTechniqueLayerCustomPass : public RenderTechniqueLayerBase {
        SR_CLASS()
    public:
        /// @property @notNull
        BasePass::Ptr pass;

    };

    class RenderTechniquePresetIntegrationShadows : public RenderTechniquePresetIntegrationBase {
        SR_CLASS()
    public:
        void Integrate(const Technique& technique, const Params& params) const override;

    public:
        /// @property
        SR_UTILS_NS::StringAtom shadowMapControllerName = "DepthCascades";
        /// @property
        SR_UTILS_NS::StringAtom shaderVariableName = "shadowMap";

    private:
        /// @property
        std::set<SR_UTILS_NS::StringAtom> m_specialShadowLayers;

    };

    class RenderTechniquePresetIntegrationColorBuffer : public RenderTechniquePresetIntegrationBase {
        SR_CLASS()
    public:
        void Integrate(const Technique& technique, const Params& params) const override;

    public:
        /// @property
        SR_UTILS_NS::StringAtom colorBufferControllerName = "ColorBuffer";
        /// @property
        uint32_t colorMultiplier = 5000;

    };

    class RenderTechniquePresetIntegrationAutoExposure : public RenderTechniquePresetIntegrationBase {
        SR_CLASS()
    public:
        void Integrate(const Technique& technique, const Params& params) const override;

    };

    class RenderTechniquePresetIntegrationSSAO : public RenderTechniquePresetIntegrationBase {
        SR_CLASS()
    public:
        void Integrate(const Technique& technique, const Params& params) const override;

        /// @property
        SR_UTILS_NS::StringAtom m_SSAOname = "SSAO";
    };

    class RenderTechniquePresetIntegrationMainView : public RenderTechniquePresetIntegrationBase {
        SR_CLASS()
    public:
        void Integrate(const Technique& technique, const Params& params) const override;

    private:
        void AddLayers(
            GroupPass& groupPass,
            const Technique& technique,
            const Params& params,
            bool useOffscreenRender,
            bool isSceneView) const;

    public:
        /// @property
        SR_UTILS_NS::StringAtom offscreenControllerName = "Offscreen";
        /// @property
        uint8_t mainRenderColorLayers = 1;
        /// @property
        PostProcessPass::Ptr defaultPostProcessPass;

    };

    class RenderTechniqueLayerSkybox : public RenderTechniqueLayerBase {
        SR_CLASS()
    };

    class RenderTechniqueLayerClearDepth : public RenderTechniqueLayerBase {
        SR_CLASS()
    };

    /// @extension(srptech)
    class FileRenderTechniquePresetResource : public IFileRenderTechniqueResource {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<FileRenderTechniquePresetResource>;
        using Params = RenderTechniqueLoadParams;

    public:
        static FileRenderTechniquePresetResource::Ptr Load(const SR_UTILS_NS::Path& path);

    public:
        const RenderTechniqueData& GetData(const Params& params) const noexcept override;
        RenderTechniqueData& GetInternalData() const noexcept { return m_data; }

        template<typename T> SR_NODISCARD SR_HTYPES_NS::SharedPtr<T> FindIntegration() const noexcept {
            for (auto&& pIntegration : m_integrations) {
                if (pIntegration && pIntegration->GetMeta() == T::GetMetaStatic()) {
                    return SR_UTILS_NS::StaticPointerCast<T>(pIntegration);
                }
            }
            return nullptr;
        }

        SR_NODISCARD const std::vector<RenderTechniqueLayerBase::Ptr>& GetCustomLayers() const noexcept { return m_customLayers; }

    private:
        mutable RenderTechniqueData m_data;

        /// @property @notNull
        std::vector<RenderTechniquePresetIntegrationBase::Ptr> m_integrations;
        /// @property
        std::vector<RenderTechniqueLayerBase::Ptr> m_customLayers;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_TECHNIQUE_PRESET_H
