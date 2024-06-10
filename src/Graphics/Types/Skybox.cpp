//
// Created by Nikita on 20.11.2020.
//

#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/FileWatcher.h>
#include <Utils/Common/StringUtils.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/Vertices.h>

#include <Graphics/Types/Skybox.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Vertices.h>
#include <Graphics/Loaders/ObjLoader.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GTYPES_NS {
    Skybox::Skybox()
        : IResource(SR_COMPILE_TIME_CRC32_TYPE_NAME(Skybox))
        , m_uboManager(Memory::UBOManager::Instance())
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

    Skybox* Skybox::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_GLOBAL_LOCK;
        SR_TRACY_ZONE;

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

        if (auto&& pResource = SR_UTILS_NS::ResourceManager::Instance().Find<Skybox>(path)) {
            return pResource;
        }

        auto&& folder = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path.GetWithoutExtension());

        SR_LOG("Skybox::Load() : loading \"" + path.ToString() + "\" skybox...");

        std::array<TextureData::Ptr, 6> sides = { };
        for (auto&& side : sides) {
            side = nullptr;
        }

        static constexpr const char* files[6] { "right", "left", "top", "bottom", "front", "back" };

        int32_t W = 0, H = 0, C = 0;

        for (uint8_t i = 0; i < 6; ++i) {
            auto&& file = folder.Concat(files[i]).ConcatExt(path.GetExtension());
            auto&& pTextureData = TextureLoader::Load(file);

            if (!pTextureData) {
                SR_ERROR("Skybox::Load() : failed to load skybox texture!\n\tPath: " + file.ToString());
                return nullptr;
            }

            if (i == 0) {
                W = pTextureData->GetWidth();
                H = pTextureData->GetHeight();
                C = pTextureData->GetChannels();
            }
            else if (pTextureData->GetWidth() != W || pTextureData->GetHeight() != H || C != pTextureData->GetChannels()) {
                SR_WARN("Skybox::Load() : \"" + path.ToString() + "\" skybox has different sizes!");
            }

            sides[i] = pTextureData;
        }

        auto&& pSkybox = new Skybox();

        pSkybox->m_width = W;
        pSkybox->m_height = H;
        pSkybox->m_data = sides;

        pSkybox->SetId(path.ToString());

        return pSkybox;
    }

    bool Skybox::Calculate() {
        if (m_isCalculated) {
            SR_ERROR("Skybox::Calculate() : the skybox is already calculated!");
            return false;
        }

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

        if (m_cubeMap = m_pipeline->AllocateCubeMap(createInfo); m_cubeMap < 0) {
            SR_ERROR("Skybox::Calculate() : failed to calculate cube map!");
            m_hasErrors = true;
            return false;
        }

        auto&& indexedVertices = Vertices::CastVertices<Vertices::SimpleVertex>(SR_UTILS_NS::SKYBOX_INDEXED_VERTICES);

        if (m_pipeline->GetType() == PipelineType::Vulkan) {
            auto&& indices = SR_UTILS_NS::SKYBOX_INDICES;

            if (m_VBO = m_pipeline->AllocateVBO(indexedVertices.data(), Vertices::VertexType::SimpleVertex, indexedVertices.size()); m_VBO == SR_ID_INVALID) {
                SR_ERROR("Skybox::Calculate() : failed to calculate VBO!");
                m_hasErrors = true;
                return false;
            }

            if (m_IBO = m_pipeline->AllocateIBO((void *) indices.data(), sizeof(uint32_t), indices.size(), SR_ID_INVALID); m_IBO == SR_ID_INVALID) {
                SR_ERROR("Skybox::Calculate() : failed to calculate IBO!");
                m_hasErrors = true;
                return false;
            }
        }
        else {
            auto&& vertices = SR_UTILS_NS::IndexedVerticesToNonIndexed(indexedVertices, SR_UTILS_NS::SKYBOX_INDICES);

            if (m_VBO = m_pipeline->AllocateVBO(vertices.data(), Vertices::VertexType::SimpleVertex, vertices.size()); m_VBO == SR_ID_INVALID) {
                SR_ERROR("Skybox::Calculate() : failed to calculate VBO!");
                m_hasErrors = true;
                return false;
            }
        }

        m_isCalculated = true;

        return true;
    }

    void Skybox::FreeVideoMemory() {
        SR_LOG("Skybox::FreeVideoMemory() : free skybox video memory...");

        if (m_VBO != SR_ID_INVALID && !m_pipeline->FreeVBO(&m_VBO)) {
            SR_ERROR("Skybox::FreeVideoMemory() : failed to free VBO!");
        }

        if (m_IBO != SR_ID_INVALID && !m_pipeline->FreeIBO(&m_IBO)) {
            SR_ERROR("Skybox::FreeVideoMemory() : failed to free IBO!");
        }

        if (m_cubeMap != SR_ID_INVALID && !m_pipeline->FreeCubeMap(&m_cubeMap)) {
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

        IGraphicsResource::FreeVideoMemory();
    }

    void Skybox::Draw() {
        SR_TRACY_ZONE;

        if (!m_isCalculated && (m_hasErrors || !Calculate())) {
            return;
        }

        auto&& uboManager = Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        if (GetPipeline()->GetCurrentBuildIteration() == 0) {
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyShader) SR_UNLIKELY_ATTRIBUTE {
                m_shader->SetSamplerCube(SHADER_SKYBOX_DIFFUSE, m_cubeMap);
                m_descriptorManager.Flush();
            }
        }

        m_pipeline->BindVBO(m_VBO);
        m_pipeline->BindIBO(m_IBO);

        if (result != DescriptorManager::BindResult::Failed) {
            m_pipeline->DrawIndices(36);
        }

        m_dirtyShader = false;
    }

    void Skybox::OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) {
        if (dynamic_cast<Shader*>(pContainer) == m_shader && m_shader) {
            m_dirtyShader = true;
            m_hasErrors = false;
        }

        IResource::OnResourceUpdated(pContainer, depth);
    }

    void Skybox::SetShader(Shader *shader) {
        if (m_shader == shader) {
            return;
        }

        m_dirtyShader = true;

        if (m_shader) {
            RemoveDependency(m_shader);
            m_shader = nullptr;
        }

        if (!(m_shader = shader)) {
            return;
        }

        AddDependency(m_shader);
    }

    int32_t Skybox::GetVBO() {
        if (!m_isCalculated && (m_hasErrors || !Calculate())) {
            return SR_ID_INVALID;
        }

        return m_VBO;
    }

    int32_t Skybox::GetIBO() {
        if (!m_isCalculated && (m_hasErrors || !Calculate())) {
            return SR_ID_INVALID;
        }

        return m_IBO;
    }

    int32_t Skybox::GetVirtualUBO() const {
        return m_virtualUBO;
    }

    void Skybox::StartWatch() {
        auto&& resourcesManager = SR_UTILS_NS::ResourceManager::Instance();

        for (auto&& pTextureData : m_data) {
            if (!pTextureData) {
                continue;
            }

            auto&& pWatch = resourcesManager.StartWatch(pTextureData->GetPath());

            pWatch->SetCallBack([this](auto &&pWatcher) {
                SignalWatch();
            });

            m_watchers.emplace_back(pWatch);
        }
    }
}