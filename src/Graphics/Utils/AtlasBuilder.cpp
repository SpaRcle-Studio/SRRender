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
        if (m_data.source.IsAbs() || m_data.destination.IsAbs()) {
            SR_ERROR("AtlasBuilder::Generate() : paths must be relative.");
            return false;
        }

        auto&& resourceManager = SR_UTILS_NS::ResourceManager::Instance();
        auto&& resourcePath = resourceManager.GetResPath();

        m_data.source = m_data.source.RemoveSubPath(resourceManager.GetResPathRef());

        if (m_data.saveInCache) {
            auto&& cacheDir = resourceManager.GetCachePath();

            if (m_data.destination.empty()) {
                m_data.destination = cacheDir.Concat("AtlasBuilder").Concat(m_data.source);
            }
            else {
                m_data.destination = cacheDir.Concat("AtlasBuilder").Concat(m_data.destination);
            }
        }
        else {
            if (m_data.destination.empty()) {
                m_data.destination = resourcePath.Concat(m_data.source);
            }
            else {
                m_data.destination = resourcePath.Concat(m_data.destination);
            }
        }

        m_data.destination = m_data.destination.Concat("Atlas").ConcatExt(m_data.extension);

        std::vector<SR_GRAPH_NS::TextureData::Ptr> spriteList;

        auto&& files = resourcePath.Concat(m_data.source).GetFiles();

        if (files.empty()) {
            SR_ERROR("AtlasBuilder::Generate() : specified path does not contain any files. \nPath: \"" + m_data.source.ToString() + "\"");
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
            SR_ERROR("AtlasBuilder::Generate() : specified path does not contain any sprites. \nPath: \"" + m_data.source.ToString() + "\"");
            return false;
        }

        return Create();
    }

    bool AtlasBuilder::SaveConfig(const SR_UTILS_NS::Path& path) const {
        auto&& document = SR_XML_NS::Document::New();

        auto&& rootNode = document.Root().AppendNode("Config");

        std::string atlasPath;
        if (m_data.saveInCache) {
            atlasPath = m_data.destination.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetCachePath());
        }
        else {
            atlasPath = m_data.destination.RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPathRef());
        }

        rootNode.AppendChild("SpriteSource").AppendAttribute("Value", m_data.source.ToString());
        rootNode.AppendChild("AtlasPath").AppendAttribute("Value", atlasPath);
        rootNode.AppendChild("IsSavedInCache").AppendAttribute("Value", m_data.saveInCache);
        rootNode.AppendChild("QuantityOnX").AppendAttribute("Value", m_data.quantityOnAxes.x);
        rootNode.AppendChild("QuantityOnY").AppendAttribute("Value", m_data.quantityOnAxes.y);
        rootNode.AppendChild("WidthStep").AppendAttribute("Value", m_data.step.x);
        rootNode.AppendChild("HeightStep").AppendAttribute("Value", m_data.step.y);

        return document.Save(path.ConcatExt("xml"));
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

        const bool isSpriteCountEven = m_sprites.size() % 2 == 0;
        if (isSpriteCountEven) {
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

        if (!Save()) {
            SR_ERROR("AtlasBuilder::CreateTightRectangleAtlas() : failed to save data.");
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

        if (!Save()) {
            SR_ERROR("AtlasBuilder::CreateRectangleAtlas() : failed to save data.");
            return false;
        }

        return true;
    }

    bool AtlasBuilder::Save() const {
        if (!m_atlas) {
            SR_ERROR("AtlasBuilder::Save() : atlas is not created!");
            return false;
        }

        if (!m_atlas->Save(m_data.destination)) {
            SR_ERROR("AtlasBuilder::Save() : failed to save atlas to file!");
            return false;
        }

        if (!SaveConfig(m_data.destination)) {
            SR_ERROR("AtlasBuilder::Save() : failed to save config to file!");
            return false;
        }

        return true;
    }
}
