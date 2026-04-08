//
// Created by Monika on 07.04.2026.
//

#include <Graphics/Types/Vertices.h>

namespace SR_GRAPH_NS::Vertices {
    const SR_UTILS_NS::VertexLayoutDescription StaticMeshVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::UV0, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Normal, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Tangent, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
    ;

    const SR_UTILS_NS::VertexLayoutDescription WireframeMeshVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
    ;

    const SR_UTILS_NS::VertexLayoutDescription SimpleMeshVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
    ;

    const SR_UTILS_NS::VertexLayoutDescription SkyboxVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
    ;

    const SR_UTILS_NS::VertexLayoutDescription SkinnedMeshVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::UV0, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Normal, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Tangent, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::BlendIndices, SR_UTILS_NS::VertexAttributeFormat::UInt32, 4)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::BlendWeights, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::BlendIndices2, SR_UTILS_NS::VertexAttributeFormat::UInt32, 4)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::BlendWeights2, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
    ;
}