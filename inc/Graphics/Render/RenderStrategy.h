//
// Created by Monika on 17.01.2024.
//

#ifndef SR_ENGINE_RENDER_STRATEGY_H
#define SR_ENGINE_RENDER_STRATEGY_H

#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Pipeline/IShaderProgram.h>

#include <Utils/ECS/Transform.h>

/**
    - Layer (Only render)
        - Priority (Only render)
            - Shader (Render/Update)
                * OnShaderUse
                - VBO (Render/Update)
                    * OnBindVBO
                    - Mesh
                    - Mesh
                    * OnUnBingVBO
               * OnShaderUnUse
*/

namespace SR_GTYPES_NS {
    class Mesh;
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderStrategy;
    class RenderScene;
    class MeshDrawerPass;

    /// ----------------------------------------------------------------------------------------------------------------

    class IRenderStage : public SR_UTILS_NS::NonCopyable {
        using Super = SR_UTILS_NS::NonCopyable;
    public:
        using MeshPtr = SR_GTYPES_NS::Mesh*;

    public:
        IRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent);

    public:
        using ShaderPtr = SR_GTYPES_NS::Shader*;

    public:
        virtual bool RegisterMesh(MeshRegistrationInfo& info) { return false; }
        virtual bool UnRegisterMesh(const MeshRegistrationInfo& info) { return false; }

        void PostUpdate() { m_uniformsDirty = false; }

        SR_NODISCARD virtual bool IsRendered() const { return m_isRendered; }
        SR_NODISCARD virtual bool IsEmpty() const { return true; }

        SR_NODISCARD RenderContext* GetRenderContext() const;
        SR_NODISCARD RenderScene* GetRenderScene() const;

        void SetError(SR_UTILS_NS::StringAtom error);

        SR_NODISCARD virtual bool IsValid() const { return true; }

        virtual void MarkUniformsDirty() { m_uniformsDirty = true; }

        virtual void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const { }

    protected:
        SR_NODISCARD bool IsNeedUpdate() const;

    protected:
        bool m_isRendered = false;
        IRenderStage* m_parent = nullptr;
        RenderStrategy* m_renderStrategy = nullptr;
        RenderContext* m_renderContext = nullptr;

    private:
        bool m_uniformsDirty = true;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    /**
        build queue:

        LayerRenderStage
            PriorityRenderStage
                MaterialRenderStage
                    VBORenderStage
                        MeshRenderStage

        render queue:

        for add/remove use lower_bound to find shader

        shader 0x0021
        shader 0x2362
        shader 0x1132

        struct RenderQueue {
            std::vector<std::pair<Shader*, > shaders;
        };

    */

    class MeshRenderStage : public IRenderStage {
        using Super = IRenderStage;
        using MeshList = std::vector<SR_GTYPES_NS::Mesh*>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
    public:
        explicit MeshRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent);

        ~MeshRenderStage() override {
            SRAssert(m_meshes.empty());
        }

    public:
        SR_NODISCARD bool HasActiveMesh() const;

        virtual bool Render();

        void Update(ShaderUseInfo info);

        bool RegisterMesh(MeshRegistrationInfo& info) override;
        bool UnRegisterMesh(const MeshRegistrationInfo& info) override;

        SR_NODISCARD bool IsEmpty() const override { return m_meshes.empty(); }

        void MarkUniformsDirty() override;

        void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const override;

    protected:
        Memory::UBOManager& m_uboManager;
        MeshList m_meshes;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class VBORenderStage : public MeshRenderStage {
        using Super = MeshRenderStage;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
    public:
        VBORenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, int32_t VBO);

    public:
        bool Render() override;

        SR_NODISCARD bool IsValid() const override { return m_VBO != SR_ID_INVALID; }

    private:
        int32_t m_VBO = -1;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class ShaderRenderStage : public IRenderStage {
        using Super = IRenderStage;
    public:
        explicit ShaderRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, SR_GTYPES_NS::Shader* pShader);
        ~ShaderRenderStage() override;

    public:
        bool Render();
        void Update();
        void PostUpdate();

        bool RegisterMesh(MeshRegistrationInfo& info) override;
        bool UnRegisterMesh(const MeshRegistrationInfo& info) override;

        SR_NODISCARD bool IsEmpty() const override { return m_VBOStages.empty() && m_meshStage->IsEmpty(); }
        SR_NODISCARD bool IsValid() const override { return m_shader; }

        void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const override;

    private:
        SR_NODISCARD bool HasActiveMesh() const;

    private:
        SR_GTYPES_NS::Shader* m_shader = nullptr;

        MeshRenderStage* m_meshStage = nullptr;
        std::map<int32_t, VBORenderStage*> m_VBOStages;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class MaterialRenderStage : public IRenderStage {
        using Super = IRenderStage;
    public:
        explicit MaterialRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, SR_GRAPH_NS::BaseMaterial* pMaterial);
        ~MaterialRenderStage() override;

    public:
        bool Render();
        void Update();
        void PostUpdate();

        bool RegisterMesh(MeshRegistrationInfo& info) override;
        bool UnRegisterMesh(const MeshRegistrationInfo& info) override;

        SR_NODISCARD bool IsEmpty() const override { return m_VBOStages.empty() && m_meshStage->IsEmpty(); }
        SR_NODISCARD bool IsValid() const override { return m_material; }

        void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const override;

    private:
        SR_NODISCARD bool HasActiveMesh() const;

    private:
        SR_GRAPH_NS::BaseMaterial* m_material = nullptr;

        MeshRenderStage* m_meshStage = nullptr;
        std::map<int32_t, VBORenderStage*> m_VBOStages;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class PriorityRenderStage : public IRenderStage {
        using Super = IRenderStage;
    public:
        explicit PriorityRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, int64_t priority)
            : Super(pRenderStrategy, pParent)
            , m_priority(priority)
        { }

        ~PriorityRenderStage() override {
            SRAssert(m_shaderStages.empty());
        }

    public:
        bool Render();
        void Update();
        void PostUpdate();

        bool RegisterMesh(MeshRegistrationInfo& info) override;
        bool UnRegisterMesh(const MeshRegistrationInfo& info) override;

        SR_NODISCARD int64_t GetPriority() const noexcept { return m_priority; }

        SR_NODISCARD bool IsEmpty() const override { return m_shaderStages.empty(); }

        void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const override;

    private:
        int64_t m_priority = 0;
        std::map<ShaderPtr, ShaderRenderStage*> m_shaderStages;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class LayerRenderStage : public IRenderStage {
        using Super = IRenderStage;
    public:
        explicit LayerRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent)
            : Super(pRenderStrategy, pParent)
        { }

        ~LayerRenderStage() override {
            SRAssert(m_priorityStages.empty() && m_shaderStages.empty());
        }

    public:
        bool Render();
        void Update();
        void PostUpdate();

        bool RegisterMesh(MeshRegistrationInfo& info) override;
        bool UnRegisterMesh(const MeshRegistrationInfo& info) override;

        SR_NODISCARD int64_t FindPriorityStageIndex(int64_t priority, bool nearest) const;

        SR_NODISCARD bool IsEmpty() const override { return m_priorityStages.empty() && m_shaderStages.empty(); }

        void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const override;

    private:
        void InsertPriorityStage(PriorityRenderStage* pStage);

    private:
        std::vector<PriorityRenderStage*> m_priorityStages;
        std::map<ShaderPtr, ShaderRenderStage*> m_shaderStages;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class RenderStrategy : public SR_UTILS_NS::NonCopyable {
        using Super = SR_UTILS_NS::NonCopyable;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using VoidCallback = SR_HTYPES_NS::Function<void()>;
        using FilterCallback = SR_HTYPES_NS::Function<bool(SR_UTILS_NS::StringAtom)>;
        using PriorityCallback = SR_HTYPES_NS::Function<bool(int64_t)>;
        using ShaderCallback = SR_HTYPES_NS::Function<void(SR_GTYPES_NS::Shader*)>;
        using MeshShaderCallback = SR_HTYPES_NS::Function<void(SR_GTYPES_NS::Shader*,SR_GTYPES_NS::Mesh*)>;
        using ShaderReplaceCallback = SR_HTYPES_NS::Function<SR_GRAPH_NS::ShaderUseInfo(SR_GTYPES_NS::Shader*)>;
    public:
        explicit RenderStrategy(RenderScene* pRenderScene);
        ~RenderStrategy() override;

    public:
        bool Render();
        void Update();
        void PostUpdate();

        void BindFilterCallback(FilterCallback callback) { m_layerFilter = std::move(callback); }
        void BindShaderReplaceCallback(ShaderReplaceCallback callback) { m_shaderReplaceCallback = std::move(callback); }
        void BindPriorityCallback(PriorityCallback callback) { m_priorityCallback = std::move(callback); }

        void RegisterMesh(SR_GTYPES_NS::Mesh* pMesh);
        bool UnRegisterMesh(SR_GTYPES_NS::Mesh* pMesh);
        void ReRegisterMesh(const MeshRegistrationInfo& info);

        void SetMeshDrawerPass(MeshDrawerPass* pMeshDrawerPass) { m_meshDrawerPass = pMeshDrawerPass; }

        void OnResourceReloaded(SR_UTILS_NS::IResource* pResource) const;

        SR_NODISCARD RenderContext* GetRenderContext() const;
        SR_NODISCARD RenderScene* GetRenderScene() const { return m_renderScene; }
        SR_NODISCARD ShaderUseInfo ReplaceShader(ShaderPtr pShader) const;
        SR_NODISCARD bool IsPriorityAllowed(int64_t priority) const;
        SR_NODISCARD bool IsNeedCheckMeshActivity() const noexcept { return m_isNeedCheckMeshActivity; }
        SR_NODISCARD bool IsDebugModeEnabled() const noexcept { return m_enableDebugMode; }
        SR_NODISCARD bool IsUniformsDirty() const noexcept { return m_isUniformsDirty; }
        SR_NODISCARD const std::set<SR_UTILS_NS::StringAtom>& GetErrors() const noexcept { return m_errors; }
        SR_NODISCARD const std::set<SR_GTYPES_NS::Mesh*>& GetProblemMeshes() const noexcept { return m_problemMeshes; }

        SR_NODISCARD MeshDrawerPass* GetMeshDrawerPass() const noexcept { return m_meshDrawerPass; }

        void ClearErrors();
        void AddError(SR_UTILS_NS::StringAtom error) { m_errors.insert(error); }
        void AddProblemMesh(SR_GTYPES_NS::Mesh* pMesh) { m_problemMeshes.insert(pMesh); }
        void SetDebugMode(bool value);

        void ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const;

        void MarkUniformsDirty() { m_isUniformsDirty = true; }

    private:
        void RegisterMesh(MeshRegistrationInfo info);
        bool UnRegisterMesh(const MeshRegistrationInfo& info);

        MeshRegistrationInfo CreateMeshRegistrationInfo(SR_GTYPES_NS::Mesh* pMesh) const;

    private:
        FilterCallback m_layerFilter;
        ShaderReplaceCallback m_shaderReplaceCallback;
        PriorityCallback m_priorityCallback;

        MeshDrawerPass* m_meshDrawerPass = nullptr;

        RenderScene* m_renderScene = nullptr;

        std::set<SR_UTILS_NS::StringAtom> m_errors;
        std::set<SR_GTYPES_NS::Mesh*> m_problemMeshes;

        bool m_isNeedCheckMeshActivity = true;
        bool m_enableDebugMode = false;
        bool m_isUniformsDirty = true;

        std::list<MeshRegistrationInfo> m_reRegisterMeshes;

        uint32_t m_meshCount = 0;

        std::map<SR_UTILS_NS::StringAtom, LayerRenderStage*> m_layers;

    };
}

#endif //SR_ENGINE_RENDER_STRATEGY_H
