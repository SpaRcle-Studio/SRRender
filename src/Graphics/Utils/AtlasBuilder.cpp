//
// Created by qulop on 17.03.2024
//
#include <Graphics/Utils/AtlasBuilder.h>
#include <Utils/Common/Hashes.h>


namespace SR_GRAPH_NS {
    AtlasBuilder::AtlasBuilder(SR_UTILS_NS::Path path, const SR_MATH_NS::USVector2& quantityOnAxes, uint32_t xStep, uint32_t yStep)
        : m_location(std::move(path))
        , m_quantityOnAxes(quantityOnAxes)
        , m_xStep(xStep)
        , m_yStep(yStep)
    { }

    AtlasBuilder::AtlasBuilder(const SR_UTILS_NS::Path& folder) {
        if (!folder.Exists()) {
            SR_ERROR("AtlasBuilder::AtlasBuilder() : The specified path does not exist: \"" + folder.ToString() + "\"");
            return;
        }
        m_isCached = true;

        std::list<SR_GRAPH_NS::TextureData::Ptr> spritesList;
        uint32_t totalWidth = 0;
        uint32_t totalHeight = 0;

        for (auto&& sprite : folder.GetFiles()) {
            spritesList.emplace_back(SR_GRAPH_NS::TextureData::Load(sprite));

            totalWidth += spritesList.back()->GetWidth();
            totalHeight += spritesList.back()->GetHeight();
        }

        AtlasCreationStrategy strategy(spritesList, totalWidth, totalHeight);
        auto&& atlas = strategy.ChooseAndCreate(folder);
        if (!atlas.has_value()) return;

        Copy(atlas.value());
    }

    void AtlasBuilder::Copy(const AtlasBuilder& other) {
        m_location = other.m_location;
        m_quantityOnAxes = other.m_quantityOnAxes;
        m_xStep = other.m_xStep;
        m_yStep = other.m_yStep;
    }

    bool AtlasBuilder::Generate(const SR_UTILS_NS::Path& path) const {  // NOLINT
        auto&& document = SR_XML_NS::Document::New();

        auto&& rootNode = document.Root().AppendNode(path.ToString());

        rootNode.AppendChild("Path").AppendAttribute("Value", m_location.ToString());
        rootNode.AppendChild("IsCached").AppendAttribute("Value", m_isCached);
        rootNode.AppendChild("QuantityOnX").AppendAttribute("Value",m_quantityOnAxes.x);
        rootNode.AppendChild("QuantityOnY").AppendAttribute("Value",m_quantityOnAxes.y);
        rootNode.AppendChild("StepInWidth").AppendAttribute("Value",m_xStep);
        rootNode.AppendChild("StepInHeight").AppendAttribute("Value",m_yStep);

        document.Save(path);
        return true;
    }

    AtlasCreationStrategy::AtlasCreationStrategy(const std::list<AtlasCreationStrategy::AtlasPtr>& sprites, uint32_t totalWidth, uint32_t totalHeight)
        : m_sprites(sprites)
        , m_totalWidth(totalWidth)
        , m_totalHeight(totalHeight)
    { }

    std::optional<AtlasBuilder> AtlasCreationStrategy::ChooseAndCreate(const SR_UTILS_NS::Path& destFolder) const {
        if (!m_destination.Copy(destFolder))
            return std::nullopt;

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
        // if (isSpritesSameSize && isDividedBy2)
        //    return CreateRectangleAtlas();
        if (!isSpritesSameSize && isDividedBy2)
            return CreateTightRectangleAtlas();
        // return CreateLineAtlas();

        return std::nullopt;
    }

    SR_UTILS_NS::Path AtlasCreationStrategy::CreateTargePath(AtlasCreationStrategy::AtlasPtr atlas) const {
        auto&& fileName = SR_UTILS_NS::sha256<SR_HTYPES_NS::HashT<16>>(atlas->GetData(), atlas->GetNumberOfBytes()).to_string();
        auto&& targetPath = m_destination.Concat(fileName);

        if (!atlas->Save(targetPath)) {
            return {};
        }

        return targetPath;
    }

//    AtlasCreationStrategy::AtlasPtr AtlasCreationStrategy::CreateRectangleAtlas() {
//        return nullptr;
//    }

    std::optional<AtlasBuilder> AtlasCreationStrategy::CreateTightRectangleAtlas() const {
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

        auto&& pTextureData = TextureData::Create(m_totalWidth, m_totalHeight, pGeneralData, [](uint8_t* pData) {
            delete[] pData;
        });

        auto&& targetPath = CreateTargePath(pTextureData);
        if (targetPath.IsEmpty()) {
            SR_ERROR("AtlasCreationStrategy::CreateTightSquare() : failed to save data on disk.");
            return std::nullopt;
        }

        return std::make_optional<AtlasBuilder>(targetPath, SR_MATH_NS::USVector2{},  0, 0);
    }


//    AtlasCreationStrategy::AtlasPtr AtlasCreationStrategy::CreateLineAtlas() {
//        return nullptr;
//    }
}
