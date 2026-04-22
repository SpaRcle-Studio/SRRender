//
// Created by Nikita on 20.11.2020.
//

#ifndef SR_ENGINE_GRAPHICS_SKYBOX_H
#define SR_ENGINE_GRAPHICS_SKYBOX_H

#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Loaders/TextureLoader.h>

#include <Utils/Resources/IResource.h>

namespace SR_GRAPH_NS {
    class DescriptorManager;
}

namespace SR_GRAPH_NS::Memory {
    class UBOManager;
}

namespace SR_GTYPES_NS {
    class Shader;

    class Skybox : public SR_UTILS_NS::IResource, public Memory::IGraphicsResource {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Skybox>;

    public:
        Skybox();
        ~Skybox() override;

    public:
        static Skybox::Ptr CreateEmpty(bool isQuad);

    public:
        SR_NODISCARD SR_HTYPES_NS::SharedPtr<Shader> GetShader() const noexcept { return m_shader; }
        SR_NODISCARD int32_t GetVBO();
        SR_NODISCARD int32_t GetIBO();
        SR_NODISCARD int32_t GetVirtualUBO() const;

        SR_NODISCARD bool IsAllowedToRevive() const override { return true; }

        void FreeVMemory() override;
        bool Draw();
        bool Draw(Shader* pShader, bool& dirtyShader, bool& hasErrors, int32_t& virtualUBO, int32_t& virtualDescriptor);

        bool Load() override;
        bool Unload() override;

        void SetShader(const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>& shader);

        void StartWatch() override;

    protected:
        void OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) override;
        uint64_t GetFileHash() const override { return 0; }

    private:
        bool Calculate();

    private:
        SR_HTYPES_NS::SharedPtr<Shader> m_shader;

        int32_t m_VBO = SR_ID_INVALID;
        int32_t m_IBO = SR_ID_INVALID;

        int32_t m_cubeMap = SR_ID_INVALID;

        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

        uint32_t m_width = 0;
        uint32_t m_height = 0;

        bool m_hasErrors = false;
        bool m_idDirty = true;
        bool m_dirtyShader = false;
        bool m_isQuad = false;

        Memory::UBOManager& m_uboManager;
        DescriptorManager& m_descriptorManager;

        std::array<TextureData::Ptr, 6> m_data;

    };
}

#endif //SR_ENGINE_GRAPHICS_SKYBOX_H
