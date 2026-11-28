//
// Created by Monika on 28.11.2026.
//

#ifndef SR_ENGINE_GRAPHICS_VOLUMETRICS_PASS_H
#define SR_ENGINE_GRAPHICS_VOLUMETRICS_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    /// Объемные эффекты одним проходом в буфер пониженного разрешения: облака (VolumetricClouds), туман и
    /// атмосферная перспектива (VolumetricFog). Один луч, одно чтение дистанции сцены и один апсемпл в post-process
    /// вместо отдельного прохода на каждый эффект. Какие эффекты скомпилированы - макросы SR_VOLUMETRIC_CLOUDS /
    /// SR_VOLUMETRIC_FOG (настройки графики), какие считаются - наличие компонентов на сцене (volumetricsFlags).
    class VolumetricsPass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<VolumetricsPass>;

    public:
        bool Init() override;
        bool Render() override;

        void SetShadowPassName(SR_UTILS_NS::StringAtom name) { m_shadowPassName = name; }

    protected:
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;

    private:
        /// Каскадные тени для лучей в тумане, пусто - без теней
        /// @property
        SR_UTILS_NS::StringAtom m_shadowPassName;

    };
}

#endif //SR_ENGINE_GRAPHICS_VOLUMETRICS_PASS_H
