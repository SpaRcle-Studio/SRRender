//
// Created by qulop on 17.03.2024
//
#include <Graphics/Utils/AtlasBuilder.h>


namespace SR_GRAPH_NS {
    AtlasBuilder::AtlasBuilder(const SR_UTILS_NS::Path& path, const SR_MATH_NS::USVector2& quantityOnAxes, uint32_t xStep, uint32_t yStep) // NOLINT
        : m_location(path)
        , m_isCached(false)
        , m_quantityOnAxes(quantityOnAxes)
        , m_xStep(xStep)
        , m_yStep(yStep)
    {}

    AtlasBuilder::AtlasBuilder(const SR_UTILS_NS::Path& folder) {
        if (!folder.Exists()) {
            SR_ERROR("AtlasBuilder::AtlasBuilder: The specified path does not exist: \" + folder.ToString()");
            return;
        }

        if (!CreateAtlas(folder.GetFiles())) {
            SR_ERROR("AtlasBuilder::AtlasBuilder: Failed to create Atlas by path " + folder.ToString());
            return;
        }

        m_isCached = true;
    }

    AtlasBuilder& AtlasBuilder::operator=(const AtlasBuilder& other) {
        m_location = other.m_location;
        m_isCached = other.m_isCached;
        m_quantityOnAxes = other.m_quantityOnAxes;
        m_xStep = other.m_xStep;
        m_yStep = other.m_yStep;

        return *this;
    }

    bool AtlasBuilder::Generate(const SR_UTILS_NS::Path& path) const {  // NOLINT
        auto&& document = SR_XML_NS::Document::New();

        auto&& rootNode = document.Root().AppendNode(path.ToString());

        rootNode.AppendChild("Path").AppendAttribute("Value", m_location.ToString());
        rootNode.AppendChild("IsCached").AppendAttribute("Value", m_isCached);
        rootNode.AppendChild("QuantityOnX").AppendAttribute("Value", m_quantityOnAxes.x);
        rootNode.AppendChild("QuantityOnY").AppendAttribute("Value", m_quantityOnAxes.y);
        rootNode.AppendChild("StepInWidth").AppendAttribute("Value", m_xStep);
        rootNode.AppendChild("StepInHeight").AppendAttribute("Value", m_yStep);

        document.Save(path);
        return true;
    }

    bool AtlasBuilder::CreateAtlas(const std::list<SR_UTILS_NS::Path>& files) const { // NOLINT
        std::list<SR_GRAPH_NS::TextureData::Ptr> spritesList;
        uint32_t totalWidth = 0;
        uint32_t totalHeight = 0;

        for (auto&& sprite : files) {
            spritesList.emplace_back(SR_GRAPH_NS::TextureData::Load(sprite));

            totalWidth += spritesList.back()->GetWidth();
            totalHeight += spritesList.back()->GetHeight();
        }

        AtlasCreationStrategy strategy(spritesList, totalWidth, totalHeight);
        auto&& pAtlas = strategy.ChooseAndCreate();
        if (!pAtlas) {
            SR_ERROR("AtlasBuilder::CreateAtlas() : failed to create atlas.");
            return false;
        }

        return true;
    }

    AtlasCreationStrategy::AtlasCreationStrategy(const std::list<SR_GRAPH_NS::TextureData::Ptr>& sprites, uint32_t totalWidth, uint32_t totalHeight)
        : m_sprites(sprites)
        , m_totalWidth(totalWidth)
        , m_totalHeight(totalHeight)
    { }

    AtlasCreationStrategy::AtlasPtr AtlasCreationStrategy::ChooseAndCreate() {
        bool isSpritesSameSize = true;
        for (auto&& pSprite : m_sprites) {
            auto referenceWidth = m_sprites.front()->GetWidth();
            auto referenceHeight = m_sprites.front()->GetHeight();

            if (pSprite->GetWidth() != referenceWidth || pSprite->GetHeight() != referenceHeight) {
                isSpritesSameSize = false;
            }
        }
        bool isDividedBy2 = m_sprites.size() % 2 == 0;

        // Choose a creation strategy
        if (isSpritesSameSize && isDividedBy2)  return CreateSquare();
        if (!isSpritesSameSize && isDividedBy2) return CreateTightSquare();

        return CreateLine();
    }

    AtlasCreationStrategy::AtlasPtr AtlasCreationStrategy::CreateSquare() {
        return nullptr;
    }

    AtlasCreationStrategy::AtlasPtr AtlasCreationStrategy::CreateTightSquare() {
        uint32_t totalBytes = 0;
        for (auto&& sprite : m_sprites) {
            totalBytes += sprite->GetNumberOfBytes();
        }

        auto* pGeneralData = new uint8_t[totalBytes];
        std::memset(pGeneralData, 0, totalBytes);

        uint32_t offset = 0;
        for (auto&& sprite : m_sprites) {
            std::memcpy(pGeneralData + offset, sprite->GetData(), sprite->GetNumberOfBytes());

            offset += sprite->GetNumberOfBytes();
        }

        auto&& pTextureData = TextureData::Create(m_totalWidth, m_totalHeight, pGeneralData, [](uint8_t* pData) {
            delete[] pData;
        });
        
        return pTextureData;
    }

    AtlasCreationStrategy::AtlasPtr AtlasCreationStrategy::CreateLine() {
        return nullptr;
    }
}
