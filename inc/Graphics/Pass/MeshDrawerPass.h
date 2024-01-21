//
// Created by Monika on 18.01.2024.
//

#ifndef SR_ENGINE_MESH_DRAWER_PASS_H
#define SR_ENGINE_MESH_DRAWER_PASS_H

#include <Graphics/Pass/BasePass.h>

namespace SR_GRAPH_NS {
    class RenderStrategy;

    class MeshDrawerPass : public BasePass {
        SR_REGISTER_LOGICAL_NODE(MeshDrawerPass, Mesh Drawer Pass, { "Passes" })
        using Super = BasePass;
    public:
        void Bind() override;
        bool Render() override;
        void Update() override;

        bool HasPreRender() const noexcept override { return false; }
        bool HasPostRender() const noexcept override { return false; }

    protected:
        virtual void UseUniforms(ShaderPtr pShader, MeshPtr pMesh);
        virtual void UseSharedUniforms(ShaderPtr pShader);
        virtual void UseConstants(ShaderPtr pShader);
        virtual void UseSamplers(ShaderPtr pShader);

    private:
        ShaderPtr ReplaceShader(ShaderPtr pShader) const;

        bool IsLayerAllowed(SR_UTILS_NS::StringAtom layer) const;

        SR_NODISCARD RenderStrategy* GetRenderStrategy() const;

    private:
        bool m_useMaterial = true;

        bool m_passWasRendered = false;

        std::map<ShaderPtr, ShaderPtr> m_shaderReplacements;
        std::set<SR_UTILS_NS::StringAtom> m_allowedLayers;
        std::set<SR_UTILS_NS::StringAtom> m_disallowedLayers;

    };
}

#endif //SR_ENGINE_MESH_DRAWER_PASS_H