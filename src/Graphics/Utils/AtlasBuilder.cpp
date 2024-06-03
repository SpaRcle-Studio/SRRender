//
// Created by qulop on 17.03.2024
// Fixed and refactored by innerviewer on 2024-06-02.
//

#include <Graphics/Utils/AtlasBuilder.h>
#include <Utils/Common/Hashes.h>

namespace SR_GRAPH_NS {
    AtlasBuilder::AtlasBuilder(AtlasBuilderData data)
        : m_data(std::move(data))
    { }

    bool AtlasBuilder::Generate() {
        if (!m_data.location.Exists()) {
            SR_ERROR("AtlasBuilder::Generate() : The sprite location does not exist: \"" + m_data.location.ToString() + "\"");
            return false;
        }

        if (m_data.saveInCache) {
            auto&& cacheDir = SR_UTILS_NS::ResourceManager::Instance().GetCachePath().GetFolder();

            if (!m_data.destination.empty()) {
                m_data.destination = cacheDir.Concat(m_data.destination.GetFolder());
            }
            else {
                m_data.destination = cacheDir.Concat("AtlasBuilder/");
            }
        }
        else {
            if (m_data.destination.empty()) {
                m_data.destination = m_data.location;
            }
            else {
                if (!m_data.destination.CreateIfNotExists()) {
                    m_data.destination = m_data.location;
                }
            }
        }

        std::vector<SR_GRAPH_NS::TextureData::Ptr> spriteList;

        auto&& files = m_data.location.GetFiles();

        if (files.empty()) {
            SR_ERROR("AtlasBuilder::Generate() : specified path does not contain any files. \nPath: \"" + m_data.location.ToString() + "\"");
            return false;
        }

        m_sprites.reserve(files.size());

        for (auto&& filePath : files) {
            auto&& sprite = SR_GRAPH_NS::TextureData::Load(filePath);

            if (!sprite) {
                continue;
            }

            m_totalSize.x += sprite->GetWidth();
            m_totalSize.y += sprite->GetHeight();

            m_sprites.emplace_back(sprite);
        }

        if (m_sprites.empty()) {
            SR_ERROR("AtlasBuilder::Generate() : specified path does not contain any sprites. \nPath: \"" + m_data.location.ToString() + "\"");
            return false;
        }

        return Create();
    }

    bool AtlasBuilder::Save(const SR_UTILS_NS::Path& path) const {
        auto&& document = SR_XML_NS::Document::New();

        auto&& rootNode = document.Root().AppendNode(path.ToString());

        rootNode.AppendChild("SpritePath").AppendAttribute("Value", m_data.location.ToString());
        rootNode.AppendChild("AtlasPath").AppendAttribute("Value", m_data.destination.ToString());
        rootNode.AppendChild("IsSavedInCache").AppendAttribute("Value", m_data.saveInCache);
        rootNode.AppendChild("QuantityOnX").AppendAttribute("Value", m_data.quantityOnAxes.x);
        rootNode.AppendChild("QuantityOnY").AppendAttribute("Value", m_data.quantityOnAxes.y);
        rootNode.AppendChild("WidthStep").AppendAttribute("Value", m_data.step.x);
        rootNode.AppendChild("HeightStep").AppendAttribute("Value", m_data.step.y);

        return document.Save(path);
    }

    bool AtlasBuilder::Create() {
        bool isSpritesSameSize = true;

        auto&& referenceSize = m_sprites.front()->GetSize();
        for (auto&& pSprite : m_sprites) {
            if (pSprite->GetSize() != referenceSize) {
                isSpritesSameSize = false;
                break;
            }
        }

        bool isEvenSpriteCount = m_sprites.size() % 2 == 0;

        if (isEvenSpriteCount) {
            if (isSpritesSameSize) {
                return CreateRectangleAtlas();
            }

            return CreateTightRectangleAtlas();
        }

        SR_WARN("AtlasBuilder::Create() : no suitable strategy found.");
        return false;
    }

    bool AtlasBuilder::CreateTightRectangleAtlas() {
        uint32_t totalBytes = 0;
        for (auto&& pSprite : m_sprites) {
            totalBytes += pSprite->GetNumberOfBytes();
        }

        auto* pGeneralData = new uint8_t[totalBytes];
        std::memset(pGeneralData, 0, totalBytes);

        uint32_t offset = 0;
        for (auto&& pSprite : m_sprites) {
            std::memcpy(pGeneralData + offset, pSprite->GetData(), pSprite->GetNumberOfBytes());

            offset += pSprite->GetNumberOfBytes();
        }

        m_atlas = TextureData::Create(m_totalSize.x, m_totalSize.y, pGeneralData, [](uint8_t* pData) {
            delete[] pData;
        });

        auto&& saveResult = SaveAtlas();
        if (!saveResult) {
            SR_ERROR("AtlasCreationStrategy::CreateTightRectangleAtlas() : failed to save data on disk.");
            return false;
        }

        return true;
    }

    bool AtlasBuilder::CreateRectangleAtlas() {
        uint32_t totalBytes = 0;
        for (auto&& pSprite : m_sprites) {
            totalBytes += pSprite->GetNumberOfBytes();
        }

        auto* pGeneralData = new uint8_t[totalBytes];
        std::memset(pGeneralData, 0, totalBytes);

        uint32_t offset = 0;
        for (auto&& pSprite : m_sprites) {
            std::memcpy(pGeneralData + offset, pSprite->GetData(), pSprite->GetNumberOfBytes());

            offset += pSprite->GetNumberOfBytes();
        }

        m_atlas = TextureData::Create(m_totalSize.x, m_totalSize.y, pGeneralData, [](uint8_t* pData) {
            delete[] pData;
        });

        auto&& saveResult = SaveAtlas();
        if (!saveResult) {
            SR_ERROR("AtlasCreationStrategy::CreateRectangleAtlas() : failed to save data on disk.");
            return false;
        }

        return true;
    }

    bool AtlasBuilder::SaveAtlas() const {
        if (!m_atlas) {
            SR_WARN("AtlasBuilder::SaveAtlas() : atlas is not yet created!");
            return false;
        }

        /// TODO: Implement ability to save in different formats.
        std::string filename = "testPiski";
        auto&& targetPath = m_data.destination.GetFolder().Concat(filename).ConcatExt("png");

        targetPath = "/home/nrv/atlasPIZDI.png";
        return m_atlas->Save(targetPath);
    }
}
