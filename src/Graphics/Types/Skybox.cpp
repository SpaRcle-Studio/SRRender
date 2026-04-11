//
// Created by Nikita on 20.11.2020.
//

#include <Graphics/Types/Skybox.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/FileWatcher.h>
#include <Utils/Common/StringUtils.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/Vertices.h>

#include <Codegen/Skybox.generated.hpp>

namespace SR_GTYPES_NS {
    Skybox::Skybox()
        : m_uboManager(Memory::UBOManager::Instance())
        , m_descriptorManager(DescriptorManager::Instance())
    { }

    Skybox::~Skybox() {
        SetShader(nullptr);

        SRAssert(
            m_cubeMap == SR_ID_INVALID &&
            m_virtualUBO == SR_ID_INVALID &&
            m_VBO == SR_ID_INVALID &&
            m_IBO == SR_ID_INVALID
        );

        for (auto&& img : m_data) {
            img.Reset();
        }
    }

    Skybox::Ptr Skybox::CreateEmpty(bool isQuad) {
        SR_GLOBAL_LOCK;
        SR_TRACY_ZONE;

        auto&& pSkybox = Skybox::MakeShared<Skybox>();

        pSkybox->m_isFromMemory = true;
        pSkybox->m_isQuad = isQuad;
        pSkybox->SetId("Skybox_From_Memory");

        return pSkybox;
    }

    bool Skybox::Calculate() {
        if (!m_idDirty) {
            SR_ERROR("Skybox::Calculate() : the skybox is already calculated!");
            return false;
        }

        RegisterGraphicsResource();

        if (!IsResourceFromMemory()) {
            SRCubeMapCreateInfo createInfo;
            createInfo.cpuUsage = SR_UTILS_NS::Features::Instance().Enabled("SkyboxCPUUsage", false);
            createInfo.width = m_width;
            createInfo.height = m_height;

            for (uint32_t i = 0; i < 6; ++i) {
                if (!m_data[i]) {
                    SR_ERROR("Skybox::Calculate() : failed to calculate cube map! Side {} is invalid!", i);
                    m_hasErrors = true;
                    return false;
                }

                createInfo.data[i] = m_data[i]->GetData();
            }

            if (m_cubeMap = GetPipeline()->AllocateCubeMap(createInfo); m_cubeMap < 0) {
                SR_ERROR("Skybox::Calculate() : failed to calculate cube map!");
                m_hasErrors = true;
                return false;
            }
        }

        if (!m_isQuad) {
            SR_UTILS_NS::VertexDataBuffer indexedVertices;

            indexedVertices.layout = SR_UTILS_NS::VertexLayoutDescription()
                .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3);

            indexedVertices.Allocate(SR_UTILS_NS::SKYBOX_INDEXED_VERTICES.size());
            for (size_t i = 0; i < SR_UTILS_NS::SKYBOX_INDEXED_VERTICES.size(); ++i) {
                indexedVertices.SetVertex(i, SR_UTILS_NS::VertexAttribute::Position, &SR_UTILS_NS::SKYBOX_INDEXED_VERTICES[i]);
            }

            if (m_VBO = GetPipeline()->AllocateVBO(indexedVertices.GetDataSize(), indexedVertices.GetRawData()); m_VBO == SR_ID_INVALID) {
                SR_ERROR("Skybox::Calculate() : failed to calculate VBO!");
                m_hasErrors = true;
                return false;
            }

            auto&& indices = SR_UTILS_NS::SKYBOX_INDICES;
            if (m_IBO = GetPipeline()->AllocateIBO((void *) indices.data(), sizeof(uint32_t), indices.size(), SR_ID_INVALID); m_IBO == SR_ID_INVALID) {
                SR_ERROR("Skybox::Calculate() : failed to calculate IBO!");
                m_hasErrors = true;
                return false;
            }
        }

        m_idDirty = false;

        return true;
    }

    void Skybox::FreeVMemory() {
        SR_LOG("Skybox::FreeVideoMemory() : free skybox video memory...");

        if (m_VBO != SR_ID_INVALID && !GetPipeline()->FreeVBO(&m_VBO)) {
            SR_ERROR("Skybox::FreeVideoMemory() : failed to free VBO!");
        }

        if (m_IBO != SR_ID_INVALID && !GetPipeline()->FreeIBO(&m_IBO)) {
            SR_ERROR("Skybox::FreeVideoMemory() : failed to free IBO!");
        }

        if (m_cubeMap != SR_ID_INVALID && !GetPipeline()->FreeCubeMap(&m_cubeMap)) {
            SR_ERROR("Skybox::FreeVideoMemory() : failed to free cube map!");
        }

        auto&& uboManager = Memory::UBOManager::Instance();
        if (m_virtualUBO != SR_ID_INVALID && !uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("Mesh::FreeVideoMemory() : failed to free virtual uniform buffer object!");
        }

        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();
        if (m_virtualDescriptor != SR_ID_INVALID) {
            descriptorManager.FreeDescriptorSet(&m_virtualDescriptor);
        }

        SetShader(nullptr);

        m_idDirty = true;
        m_dirtyShader = true;
        m_hasErrors = false;

        IGraphicsResource::FreeVMemory();
    }

    bool Skybox::Draw() {
        SR_TRACY_ZONE;

        if (m_idDirty && (m_hasErrors || !Calculate())) {
            return false;
        }

        if (!GetPipeline()) SR_UNLIKELY_ATTRIBUTE {
            SR_ERROR("Skybox::Draw() : pipeline is null!");
            return false;
        }

        if (!GetPipeline()->GetCurrentShader()) {
            SR_ERROR("Skybox::Draw() : current shader is null!");
            return false;
        }

        auto&& uboManager = Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return false;
            }

            m_virtualDescriptor = descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            if (m_cubeMap != SR_ID_INVALID) {
                m_shader->SetSamplerCube(SHADER_SKYBOX_DIFFUSE, m_cubeMap);
            }
            m_descriptorManager.Flush();
        }
        GetPipeline()->GetCurrentShader()->FlushConstants();

        if (!m_isQuad) {
            GetPipeline()->BindVBO(m_VBO);
            GetPipeline()->BindIBO(m_IBO);
        }

        if (result != DescriptorManager::BindResult::Failed) {
            m_dirtyShader = false;
            if (m_isQuad) {
                GetPipeline()->Draw(3);
            }
            else {
                GetPipeline()->DrawIndices(36);
            }
            return true;
        }

        return false;
    }

    void Skybox::OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) {
        if (dynamic_cast<Shader*>(pContainer) == m_shader.Get() && m_shader) {
            m_dirtyShader = true;
            m_hasErrors = false;
        }

        IResource::OnResourceUpdated(pContainer, depth);
    }

    void Skybox::SetShader(const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>& shader) {
        if (m_shader == shader) {
            return;
        }

        m_dirtyShader = true;

        if (m_shader) {
            RemoveDependency(SR_UTILS_NS::StaticPointerCast<SR_UTILS_NS::ResourceContainer>(m_shader));
            m_shader = nullptr;
        }

        if (!(m_shader = shader)) {
            return;
        }

        AddDependency(SR_UTILS_NS::StaticPointerCast<SR_UTILS_NS::ResourceContainer>(m_shader));
    }

    int32_t Skybox::GetVBO() {
        if (m_idDirty && (m_hasErrors || !Calculate())) {
            return SR_ID_INVALID;
        }

        return m_VBO;
    }

    int32_t Skybox::GetIBO() {
        if (m_idDirty && (m_hasErrors || !Calculate())) {
            return SR_ID_INVALID;
        }

        return m_IBO;
    }

    int32_t Skybox::GetVirtualUBO() const {
        return m_virtualUBO;
    }

    void Skybox::StartWatch() {
        for (auto&& pTextureData : m_data) {
            if (!pTextureData) {
                continue;
            }

            auto&& pWatch = SR_UTILS_NS::FileWatcher::MakeShared(pTextureData->GetPath());

            pWatch->SetCallBack([this](auto&& pWatcher) {
                SignalWatch();
            });

            m_watchers.emplace_back(pWatch);
        }
    }

    bool Skybox::Load() {
        SR_TRACY_ZONE;

        auto&& path = GetResourcePath();
        auto&& folder = SR_UTILS_NS::Path(path.GetWithoutExtension());

        SR_LOG("Skybox::Load() : loading \"" + path.ToString() + "\" skybox...");

        std::array<TextureData::Ptr, 6> sides = { };
        for (auto&& side : sides) {
            side = nullptr;
        }

        static constexpr const char* files[6] { "right", "left", "top", "bottom", "front", "back" };

        uint32_t width = 0;
        uint32_t height = 0;

        for (uint8_t i = 0; i < 6; ++i) {
            auto&& file = folder.Concat(files[i]).ConcatExt(path.GetExtension());

            TextureLoadInfo info;
            info.mips = 1;
            info.channels = 4;

            auto&& pTextureData = TextureLoader::Load(file, info);

            if (!pTextureData) {
                SR_ERROR("Skybox::Load() : failed to load skybox texture!\n\tPath: " + file.ToString());
                return false;
            }

            if (i == 0) {
                width = pTextureData->GetWidth();
                height = pTextureData->GetHeight();
            }
            else if (pTextureData->GetWidth() != width || pTextureData->GetHeight() != height) {
                SR_WARN("Skybox::Load() : \"" + path.ToString() + "\" skybox has different sizes!");
            }

            sides[i] = pTextureData;
        }

        m_width = width;
        m_height = height;
        m_data = sides;

        return IResource::Load();
    }

    bool Skybox::Unload() {
        for (auto&& img : m_data) {
            img.Reset();
        }

        return IResource::Unload();
    }
}