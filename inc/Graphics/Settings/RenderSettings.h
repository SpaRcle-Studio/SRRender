//
// Created by Monika on 31.08.2025.
//

#ifndef SR_ENGINE_GRAPHICS_RENDER_SETTINGS_H
#define SR_ENGINE_GRAPHICS_RENDER_SETTINGS_H

#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/SRSL/ShaderVariables.h>
#include <Graphics/Settings/Quality.h>

#include <Utils/Resources/Asset.h>

namespace SR_GRAPH_NS {
    template<typename K, typename V> const V& FindSuitableQuality(Quality quality, const std::map<K, V>& presets, bool findLowerFirst = false) {
        if (auto&& pIt = presets.find(quality); pIt != presets.end()) {
            return pIt->second;
        }

        auto pAboveIt = presets.upper_bound(quality);
        auto tryLower = [&]() -> const V* { return (pAboveIt != presets.begin()) ? &std::prev(pAboveIt)->second : nullptr; };
        auto tryUpper = [&]() -> const V* { return (pAboveIt != presets.end()) ? &pAboveIt->second : nullptr; };

        if (findLowerFirst) {
            if (auto p = tryLower()) return *p;
            if (auto p = tryUpper()) return *p;
        }
        else {
            if (auto p = tryUpper()) return *p;
            if (auto p = tryLower()) return *p;
        }

        static V defaultValue{};
        return defaultValue;
    }

    struct ShadowQualityPreset : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        bool instancing = true;
        /// @property
        uint32_t shadowMapResolution = 4096;
        /// @property
        uint8_t cascadesCount = 4;
        /// @property
        uint8_t frustumCount = 2;
        /// @property
        float_t oneMeterUnit = 0.1f;
        /// @property
        float_t maxShadowDistance = 500.f;
        /// @property
        float_t split1 = 25.f;
        /// @property
        float_t split2 = 75.f;
        /// @property
        float_t split3 = 150.f;

    };

    struct RenderSettingsPreset : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        SR_UTILS_NS::StringAtom name;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> shaderDefines;
        /// @property
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech,srptech)
        SR_UTILS_NS::Path mainCameraRenderTechnique;
        /// @property
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech,srptech)
        SR_UTILS_NS::Path editorCameraRenderTechnique;

    };

    class RenderSettings : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RenderSettings>;

    public:
        SR_NODISCARD const RenderSettingsPreset& GetPreset(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD const ShadowQualityPreset& GetShadowQualityPreset(Quality quality) const;
        SR_NODISCARD float_t GetColorBufferResolutionCoefficient(Quality quality) const;

    public:
        /// @property
        SR_UTILS_NS::StringAtom appName = "SpaRcle Engine";
        /// @property
        SR_UTILS_NS::StringAtom engineName = "SREngine";
        /// @property
        /// @customArgs(pick: enabled, filter name: Render Techniques, relative: resources)
        /// @customArg(filter value: srtech,srptech)
        SR_UTILS_NS::Path overlayRenderTechnique = "Editor/Render/Overlay.srtech";
        /// @property
        /// @customArgs(pick: enabled, filter name: Shader, relative: resources)
        /// @customArg(filter value: srsl)
        SR_UTILS_NS::Path defaultShader = "Engine/Shaders/standard.srsl";
        /// @property
        /// @customArgs(pick: enabled, filter name: Material, relative: resources)
        /// @customArg(filter value: mat)
        SR_UTILS_NS::Path defaultMaterial = "Engine/Materials/default.mat";
        /// @property
        SR_UTILS_NS::StringAtom editorSceneImageName = "SceneView";

        /// @property
        std::vector<RenderSettingsPreset> presets;
        /// @property
        RenderSettingsPreset defaultPreset;

        /// @property
        std::map<Quality, ShadowQualityPreset> shadowQualityPresets;
        /// @property
        std::map<Quality, float_t> colorBufferQualityPresets;

    };
}

#endif //SR_ENGINE_GRAPHICS_RENDER_SETTINGS_H
