#include <Graphics/Utils/AtlasBuilder.h>


namespace SR_GRAPH_NS {
// Public
    AtlasBuilder::AtlasBuilder(const SR_UTILS_NS::Path& path, const SR_MATH_NS::USVector2& quantityOnAxes, uint32_t xStep, uint32_t yStep) // NOLINT
        : m_location(path)
        , m_isCached(false)
        , m_quantityOnAxes(quantityOnAxes)
        , m_xStep(xStep)
        , m_yStep(yStep)
    {}

    AtlasBuilder::AtlasBuilder(const SR_UTILS_NS::Path& folder) {
        if (!CreateAtlas(folder)) {
            SR_ERROR("AtlasBuilder::AtlasBuilder: Failed to create Atlas by path " + folder.ToString());
        }

        m_isCached = true;
    }

    SR_UTILS_NS::Path AtlasBuilder::GetLocation() const {
        return m_location;
    }

    SR_NODISCARD bool AtlasBuilder::IsCached() const {
        return m_isCached;
    }

    SR_MATH_NS::USVector2 AtlasBuilder::GetQuantity() const {
        return m_quantityOnAxes;
    }

    uint32_t AtlasBuilder::GetXStep() const {
        return m_xStep;
    }

    uint32_t AtlasBuilder::GetYStep() const {
        return m_yStep;
    }

    bool AtlasBuilder::Generate(const SR_UTILS_NS::Path& path) const {  // NOLINT
        auto&& rootNodeName = std::format("AtlasOf{}", path.GetBaseName());
        auto&& document = SR_XML_NS::Document::New();

        auto&& rootNode = document.Root().AppendNode(rootNodeName);

        rootNode.AppendChild("Path").AppendAttribute("Value", m_location.ToString());
        rootNode.AppendChild("IsCached").AppendAttribute("Value", m_isCached);
        rootNode.AppendChild("QuantityOnX").AppendAttribute("Value", m_quantityOnAxes.x);
        rootNode.AppendChild("QuantityOnY").AppendAttribute("Value", m_quantityOnAxes.y);
        rootNode.AppendChild("StepInWidth").AppendAttribute("Value", m_xStep);
        rootNode.AppendChild("StepInHeight").AppendAttribute("Value", m_yStep);

        document.Save(path);
        return true;
    }

// Private
    bool AtlasBuilder::CreateAtlas(const SR_UTILS_NS::Path& folder) const { // NOLINT
        if (!folder.Exists()) {
            SR_ERROR("AtlasBuilder::CreateAtlas: The specified path does not exist: " + folder.ToString());

            return false;
        }

        std::vector<SR_UTILS_NS::Path> sprites;
        for (auto&& file : std::filesystem::directory_iterator(folder.ToString())) {
            sprites.emplace_back(file.path().string());
        }

        return true;
    }
}