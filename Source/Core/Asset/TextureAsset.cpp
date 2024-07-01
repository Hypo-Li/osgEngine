#include "TextureAsset.h"
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>

namespace xxx
{

    const Asset::ConstBiMap<GLenum, std::string> TextureAsset::_sTextureTypeStringMap = {
        {GL_TEXTURE_2D, "Texture2D"},
        {GL_TEXTURE_2D_ARRAY, "Texture2DArray"},
        {GL_TEXTURE_3D, "Texture3D"},
        {GL_TEXTURE_CUBE_MAP, "TextureCubemap"}
    };

    const Asset::ConstBiMap<GLenum, std::string> TextureAsset::_sTextureFormatStringMap = {
        {GL_R8, "R8"},
        {GL_RG8, "RG8"},
        {GL_RGB8, "RGB8"},
        {GL_RGBA8, "RGBA8"},

        {GL_R16, "R16"},
        {GL_RG16, "RG16"},
        {GL_RGB16, "RGB16"},
        {GL_RGBA16, "RGBA16"},

        {GL_R8_SNORM, "R8S"},
        {GL_RG8_SNORM, "RG8S"},
        {GL_RGB8_SNORM, "RGB8S"},
        {GL_RGBA8_SNORM, "RGBA8S"},

        {GL_R16_SNORM, "R16S"},
        {GL_RG16_SNORM, "RG16S"},
        {GL_RGB16_SNORM, "RGB16S"},
        {GL_RGBA16_SNORM, "RGBA16S"},

        {GL_R16F, "R16F"},
        {GL_RG16F, "RG16F"},
        {GL_RGB16F, "RGB16F"},
        {GL_RGBA16F, "RGBA16F"},

        {GL_R32F, "R32F"},
        {GL_RG32F, "RG32F"},
        {GL_RGB32F, "RGB32F"},
        {GL_RGBA32F, "RGBA32F"},

        // Did not support integer format for now!
        // {GL_R8I, "R8I"},
        // {GL_RG8I, "RG8I"},
        // {GL_RGB8I, "RGB8I"},
        // {GL_RGBA8I, "RGBA8I"},
        // 
        // {GL_R16I, "R16I"},
        // {GL_RG16I, "RG16I"},
        // {GL_RGB16I, "RGB16I"},
        // {GL_RGBA16I, "RGBA16I"},
        // 
        // {GL_R32I, "R32I"},
        // {GL_RG32I, "RG32I"},
        // {GL_RGB32I, "RGB32I"},
        // {GL_RGBA32I, "RGBA32I"},
        // 
        // {GL_R8UI, "R8UI"},
        // {GL_RG8UI, "RG8UI"},
        // {GL_RGB8UI, "RGB8UI"},
        // {GL_RGBA8UI, "RGBA8UI"},
        // 
        // {GL_R16UI, "R16UI"},
        // {GL_RG16UI, "RG16UI"},
        // {GL_RGB16UI, "RGB16UI"},
        // {GL_RGBA16UI, "RGBA16UI"},
        // 
        // {GL_R32UI, "R32UI"},
        // {GL_RG32UI, "RG32UI"},
        // {GL_RGB32UI, "RGB32UI"},
        // {GL_RGBA32UI, "RGBA32UI"},

        {GL_RGBA2, "RGBA2"},
        {GL_RGBA4, "RGBA4"},
        {GL_RGB5, "RGB5"},
        {GL_RGB10, "RGB10"},
        {GL_RGBA12, "RGBA12"},

        {GL_R3_G3_B2, "R3G3B2"},
        {GL_RGB5_A1, "RGB5A1"},
        {GL_RGB10_A2, "RGB10A2"},
        {GL_RGB10_A2UI, "RGB10A2UI"},
        {GL_R11F_G11F_B10F, "R11FG11FB10F"},
        {GL_RGB9_E5, "RGB9E5"},
        {GL_RGB565, "RGB565"},

        {GL_SRGB8, "SRGB8"},
        {GL_SRGB8_ALPHA8, "SRGBA8"},

        {GL_COMPRESSED_RED_RGTC1, "COMP_RED_RGTC1"},
        {GL_COMPRESSED_SIGNED_RED_RGTC1, "COMP_SIGNED_RED_RGTC1"},
        {GL_COMPRESSED_RG_RGTC2, "COMP_RG_RGTC2"},
        {GL_COMPRESSED_SIGNED_RG_RGTC2, "COMP_SIGNED_RG_RGTC2"},

        {GL_COMPRESSED_RGBA_BPTC_UNORM, "COMP_RGBA_BPTC_UNORM"},
        {GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, "COMP_SRGB_ALPHA_BPTC_UNORM"},
        {GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, "COMP_RGB_BPTC_SIGNED_FLOAT"},
        {GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, "COMP_RGB_BPTC_UNSIGNED_FLOAT"},

        {GL_COMPRESSED_RGB_S3TC_DXT1_EXT, "COMP_RGB_S3TC_DXT1"},
        {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, "COMP_RGBA_S3TC_DXT1"},
        {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, "COMP_RGBA_S3TC_DXT3"},
        {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, "COMP_RGBA_S3TC_DXT5"},
        // {GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, "COMP_SRGB_S3TC_DXT1"},
        // {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, "COMP_SRGB_ALPHA_S3TC_DXT1"},
        // {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, "COMP_SRGB_ALPHA_S3TC_DXT3"},
        // {GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, "COMP_SRGB_ALPHA_S3TC_DXT5"},

        {GL_DEPTH_COMPONENT16, "D16"},
        {GL_DEPTH_COMPONENT24, "D24"},
        {GL_DEPTH_COMPONENT32, "D32"},
        {GL_DEPTH_COMPONENT32F, "D32F"},

        {GL_DEPTH24_STENCIL8, "D24S8"},
        {GL_DEPTH32F_STENCIL8, "D32FS8"},

        {GL_STENCIL_INDEX1, "S1"},
        {GL_STENCIL_INDEX4, "S4" },
        {GL_STENCIL_INDEX8, "S8" },
        {GL_STENCIL_INDEX16, "S16" },
    };

    const Asset::ConstBiMap<GLenum, std::string> TextureAsset::_sPixelFormatStringMap = {
        {GL_RED, "R"},
        {GL_GREEN, "G"},
        {GL_BLUE, "B"},
        {GL_RG, "RG"},
        {GL_RGB, "RGB"},
        {GL_BGR, "BGR"},
        {GL_RGBA, "RGBA"},
        {GL_BGRA, "BGRA"},
        {GL_DEPTH_COMPONENT, "D"},
        {GL_STENCIL_INDEX, "S"},
        {GL_DEPTH_STENCIL, "DS"}
    };

    const Asset::ConstBiMap<GLenum, std::string> TextureAsset::_sPixelTypeStringMap = {
        {GL_BYTE, "Byte"},
        {GL_UNSIGNED_BYTE, "UByte"},
        {GL_SHORT, "Short"},
        {GL_UNSIGNED_SHORT, "UShort"},
        {GL_INT, "Int"},
        {GL_UNSIGNED_INT, "UInt"},
        {GL_HALF_FLOAT, "Half"},
        {GL_FLOAT, "Float"}
    };

    const Asset::ConstBiMap<osg::Texture::FilterMode, std::string> TextureAsset::_sTextureFilterStringMap = {
        {osg::Texture::FilterMode::LINEAR, "Linear"},
        {osg::Texture::FilterMode::LINEAR_MIPMAP_LINEAR, "LinearMipmapLinear"},
        {osg::Texture::FilterMode::LINEAR_MIPMAP_NEAREST, "LinearMipmapNearest"},
        {osg::Texture::FilterMode::NEAREST, "Nearest"},
        {osg::Texture::FilterMode::NEAREST_MIPMAP_LINEAR, "NearestMipmapLinear"},
        {osg::Texture::FilterMode::NEAREST_MIPMAP_NEAREST, "NearestMipmapNearest"},
    };

    const Asset::ConstBiMap<osg::Texture::WrapMode, std::string> TextureAsset::_sTextureWrapStringMap = {
        {osg::Texture::WrapMode::CLAMP, "Clamp"},
        {osg::Texture::WrapMode::CLAMP_TO_EDGE, "ClampToEdge"},
        {osg::Texture::WrapMode::CLAMP_TO_BORDER, "ClampToBorder"},
        {osg::Texture::WrapMode::REPEAT, "Repeat"},
        {osg::Texture::WrapMode::MIRROR, "Mirror"},
    };

    osg::ref_ptr<osg::Texture> TextureAsset::createTextureByType(GLenum type)
    {
        switch (type)
        {
        case GL_TEXTURE_2D:
            return new osg::Texture2D;
        case GL_TEXTURE_2D_ARRAY:
            return new osg::Texture2DArray;
        case GL_TEXTURE_3D:
            return new osg::Texture3D;
        case GL_TEXTURE_CUBE_MAP:
            return new osg::TextureCubeMap;
        default:
            return nullptr;
        }
    }

    void TextureAsset::serialize(Json& json, std::vector<char>& binary, std::vector<std::string>& reference) const
    {
        GLenum textureType = _texture->getTextureTarget();
        json["Type"] = _sTextureTypeStringMap.forwardAt(textureType);
        json["Width"] = _texture->getTextureWidth();
        json["Height"] = _texture->getTextureHeight();
        if (textureType == GL_TEXTURE_2D_ARRAY || textureType == GL_TEXTURE_3D)
            json["Depth"] = _texture->getTextureDepth();

        json["Format"] = _sTextureFormatStringMap.forwardAt(_texture->getInternalFormat());
        json["PixelFormat"] = _sPixelFormatStringMap.forwardAt(_texture->getSourceFormat());
        json["PixelType"] = _sPixelTypeStringMap.forwardAt(_texture->getSourceType());

        json["MinFilter"] = _sTextureFilterStringMap.forwardAt(_texture->getFilter(osg::Texture::MIN_FILTER));
        json["MagFilter"] = _sTextureFilterStringMap.forwardAt(_texture->getFilter(osg::Texture::MAG_FILTER));
        if (textureType == GL_TEXTURE_3D)
            json["WrapR"] = _sTextureWrapStringMap.forwardAt(_texture->getWrap(osg::Texture::WRAP_R));
        json["WrapS"] = _sTextureWrapStringMap.forwardAt(_texture->getWrap(osg::Texture::WRAP_S));
        json["WrapT"] = _sTextureWrapStringMap.forwardAt(_texture->getWrap(osg::Texture::WRAP_T));

        bool generationMipmap = _texture->getUseHardwareMipMapGeneration();
        json["GenerationMipmap"] = generationMipmap;
        uint32_t mipmapCount = _texture->getImage(0)->getNumMipmapLevels();
        json["MipmapCount"] = mipmapCount;

        uint32_t imageCount = _texture->getNumImages();
        size_t bufferSize = 0;
        for (uint32_t i = 0; i < imageCount; ++i)
            bufferSize += generationMipmap ? _texture->getImage(i)->getTotalSizeInBytes() : _texture->getImage(i)->getTotalSizeInBytesIncludingMipmaps();
        binary.resize(bufferSize);

        std::vector<Json> imagesJsonArray(imageCount);
        size_t offset = 0;
        for (uint32_t i = 0; i < imageCount; ++i)
        {
            size_t bufferSize = generationMipmap ? _texture->getImage(i)->getTotalSizeInBytes() : _texture->getImage(i)->getTotalSizeInBytesIncludingMipmaps();
            osg::Image* image = _texture->getImage(i);
            imagesJsonArray[i]["BufferOffset"] = offset;
            imagesJsonArray[i]["BufferSize"] = bufferSize;

            // write mipmap buffer offsets when didn't hardware generation mipmaps
            if (!generationMipmap && mipmapCount > 0)
            {
                std::vector<uint32_t> mipmapBufferOffsets(mipmapCount);
                for (uint32_t j = 0; j < mipmapCount; ++j)
                    mipmapBufferOffsets.push_back(image->getMipmapOffset(j));
                imagesJsonArray[i]["MipmapBufferOffsets"] = mipmapBufferOffsets;
            }

            std::memcpy(
                &binary[offset],
                (char*)image->getDataPointer(),
                bufferSize
            );

            offset += bufferSize;
        }
        json["Images"] = imagesJsonArray;
    }

    void TextureAsset::deserialize(const Json& json, const std::vector<char>& binary, const std::vector<std::string>& reference)
    {
        int width = json["Width"].get<int>();
        int height = json["Height"].get<int>();
        int depth = 1;
        if (json.contains("Depth"))
            depth = json["Depth"].get<int>();

        GLenum textureType = _sTextureTypeStringMap.backwardAt(json["Type"].get<std::string>());
        _texture = createTextureByType(textureType);

        GLenum internalFormat = _sTextureFormatStringMap.backwardAt(json["Format"].get<std::string>());
        _texture->setInternalFormat(internalFormat);
        GLenum pixelFormat = _sPixelFormatStringMap.backwardAt(json["PixelFormat"].get<std::string>());
        _texture->setSourceFormat(pixelFormat);
        GLenum pixelType = _sPixelTypeStringMap.backwardAt(json["PixelType"].get<std::string>());
        _texture->setSourceType(pixelType);

        osg::Texture::FilterMode minFilter = _sTextureFilterStringMap.backwardAt(json["MinFilter"].get<std::string>());
        _texture->setFilter(osg::Texture::MIN_FILTER, minFilter);
        osg::Texture::FilterMode magFilter = _sTextureFilterStringMap.backwardAt(json["MagFilter"].get<std::string>());
        _texture->setFilter(osg::Texture::MAG_FILTER, minFilter);
        osg::Texture::WrapMode wrapS = _sTextureWrapStringMap.backwardAt(json["WrapS"].get<std::string>());
        _texture->setWrap(osg::Texture::WRAP_S, wrapS);
        osg::Texture::WrapMode wrapT = _sTextureWrapStringMap.backwardAt(json["WrapT"].get<std::string>());
        _texture->setWrap(osg::Texture::WRAP_T, wrapT);
        if (textureType == GL_TEXTURE_3D)
        {
            osg::Texture::WrapMode wrapR = _sTextureWrapStringMap.backwardAt(json["WrapR"].get<std::string>());
            _texture->setWrap(osg::Texture::WRAP_R, wrapR);
        }

        bool generationMipmap = json["GenerationMipmap"].get<bool>();
        _texture->setUseHardwareMipMapGeneration(generationMipmap);
        uint32_t mipmapCount = json["MipmapCount"].get<uint32_t>();
        if (textureType == GL_TEXTURE_2D)
            dynamic_cast<osg::Texture2D*>(_texture.get())->setNumMipmapLevels(mipmapCount);
        else if (textureType == GL_TEXTURE_2D_ARRAY)
            dynamic_cast<osg::Texture2DArray*>(_texture.get())->setNumMipmapLevels(mipmapCount);
        else if (textureType == GL_TEXTURE_3D)
            dynamic_cast<osg::Texture3D*>(_texture.get())->setNumMipmapLevels(mipmapCount);
        else if (textureType == GL_TEXTURE_CUBE_MAP)
            dynamic_cast<osg::TextureCubeMap*>(_texture.get())->setNumMipmapLevels(mipmapCount);

        const Json& imagesJsonArray = json["Images"];
        for (uint32_t i = 0; i < imagesJsonArray.size(); ++i)
        {
            const Json& imageJson = imagesJsonArray.at(i);
            size_t bufferOffset = imageJson["BufferOffset"].get<size_t>();
            size_t bufferSize = imageJson["BufferSize"].get<size_t>();
            uint8_t* buffer = new uint8_t[bufferSize];
            std::memcpy(buffer, &binary[bufferOffset], bufferSize);

            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->setImage(width, height, textureType == GL_TEXTURE_3D ? depth : 1, internalFormat, pixelFormat, pixelType, buffer, osg::Image::USE_NEW_DELETE);
            if (!generationMipmap && mipmapCount > 0)
            {
                std::vector<uint32_t> mipmapBufferOffsets = imageJson["MipmapBufferOffsets"].get<std::vector<uint32_t>>();
                image->setMipmapLevels(mipmapBufferOffsets);
            }

            if (textureType == GL_TEXTURE_2D)
                dynamic_cast<osg::Texture2D*>(_texture.get())->setImage(image);
            else if (textureType == GL_TEXTURE_2D_ARRAY)
                dynamic_cast<osg::Texture2DArray*>(_texture.get())->setImage(i, image);
            else if (textureType == GL_TEXTURE_3D)
                dynamic_cast<osg::Texture3D*>(_texture.get())->setImage(image);
            else if (textureType == GL_TEXTURE_CUBE_MAP)
                dynamic_cast<osg::TextureCubeMap*>(_texture.get())->setImage(osg::TextureCubeMap::POSITIVE_X + i, image);
        }
    }
}
