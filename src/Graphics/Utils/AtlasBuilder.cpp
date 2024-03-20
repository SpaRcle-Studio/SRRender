#include <Graphics/Utils/AtlasBuilder.h>
#include <Graphics/Loaders/TextureLoader.h>


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
        }

        m_isCached = true;
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


        bool isSpritesSameSize = true;
        for (auto&& sprite : spritesList) {
            auto referenceWidth = spritesList.front()->GetWidth();
            auto referenceHeight = spritesList.front()->GetHeight();

            if (sprite->GetWidth() == referenceWidth && sprite->GetHeight() == referenceHeight) {
                isSpritesSameSize = false;
            }
        }

        // Choose a creation strategy
        if (isSpritesSameSize && spritesList.size() % 2 == 0) {

        }
    }

    AtlasCreateStrategy::AtlasCreateStrategy(const std::list<SpaRcle::Graphics::TextureData::Ptr>& sprites, uint32_t totalWidth, uint32_t totalHeight)
        : m_sprites(sprites)
        , m_totalWidth(totalWidth)
        , m_totalHeight(totalHeight)
    { }

    AtlasCreateStrategy::AtlasPtr AtlasCreateStrategy::Create(AtlasCreateStrategy::CreationStategies strategy) {
        switch(strategy) {
            case Square: return CreateSquare();
            case TightSquare: return CreateTightSquare();
            case Line: return CreateLine();
            default: SRHalt("AtlasCreateStrategy::Create() : Bad strategy received(unknown flag)."); return nullptr;
        }
    }

    AtlasCreateStrategy::AtlasPtr AtlasCreateStrategy::CreateSquare() {
        return nullptr;
    }

    AtlasCreateStrategy::AtlasPtr AtlasCreateStrategy::CreateTightSquare() {
        return nullptr;
    }

    AtlasCreateStrategy::AtlasPtr AtlasCreateStrategy::CreateLine() {
        return nullptr;
    }
}
