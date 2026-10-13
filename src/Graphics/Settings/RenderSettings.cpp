//
// Created by Monika on 31.07.2025.
//

#include <Graphics/Settings/RenderSettings.h>

#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/Platform/Platform.h>

#include <Codegen/RenderSettings.generated.hpp>

namespace SR_GRAPH_NS {
    const RenderSettingsPreset& RenderSettings::GetPreset(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        static SR_UTILS_NS::StringAtom defaultPresetName = "Default";
        if (name.empty() || name == defaultPreset.name || name == defaultPresetName) {
            return defaultPreset;
        }

        auto&& pIt = std::ranges::find_if(presets, [name](auto&& preset) {
            return preset.name == name;
        });
        if (pIt != presets.end()) {
            return *pIt;
        }

        return defaultPreset;
    }

    const ShadowQualityPreset& RenderSettings::GetShadowQualityPreset(Quality quality) const {
        return FindSuitableQuality(quality, shadowQualityPresets);
    }

    float_t RenderSettings::GetColorBufferResolutionCoefficient(Quality quality) const {
        const float_t coefficient = FindSuitableQuality(quality, colorBufferQualityPresets);
        if (coefficient <= 0.0f) {
            return 1.0f;
        }
        return coefficient;
    }

    const SSAOPreset& RenderSettings::GetSSAOResolutionPreset(Quality quality) const {
       return FindSuitableQuality(quality, SSAOQualityPresets);
    }

    BloomPreset RenderSettings::GetBloomPreset(Quality quality) const {
        if (!bloomQualityPresets.empty()) {
            return FindSuitableQuality(quality, bloomQualityPresets);
        }

        BloomPreset preset;
        switch (quality) {
            case Quality::Low:
                preset.preScale = 0.25f;
                preset.mipCount = 4;
                preset.highQualityFilter = false;
                break;
            case Quality::Medium:
                preset.mipCount = 5;
                preset.highQualityFilter = false;
                break;
            case Quality::High:
                preset.mipCount = 6;
                break;
            case Quality::Ultra:
                preset.mipCount = 7;
                preset.highPrecision = true;
                break;
            default:
                preset.mipCount = 8;
                preset.highPrecision = true;
                break;
        }
        return preset;
    }

    const ActiveGraphicsSettings& RenderSettings::GetDefaultGraphicsSettings() const {
        for (auto&& settings : defaultPlatformGraphicsSettings) {
            if (settings.platform.has_value() && settings.platform.value() != SR_PLATFORM_NS::GetType()) {
                continue;
            }
            return settings.graphics;
        }
        static ActiveGraphicsSettings defaultSettings;
        return defaultSettings;
    }
}
