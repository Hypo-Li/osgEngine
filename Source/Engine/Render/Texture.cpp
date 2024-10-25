#include <Engine/Render/Texture.h>

namespace xxx
{
    Texture::Texture(const TextureImportOptions& options)
    {
        mFormat = options.format;
        mMinFilter = options.minFilter;
        mMagFilter = options.magFilter;
        mWrapR = options.wrapR;
        mWrapS = options.wrapS;
        mWrapT = options.wrapT;
        mBorderColor = options.borderColor;
        mMipmapGeneration = options.mipmapGeneration;
    }

    std::pair<Texture::PixelFormat, Texture::PixelType> Texture::getPixelFormatAndTypeFromFormat(Format format)
    {
        switch (format)
        {
        case Format::R8:
            return { PixelFormat::R, PixelType::Unsigned_Byte };
        case Format::RG8:
            return { PixelFormat::RG, PixelType::Unsigned_Byte };
        case Format::RGB8:
            return { PixelFormat::RGB, PixelType::Unsigned_Byte };
        case Format::RGBA8:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        case Format::R16:
            return { PixelFormat::R, PixelType::Unsigned_Short };
        case Format::RG16:
            return { PixelFormat::RG, PixelType::Unsigned_Short };
        case Format::RGB16:
            return { PixelFormat::RGB, PixelType::Unsigned_Short };
        case Format::RGBA16:
            return { PixelFormat::RGBA, PixelType::Unsigned_Short };
        case Format::R8S:
            return { PixelFormat::R, PixelType::Byte };
        case Format::RG8S:
            return { PixelFormat::RG, PixelType::Byte };
        case Format::RGB8S:
            return { PixelFormat::RGB, PixelType::Byte };
        case Format::RGBA8S:
            return { PixelFormat::RGBA, PixelType::Byte };
        case Format::R16S:
            return { PixelFormat::R, PixelType::Short };
        case Format::RG16S:
            return { PixelFormat::RG, PixelType::Short };
        case Format::RGB16S:
            return { PixelFormat::RGB, PixelType::Short };
        case Format::RGBA16S:
            return { PixelFormat::RGBA, PixelType::Short };
        case Format::R8I:
            return { PixelFormat::R, PixelType::Byte };
        case Format::RG8I:
            return { PixelFormat::RG, PixelType::Byte };
        case Format::RGB8I:
            return { PixelFormat::RGB, PixelType::Byte };
        case Format::RGBA8I:
            return { PixelFormat::RGBA, PixelType::Byte };
        case Format::R16I:
            return { PixelFormat::R, PixelType::Short };
        case Format::RG16I:
            return { PixelFormat::RG, PixelType::Short };
        case Format::RGB16I:
            return { PixelFormat::RGB, PixelType::Short };
        case Format::RGBA16I:
            return { PixelFormat::RGBA, PixelType::Short };
        case Format::R32I:
            return { PixelFormat::R, PixelType::Int };
        case Format::RG32I:
            return { PixelFormat::RG, PixelType::Int };
        case Format::RGB32I:
            return { PixelFormat::RGB, PixelType::Int };
        case Format::RGBA32I:
            return { PixelFormat::RGBA, PixelType::Int };
        case Format::R8UI:
            return { PixelFormat::R, PixelType::Unsigned_Byte };
        case Format::RG8UI:
            return { PixelFormat::RG, PixelType::Unsigned_Byte };
        case Format::RGB8UI:
            return { PixelFormat::RGB, PixelType::Unsigned_Byte };
        case Format::RGBA8UI:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        case Format::R16UI:
            return { PixelFormat::R, PixelType::Unsigned_Short };
        case Format::RG16UI:
            return { PixelFormat::RG, PixelType::Unsigned_Short };
        case Format::RGB16UI:
            return { PixelFormat::RGB, PixelType::Unsigned_Short };
        case Format::RGBA16UI:
            return { PixelFormat::RGBA, PixelType::Unsigned_Short };
        case Format::R32UI:
            return { PixelFormat::R, PixelType::Unsigned_Int };
        case Format::RG32UI:
            return { PixelFormat::RG, PixelType::Unsigned_Int };
        case Format::RGB32UI:
            return { PixelFormat::RGB, PixelType::Unsigned_Int };
        case Format::RGBA32UI:
            return { PixelFormat::RGBA, PixelType::Unsigned_Int };
        case Format::R16F:
            return { PixelFormat::R, PixelType::Half };
        case Format::RG16F:
            return { PixelFormat::RG, PixelType::Half };
        case Format::RGB16F:
            return { PixelFormat::RGB, PixelType::Half };
        case Format::RGBA16F:
            return { PixelFormat::RGBA, PixelType::Half };
        case Format::R32F:
            return { PixelFormat::R, PixelType::Float };
        case Format::RG32F:
            return { PixelFormat::RG, PixelType::Float };
        case Format::RGB32F:
            return { PixelFormat::RGB, PixelType::Float };
        case Format::RGBA32F:
            return { PixelFormat::RGBA, PixelType::Float };
        case Format::SRGB8:
            return { PixelFormat::RGB, PixelType::Unsigned_Byte };
        case Format::SRGB8_Alpha8:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        /*case Format::Compressed_Red_RGTC1:
            break;
        case Format::Compressed_Signed_Red_RGTC1:
            break;
        case Format::Compressed_RG_RGTC2:
            break;
        case Format::Compressed_Signed_RG_RGTC2:
            break;
        case Format::Compressed_RGBA_BPTC_Unorm:
            break;
        case Format::Compressed_SRGB_Alpha_BPTC_Unorm:
            break;
        case Format::Compressed_RGB_BPTC_Signed_Float:
            break;
        case Format::Compressed_RGB_BPTC_Unsigned_Float:
            break;
        case Format::Compressed_RGBA_ASTC_4x4:
            break;
        case Format::Compressed_RGBA_ASTC_5x4:
            break;
        case Format::Compressed_RGBA_ASTC_5x5:
            break;
        case Format::Compressed_RGBA_ASTC_6x5:
            break;
        case Format::Compressed_RGBA_ASTC_6x6:
            break;
        case Format::Compressed_RGBA_ASTC_8x5:
            break;
        case Format::Compressed_RGBA_ASTC_8x6:
            break;
        case Format::Compressed_RGBA_ASTC_10x5:
            break;
        case Format::Compressed_RGBA_ASTC_10x6:
            break;
        case Format::Compressed_RGBA_ASTC_8x8:
            break;
        case Format::Compressed_RGBA_ASTC_10x8:
            break;
        case Format::Compressed_RGBA_ASTC_10x10:
            break;
        case Format::Compressed_RGBA_ASTC_12x10:
            break;
        case Format::Compressed_RGBA_ASTC_12x12:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_4x4:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_5x4:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_5x5:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_6x5:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_6x6:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_8x5:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_8x6:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_10x5:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_10x6:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_8x8:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_10x8:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_10x10:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_12x10:
            break;
        case Format::Compressed_SRGB8_Alpha8_ASTC_12x12:
            break;
        */
        case Format::Compressed_RGB_S3TC_DXT1:
            return { PixelFormat::RGB, PixelType::Unsigned_Byte };
        case Format::Compressed_RGBA_S3TC_DXT1:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        case Format::Compressed_RGBA_S3TC_DXT3:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        case Format::Compressed_RGBA_S3TC_DXT5:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        case Format::Compressed_RGBA_BPTC_Unorm:
            return { PixelFormat::RGBA, PixelType::Unsigned_Byte };
        /*
        case Format::Compressed_SRGB_S3TC_DXT1:
            break;
        case Format::Compressed_SRGB_Alpha_S3TC_DXT1:
            break;
        case Format::Compressed_SRGB_Alpha_S3TC_DXT3:
            break;
        case Format::Compressed_SRGB_Alpha_S3TC_DXT5:
            break;
        */
        default:
            return { PixelFormat::R, PixelType::Unsigned_Byte };
        }
    }

    std::string Texture::getImageFormatName(Format format)
    {
        switch (format)
        {
        case Format::RGBA32F:
            return "rgba32f";
        case Format::RGBA16F:
            return "rgba16f";
        case Format::RG32F:
            return "rg32f";
        case Format::RG16F:
            return "rg16f";
        case Format::R32F:
            return "r32f";
        case Format::R16F:
            return "r16f";
        default:
            return "";
        }
    }
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Texture::Format>()
    {
        Enum* enumerate = new EnumInstance<Texture::Format>("Texture::Format", {
            {"R8", Texture::Format::R8},
            {"RG8", Texture::Format::RG8},
            {"RGB8", Texture::Format::RGB8},
            {"RGBA8", Texture::Format::RGBA8},

            {"R16", Texture::Format::R16},
            {"RG16", Texture::Format::RG16},
            {"RGB16", Texture::Format::RGB16},
            {"RGBA16", Texture::Format::RGBA16},

            {"R8S", Texture::Format::R8S},
            {"RG8S", Texture::Format::RG8S},
            {"RGB8S", Texture::Format::RGB8S},
            {"RGBA8S", Texture::Format::RGBA8S},

            {"R16S", Texture::Format::R16S},
            {"RG16S", Texture::Format::RG16S},
            {"RGB16S", Texture::Format::RGB16S},
            {"RGBA16S", Texture::Format::RGBA16S},

            {"R8I", Texture::Format::R8I},
            {"RG8I", Texture::Format::RG8I},
            {"RGB8I", Texture::Format::RGB8I},
            {"RGBA8I", Texture::Format::RGBA8I},

            {"R16I", Texture::Format::R16I},
            {"RG16I", Texture::Format::RG16I},
            {"RGB16I", Texture::Format::RGB16I},
            {"RGBA16I", Texture::Format::RGBA16I},

            {"R32I", Texture::Format::R32I},
            {"RG32I", Texture::Format::RG32I},
            {"RGB32I", Texture::Format::RGB32I},
            {"RGBA32I", Texture::Format::RGBA32I},

            {"R8UI", Texture::Format::R8UI},
            {"RG8UI", Texture::Format::RG8UI},
            {"RGB8UI", Texture::Format::RGB8UI},
            {"RGBA8UI", Texture::Format::RGBA8UI},

            {"R16UI", Texture::Format::R16UI},
            {"RG16UI", Texture::Format::RG16UI},
            {"RGB16UI", Texture::Format::RGB16UI},
            {"RGBA16UI", Texture::Format::RGBA16UI},

            {"R32UI", Texture::Format::R32UI},
            {"RG32UI", Texture::Format::RG32UI},
            {"RGB32UI", Texture::Format::RGB32UI},
            {"RGBA32UI", Texture::Format::RGBA32UI},

            {"R16F", Texture::Format::R16F},
            {"RG16F", Texture::Format::RG16F},
            {"RGB16F", Texture::Format::RGB16F},
            {"RGBA16F", Texture::Format::RGBA16F},

            {"R32F", Texture::Format::R32F},
            {"RG32F", Texture::Format::RG32F},
            {"RGB32F", Texture::Format::RGB32F},
            {"RGBA32F", Texture::Format::RGBA32F},

            {"SRGB8", Texture::Format::SRGB8},
            {"SRGB8_Alpha8", Texture::Format::SRGB8_Alpha8},

            { "Compressed_RGB_S3TC_DXT1", Texture::Format::Compressed_RGB_S3TC_DXT1},
            { "Compressed_RGBA_S3TC_DXT1", Texture::Format::Compressed_RGBA_S3TC_DXT1 },
            { "Compressed_RGBA_S3TC_DXT3", Texture::Format::Compressed_RGBA_S3TC_DXT3 },
            { "Compressed_RGBA_S3TC_DXT5", Texture::Format::Compressed_RGBA_S3TC_DXT5 },

            {"Compressed_RGBA_BPTC_Unorm", Texture::Format::Compressed_RGBA_BPTC_Unorm},
        });
        return enumerate;
    }

    template <> Type* Reflection::createType<Texture::PixelFormat>()
    {
        Enum* enumerate = new EnumInstance<Texture::PixelFormat>("Texture::PixelFormat", {
            {"R", Texture::PixelFormat::R},
            {"RG", Texture::PixelFormat::RG},
            {"RGB", Texture::PixelFormat::RGB},
            {"RGBA", Texture::PixelFormat::RGBA},
        });
        return enumerate;
    }

    template <> Type* Reflection::createType<Texture::PixelType>()
    {
        Enum* enumerate = new EnumInstance<Texture::PixelType>("Texture::PixelType", {
            {"Unsigned Byte", Texture::PixelType::Unsigned_Byte},
            {"Byte", Texture::Byte},
            {"Unsigned Short", Texture::PixelType::Unsigned_Short},
            {"Short", Texture::PixelType::Short},
            {"Unsigned Int", Texture::PixelType::Unsigned_Int},
            {"Int", Texture::PixelType::Int},
            {"Half", Texture::PixelType::Half},
            {"Float", Texture::PixelType::Float},
        });
        return enumerate;
    }

    template <> Type* Reflection::createType<Texture::FilterMode>()
    {
        Enum* enumerate = new EnumInstance<Texture::FilterMode>("Texture::FilterMode", {
            {"Linear", Texture::FilterMode::Linear},
            {"Nearest", Texture::FilterMode::Nearest},
            {"Linear Mipmap Linear", Texture::FilterMode::Linear_Mipmap_Linear},
            {"Linear Mipmap Nearest", Texture::FilterMode::Linear_Mipmap_Nearest},
            {"Nearest Mipmap Linear", Texture::FilterMode::Nearest_Mipmap_Linear},
            {"Nearest Mipmap Nearest", Texture::FilterMode::Nearest_Mipmap_Nearest},
        });
        return enumerate;
    }

    template <> Type* Reflection::createType<Texture::WrapMode>()
    {
        Enum* enumerate = new EnumInstance<Texture::WrapMode>("Texture::WrapMode", {
            {"Repeat", Texture::WrapMode::Repeat},
            {"Mirrored Repeat", Texture::WrapMode::Mirrored_Repeat},
            {"Clamp", Texture::WrapMode::Clamp},
            {"Clamp To Edge", Texture::WrapMode::Clamp_To_Edge},
            {"Clamp To Border", Texture::WrapMode::Clamp_To_Border},
            {"Mirror Clamp To Edge", Texture::WrapMode::Clamp_To_Edge},
        });
        return enumerate;
    }

    template <> Type* Reflection::createType<Texture>()
    {
        Class* clazz = new ClassInstance<Texture>("Texture");
        clazz->addProperty("Format", &Texture::mFormat);
        clazz->addProperty("PixelFormat", &Texture::mPixelFormat);
        clazz->addProperty("PixelType", &Texture::mPixelType);
        clazz->addProperty("MinFilter", &Texture::mMinFilter);
        clazz->addProperty("MagFilter", &Texture::mMagFilter);
        clazz->addProperty("WrapR", &Texture::mWrapR);
        clazz->addProperty("WrapS", &Texture::mWrapS);
        clazz->addProperty("WrapT", &Texture::mWrapT);
        clazz->addProperty("BorderColor", &Texture::mBorderColor);
        clazz->addProperty("MipmapGeneration", &Texture::mMipmapGeneration);
        clazz->addProperty("MipmapCount", &Texture::mMipmapCount);
        clazz->addProperty("MipmapDataOffsets", &Texture::mMipmapDataOffsets);
        clazz->addProperty("DataCompression", &Texture::mDataCompression);
        Property* propData = clazz->addProperty("Data", &Texture::mData);
        propData->addMetadata(MetaKey::Hidden, true);
        return clazz;
    }
}
