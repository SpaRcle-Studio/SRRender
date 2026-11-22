//
// Created by Nikita on 12.12.2020.
//

#ifndef SR_ENGINE_TEXTUREHELPER_H
#define SR_ENGINE_TEXTUREHELPER_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/Enumerations.h>
#include <Utils/FileSystem/Path.h>

namespace SR_UTILS_NS {
    enum class TaskState : uint8_t;
}

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(ImageType, uint8_t,
        Albedo,
        Normal,
        Direction,
        Roughness,
        Metallic,
        AmbientOcclusion,
        Emissive,
        Height,
        Mask,
        SSS,
        UI,
        ORM,
        HDR,
        MSDF,
        PixelArt
    );

    SR_ENUM_NS_CLASS_T(Antialiasing, uint8_t,
        None,
        Samples2,
        Samples4,
        Samples8,
        Samples16,
        Samples32
    );

    SR_ENUM_NS_CLASS(ImageAspect,
        None, Depth, Stencil, Color, DepthStencil
    );

    SR_ENUM_NS_CLASS(AddressMode,
        Unknown,
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
        MirrorClampToEdge
    );

    SR_ENUM_NS_CLASS(ImageFormat,
        Unknown,
        None,
        Auto,

        RGBA8_UNORM,
        BGRA8_UNORM,
        RGBA16_UNORM,
        RGBA16_SFLOAT,
        B10G11R11_UFLOAT_PACK32,

        RGB8_UNORM,
        RGB8_SRGB,
        RGB16_UNORM,

        RGBA8_SRGB,

        R8_UNORM,
        R16_UNORM,
        R16_SFLOAT,

        R32_SFLOAT,
        R64_SFLOAT,

        R8_UINT,
        R16_UINT,
        R32_UINT,
        R64_UINT,

        RG8_UNORM,

        D16_UNORM,
        D24_UNORM_S8_UINT,
        D32_SFLOAT,
        D32_SFLOAT_S8_UINT
    );

    SR_GRAPHICS_DLL_API extern bool IsTextureSupportsFormat(ImageFormat format);

    SR_GRAPHICS_DLL_API extern uint8_t GetChannelCount(ImageFormat format);

    struct ColorLayer {
        SR_UTILS_NS::Vector<int32_t> texture;
        ImageFormat format = ImageFormat::Unknown;
    };

    struct DepthLayer {
        SR_UTILS_NS::Vector<int32_t> texture;
        ImageFormat format = ImageFormat::Unknown;
        ImageAspect aspect = ImageAspect::DepthStencil;
        SR_UTILS_NS::Vector<SR_UTILS_NS::Vector<int32_t>> subLayers;
    };

    SR_ENUM_NS_CLASS(TextureFilter,
        Unknown = 0, NEAREST = 1, LINEAR = 2, NEAREST_MIPMAP_NEAREST = 3,
        LINEAR_MIPMAP_NEAREST = 4, NEAREST_MIPMAP_LINEAR = 5, LINEAR_MIPMAP_LINEAR = 6
    );

    SR_ENUM_NS_CLASS(TextureCompression,
        None = 0, BC1 = 1, BC2 = 2, BC3 = 3, BC4 = 4, BC5 = 5, BC6 = 6, BC7 = 7,
        ASTC4x4, ASTC6x6, ASTC8x8, ETC2RGB, ETC2RGBA, EACR11, EACRG11
    );

    SR_ENUM_NS_CLASS(TextureCompressionPolicy, Disabled, Auto, Explicit, Lossless, HighQuality, Balanced, Aggressive);

    SR_ENUM_NS_CLASS(TextureContent, Auto, Illustration, Interface);

    SR_ENUM_NS_CLASS(TextureQualityValidation, Off, Warn, Strict);

    SR_ENUM_NS_CLASS(TextureExportProfile, Desktop, Mobile, Universal);

    enum class TextureLoadMode : uint8_t {
        STBImage, LibsPNG
    };

    struct TextureLoadInfo {
        TextureCompression compression = TextureCompression::None;
        uint32_t mips = 0;
        uint8_t channels = 0;
        TextureLoadMode loadMode = TextureLoadMode::STBImage;
        bool caching = false;
        bool disableAlphaChannel = false;
        bool preparedMipChain = false;
        bool requireAlignedBlocks = false;
        ImageFormat format = ImageFormat::RGBA8_UNORM;
        SR_UTILS_NS::Vector<SR_UTILS_NS::Path> preparedVariants;
        SR_UTILS_NS::Vector<uint64_t> preparedHashes;
        SR_UTILS_NS::Vector<uint32_t> supportedFormats;
    };

    uint32_t Find4(uint32_t i);

    SR_UTILS_NS::Pair<uint32_t, uint32_t> MakeGoodSizes(uint32_t w, uint32_t h);

    uint8_t* ResizeToLess(uint32_t ow, uint32_t oh, uint32_t nw, uint32_t nh, const uint8_t* pixels);

    uint32_t GetPixelSize(ImageFormat format);

    uint64_t GetCompressedImageSize(uint32_t w, uint32_t h, const TextureLoadInfo& info);
}

#endif //SR_ENGINE_TEXTUREHELPER_H
