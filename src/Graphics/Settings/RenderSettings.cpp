//
// Created by Monika on 31.07.2025.
//

#include <Graphics/Settings/RenderSettings.h>

#include <Utils/FileSystem/PathDataAccessor.h>

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
}
