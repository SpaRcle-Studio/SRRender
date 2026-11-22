//
// Created by Monika on 08.12.2025.
//

#include <Graphics/Utils/ImageMetaInfo.h>

#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/ImageMetaInfo.generated.hpp>

namespace SR_GRAPH_NS {
    bool ImageMetaInfo::operator==(const ImageMetaInfo& lrs) const {
        return format == lrs.format
           && loadMode == lrs.loadMode
           && imageType == lrs.imageType
           && filter == lrs.filter
           && addressMode == lrs.addressMode
           && compression == lrs.compression
           && compressionPolicy == lrs.compressionPolicy
           && content == lrs.content
           && qualityValidation == lrs.qualityValidation
           && SR_EQUALS(compressionQuality, lrs.compressionQuality)
           && SR_EQUALS(alphaCutoff, lrs.alphaCutoff)
           && preserveAlphaCoverage == lrs.preserveAlphaCoverage
           && compressed == lrs.compressed
           && preparedVariants == lrs.preparedVariants
           && preparedHashes == lrs.preparedHashes
           && mipLevels == lrs.mipLevels
           && m_border == lrs.m_border
           && SR_EQUALS_T(m_pixelsPerUnit, lrs.m_pixelsPerUnit, SR_SCALAR_EPSILON)
           && cpuUsage == lrs.cpuUsage
           && disableAlphaChannel == lrs.disableAlphaChannel;
    }
}
