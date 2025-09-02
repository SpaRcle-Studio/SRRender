//
// Created by Monika on 31.08.2025.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_SETTINGS_H
#define SR_ENGINE_GRAPHICS_RENDER_SETTINGS_H

#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/SRSL/ShaderVariables.h>

#include <Utils/Resources/Asset.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(Quality, uint8_t,
        None,
        Low,
        Medium,
        High,
        Ultra,
        Extreme
    );

    struct RenderSettingsPreset : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::StringAtom name;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> shaderDefines;
        /// @property
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech)
        SR_UTILS_NS::Path mainCameraRenderTechnique;
        /// @property
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech)
        SR_UTILS_NS::Path editorCameraRenderTechnique;
        /// @property
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech)
        SR_UTILS_NS::Path prefabCameraRenderTechnique;

    };

    class RenderSettings : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RenderSettings>;

    public:
        SR_NODISCARD const RenderSettingsPreset& GetPreset(SR_UTILS_NS::StringAtom name) const;

    public:
        /// @property
        SR_UTILS_NS::StringAtom appName = "SpaRcle Engine";
        /// @property
        SR_UTILS_NS::StringAtom engineName = "SREngine";

        /// @property
        std::vector<RenderSettingsPreset> presets;
        /// @property
        RenderSettingsPreset defaultPreset;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_SETTINGS_H
