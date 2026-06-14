//
// Created by Monika on 13.06.2026.
//

#include <Graphics/Types/Atlas.h>

#include <Codegen/Atlas.generated.hpp>

namespace SR_GRAPH_NS {
    const AtlasEntry* Atlas::GetTexture(const SR_UTILS_NS::StringAtom path) const {
        if (auto&& pIt = m_entries.find(path); pIt != m_entries.end()) {
            return &pIt->second;
        }
        return nullptr;
    }
}
