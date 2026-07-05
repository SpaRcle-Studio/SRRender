//
// Created by Monika on 23.04.2023.
//

#ifndef SR_ENGINE_ANIMATIONGRAPH_H
#define SR_ENGINE_ANIMATIONGRAPH_H

#include <Graphics/Animations/AnimationGraphNode.h>

#include <Utils/Resources/Asset.h>

namespace SR_ANIMATIONS_NS {
    class Animator;
    class AnimationClip;
    class AnimationGraphAsset;

    class AnimationGraph : public IAnimationDataSet {
        SR_CLASS()
        using Hash = uint64_t;
        using Super = IAnimationDataSet;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationGraph>;

    public:
        AnimationGraph();
        ~AnimationGraph() override;

    public:
        void OnPostLoad() override;
        void CloneTo(SR_UTILS_NS::SRClass& clone) const override;
        void SetSimpleClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip);

        SR_NODISCARD AnimationGraphNode* GetNode(uint64_t index) const;
        SR_NODISCARD uint64_t GetNodeIndex(const AnimationGraphNode* pNode) const;
        SR_NODISCARD AnimationGraphNode* GetFinal() const;
        SR_NODISCARD bool IsStateActive(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD uint32_t GetNodesCount() const noexcept { return static_cast<uint32_t>(m_nodes.size()); }
        SR_NODISCARD const std::vector<AnimationGraphNode::Ptr>& GetNodes() const noexcept { return m_nodes; }
        SR_NODISCARD std::vector<AnimationGraphNode::Ptr>& GetNodes() noexcept { return m_nodes; }
        SR_NODISCARD const SR_UTILS_NS::Path& GetPath() const noexcept;
        SR_NODISCARD AnimationGraphAsset* GetAsset() const noexcept { return m_pAsset; }
        void InvalidateCompile() noexcept { m_isCompiled = false; }

        void Update(UpdateContext& context);

        template<class T, typename... Args> T* CreateNode(Args&& ...args) {
            return AddNode(new T(std::forward<Args>(args)...));
        }

        template<class T> T* AddNode(T* pNode) {
            SR_STATIC_ASSERT2((std::is_base_of_v<AnimationGraphNode, T>), "T must be derived from AnimationGraphNode");
            m_nodes.emplace_back(pNode);
            pNode->SetGraph(this);
            return pNode;
        }

        void SetAsset(AnimationGraphAsset* pAsset);
        void SetAnimator(Animator* pAnimator) { m_pAnimator = pAnimator; }

    private:
        void Apply(AnimationPose* pPose);
        void Compile();

    private:
        bool m_isCompiled = false;
        Animator* m_pAnimator = nullptr;
        AnimationGraphAsset* m_pAsset = nullptr;
        std::vector<SR_UTILS_NS::GameObject::Ptr> m_gameObjects;
        std::vector<AnimationGameObjectData> m_gameObjectsCache;

    private:
        /// @property
        std::vector<AnimationGraphNode::Ptr> m_nodes;

    };

    /// @extension(animator)
    class AnimationGraphAsset : public SR_UTILS_NS::Asset {
        SR_CLASS()
        using Super = SR_UTILS_NS::Asset;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationGraphAsset>;

    public:
        AnimationGraphAsset();
        ~AnimationGraphAsset() override;

        void OnAssetLoaded() override;

    public:
        const AnimationGraph& GetData() const noexcept;
        AnimationGraph& GetDataMutable() const noexcept;

    private:
        /// @property @noHeader @notNull
        mutable AnimationGraph::Ptr m_data;

    };
}

#endif //SRENGINEANIMATIONRGRAPH_H
