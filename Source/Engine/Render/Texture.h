#pragma once
#include <Engine/Core/Object.h>
#include <Engine/Core/Context.h>
#include <Engine/Core/OsgReflection.h>

#include <ThirdParty/fast-lzma2/fast-lzma2.h>

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <functional>
#include <future>

namespace xxx
{
    struct TextureImportOptions;

    class Texture : public Object
    {
        REFLECT_CLASS(Texture)
    public:
        Texture() = default;
        Texture(osg::Texture* texture) : mOsgTexture(texture)
        {
            if (!mOsgTexture)
                return;
            mFormat = Format(mOsgTexture->getInternalFormat());
            mPixelFormat = PixelFormat(mOsgTexture->getSourceFormat());
            mPixelType = PixelType(mOsgTexture->getSourceType());
            mMinFilter = FilterMode(mOsgTexture->getFilter(osg::Texture::MIN_FILTER));
            mMagFilter = FilterMode(mOsgTexture->getFilter(osg::Texture::MAG_FILTER));
            mWrapR = WrapMode(mOsgTexture->getWrap(osg::Texture::WRAP_R));
            mWrapS = WrapMode(mOsgTexture->getWrap(osg::Texture::WRAP_S));
            mWrapT = WrapMode(mOsgTexture->getWrap(osg::Texture::WRAP_T));
            mBorderColor = mOsgTexture->getBorderColor();
            mMipmapGeneration = mOsgTexture->getUseHardwareMipMapGeneration();
        }
        Texture(const TextureImportOptions& options);
        virtual ~Texture() = default;

        enum Format
        {
            R8 = GL_RED,
            RG8 = GL_RG,
            RGB8 = GL_RGB,
            RGBA8 = GL_RGBA,

            R16 = GL_R16,
            RG16 = GL_RG16,
            RGB16 = GL_RGB16,
            RGBA16 = GL_RGBA16,

            R8S = GL_R8_SNORM,
            RG8S = GL_RG8_SNORM,
            RGB8S = GL_RGB8_SNORM,
            RGBA8S = GL_RGBA8_SNORM,

            R16S = GL_R16_SNORM,
            RG16S = GL_RG16_SNORM,
            RGB16S = GL_RGB16_SNORM,
            RGBA16S = GL_RGBA16_SNORM,

            R8I = GL_R8I,
            RG8I = GL_RG8I,
            RGB8I = GL_RGB8I,
            RGBA8I = GL_RGBA8I,

            R16I = GL_R16I,
            RG16I = GL_RG16I,
            RGB16I = GL_RGB16I,
            RGBA16I = GL_RGBA16I,

            R32I = GL_R32I,
            RG32I = GL_RG32I,
            RGB32I = GL_RGB32I,
            RGBA32I = GL_RGBA32I,

            R8UI = GL_R8UI,
            RG8UI = GL_RG8UI,
            RGB8UI = GL_RGB8UI,
            RGBA8UI = GL_RGBA8UI,

            R16UI = GL_R16UI,
            RG16UI = GL_RG16UI,
            RGB16UI = GL_RGB16UI,
            RGBA16UI = GL_RGBA16UI,

            R32UI = GL_R32UI,
            RG32UI = GL_RG32UI,
            RGB32UI = GL_RGB32UI,
            RGBA32UI = GL_RGBA32UI,

            R16F = GL_R16F,
            RG16F = GL_RG16F,
            RGB16F = GL_RGB16F,
            RGBA16F = GL_RGBA16F,

            R32F = GL_R32F,
            RG32F = GL_RG32F,
            RGB32F = GL_RGB32F,
            RGBA32F = GL_RGBA32F,

            SRGB8 = GL_SRGB8,
            SRGB8_Alpha8 = GL_SRGB8_ALPHA8,

            /*Compressed_Red_RGTC1,
            Compressed_Signed_Red_RGTC1,
            Compressed_RG_RGTC2,
            Compressed_Signed_RG_RGTC2,

            Compressed_RGBA_BPTC_Unorm = GL_COMPRESSED_RGBA_BPTC_UNORM,
            Compressed_SRGB_Alpha_BPTC_Unorm,
            Compressed_RGB_BPTC_Signed_Float,
            Compressed_RGB_BPTC_Unsigned_Float,

            Compressed_RGBA_ASTC_4x4,
            Compressed_RGBA_ASTC_5x4,
            Compressed_RGBA_ASTC_5x5,
            Compressed_RGBA_ASTC_6x5,
            Compressed_RGBA_ASTC_6x6,
            Compressed_RGBA_ASTC_8x5,
            Compressed_RGBA_ASTC_8x6,
            Compressed_RGBA_ASTC_10x5,
            Compressed_RGBA_ASTC_10x6,
            Compressed_RGBA_ASTC_8x8,
            Compressed_RGBA_ASTC_10x8,
            Compressed_RGBA_ASTC_10x10,
            Compressed_RGBA_ASTC_12x10,
            Compressed_RGBA_ASTC_12x12,
            Compressed_SRGB8_Alpha8_ASTC_4x4,
            Compressed_SRGB8_Alpha8_ASTC_5x4,
            Compressed_SRGB8_Alpha8_ASTC_5x5,
            Compressed_SRGB8_Alpha8_ASTC_6x5,
            Compressed_SRGB8_Alpha8_ASTC_6x6,
            Compressed_SRGB8_Alpha8_ASTC_8x5,
            Compressed_SRGB8_Alpha8_ASTC_8x6,
            Compressed_SRGB8_Alpha8_ASTC_10x5,
            Compressed_SRGB8_Alpha8_ASTC_10x6,
            Compressed_SRGB8_Alpha8_ASTC_8x8,
            Compressed_SRGB8_Alpha8_ASTC_10x8,
            Compressed_SRGB8_Alpha8_ASTC_10x10,
            Compressed_SRGB8_Alpha8_ASTC_12x10,
            Compressed_SRGB8_Alpha8_ASTC_12x12,*/

            Compressed_RGB_S3TC_DXT1 = GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
            Compressed_RGBA_S3TC_DXT1 = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
            Compressed_RGBA_S3TC_DXT3 = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
            Compressed_RGBA_S3TC_DXT5 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
            /*Compressed_SRGB_S3TC_DXT1,
            Compressed_SRGB_Alpha_S3TC_DXT1,
            Compressed_SRGB_Alpha_S3TC_DXT3,
            Compressed_SRGB_Alpha_S3TC_DXT5,*/
        };

        enum PixelFormat
        {
            R = GL_RED,
            RG = GL_RG,
            RGB = GL_RGB,
            RGBA = GL_RGBA,
        };

        enum PixelType
        {
            Unsigned_Byte = GL_UNSIGNED_BYTE,
            Byte = GL_BYTE,
            Unsigned_Short = GL_UNSIGNED_SHORT,
            Short = GL_SHORT,
            Unsigned_Int = GL_UNSIGNED_INT,
            Int = GL_INT,
            Half = GL_HALF_FLOAT,
            Float = GL_FLOAT,
        };

        enum FilterMode
        {
            Linear = GL_LINEAR,
            Nearest = GL_NEAREST,
            Linear_Mipmap_Linear = GL_LINEAR_MIPMAP_LINEAR,
            Linear_Mipmap_Nearest = GL_LINEAR_MIPMAP_NEAREST,
            Nearest_Mipmap_Linear = GL_NEAREST_MIPMAP_LINEAR,
            Nearest_Mipmap_Nearest = GL_NEAREST_MIPMAP_NEAREST,
        };

        enum WrapMode
        {
            Repeat = GL_REPEAT,
            Mirrored_Repeat = GL_MIRRORED_REPEAT,
            Clamp = GL_CLAMP,
            Clamp_To_Edge = GL_CLAMP_TO_EDGE,
            Clamp_To_Border = GL_CLAMP_TO_BORDER,
            Mirror_Clamp_To_Edge = GL_MIRROR_CLAMP_TO_EDGE,
        };

        virtual void postSave() override
        {
            mData.clear();
            mData.shrink_to_fit();
        }

        osg::Texture* getOsgTexture() const
        {
            return mOsgTexture;
        }

        void setFormat(Format format)
        {
            mFormat = format;
        }

        Format getFormat() const
        {
            return mFormat;
        }

        void setPixelFormat(PixelFormat pixelFormat)
        {
            mPixelFormat = pixelFormat;
        }

        PixelFormat getPixelFormat() const
        {
            return mPixelFormat;
        }

        void setPixelType(PixelType pixelType)
        {
            mPixelType = pixelType;
        }

        PixelType getPixelType() const
        {
            return mPixelType;
        }

        void setMinFilter(FilterMode minFilter)
        {
            mMinFilter = minFilter;
        }

        FilterMode getMinFilter() const
        {
            return mMinFilter;
        }

        void setMagFilter(FilterMode magFilter)
        {
            mMagFilter = magFilter;
        }

        FilterMode getMagFilter() const
        {
            return mMagFilter;
        }

        void setWrapR(WrapMode wrapR)
        {
            mWrapR = wrapR;
        }

        WrapMode getWrapR() const
        {
            return mWrapR;
        }

        void setWrapS(WrapMode wrapS)
        {
            mWrapS = wrapS;
        }

        WrapMode getWrapS() const
        {
            return mWrapS;
        }

        void setWrapT(WrapMode wrapT)
        {
            mWrapT = wrapT;
        }

        WrapMode getWrapT() const
        {
            return mWrapT;
        }

        void setBorderColor(osg::Vec4f borderColor)
        {
            mBorderColor = borderColor;
        }

        osg::Vec4f getBorderColor() const
        {
            return mBorderColor;
        }

        void setMipmapGeneration(bool generation)
        {
            mMipmapGeneration = generation;
        }

        bool getMipmapGeneration() const
        {
            return mMipmapGeneration;
        }

        void setMipmapCount(uint8_t mipmapCount)
        {
            mMipmapCount = mipmapCount;
        }

        uint8_t getMipmapCount() const
        {
            return mMipmapCount;
        }

        void setDataCompression(bool compression)
        {
            mDataCompression = compression;
        }

        bool getDataCompression() const
        {
            return mDataCompression;
        }

        virtual void apply()
        {
            mOsgTexture->setInternalFormat(mFormat);
            mOsgTexture->setSourceFormat(mPixelFormat);
            mOsgTexture->setSourceType(mPixelType);
            mOsgTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::FilterMode(mMinFilter));
            mOsgTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::FilterMode(mMagFilter));
            mOsgTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::WrapMode(mWrapR));
            mOsgTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode(mWrapS));
            mOsgTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::WrapMode(mWrapT));
            mOsgTexture->setBorderColor(mBorderColor);
            mOsgTexture->setUseHardwareMipMapGeneration(mMipmapGeneration);

            if (std::this_thread::get_id() == Context::get().getRenderingThreadId())
            {
                osg::State* state = Context::get().getGraphicsContext()->getState();
                mOsgTexture->apply(*state);
            }
            else
            {
                std::promise<void> promise;
                std::future<void> future = promise.get_future();
                Context::get().getRenderingCommandQueue().push([this, &promise] {
                    osg::State* state = Context::get().getGraphicsContext()->getState();
                    mOsgTexture->apply(*state);
                    promise.set_value();
                });
                future.get();
            }
        }

        static void bindOsgTexture(osg::Texture* texture)
        {
            osg::State* state = Context::get().getGraphicsContext()->getState();
            osg::Texture::TextureObject* texObj = texture->getTextureObject(state->getContextID());
            if (texObj)
            {
#if USE_OSG_370
                texObj->bind(*state);
#else
                texObj->bind();
#endif
            }
        }

    protected:
        Format mFormat = Format::RGBA8;
        PixelFormat mPixelFormat = PixelFormat::RGBA;
        PixelType mPixelType = PixelType::Unsigned_Byte;
        FilterMode mMinFilter = FilterMode::Linear_Mipmap_Linear;
        FilterMode mMagFilter = FilterMode::Linear;
        WrapMode mWrapR = WrapMode::Clamp;
        WrapMode mWrapS = WrapMode::Clamp;
        WrapMode mWrapT = WrapMode::Clamp;
        osg::Vec4f mBorderColor = osg::Vec4(0, 0, 0, 1);
        bool mMipmapGeneration = true;
        // only useful when mMipmapGeneration == false and mMinFilter is mipmap filter
        uint8_t mMipmapCount = 0;
        std::vector<uint32_t> mMipmapDataOffsets;
        std::vector<uint8_t> mData;
        bool mDataCompression = false;

        osg::ref_ptr<osg::Texture> mOsgTexture = nullptr;

        void compressData()
        {
            if (!mDataCompression)
                return;
            std::vector<uint8_t> compressedData(mData.size());
            size_t compressedSize = FL2_compress(compressedData.data(), compressedData.size(), mData.data(), mData.size(), 0);
            compressedData.resize(compressedSize);
            mData = compressedData;
        }

        void decompressData()
        {
            if (!mDataCompression)
                return;
            size_t decompressedSize = FL2_findDecompressedSize(mData.data(), mData.size());
            std::vector<uint8_t> decompressedData(decompressedSize);
            FL2_decompress(decompressedData.data(), decompressedSize, mData.data(), mData.size());
            mData = decompressedData;
        }

        static std::pair<PixelFormat, PixelType> choosePixelFormatAndTypeByFormat(Format format);

        static std::string getImageFormatName(Format format);

        static bool isCompressedFormat(Format format)
        {
            switch (format)
            {
            case Format::Compressed_RGB_S3TC_DXT1:
            case Format::Compressed_RGBA_S3TC_DXT1:
            case Format::Compressed_RGBA_S3TC_DXT3:
            case Format::Compressed_RGBA_S3TC_DXT5:
                return true;
            default:
                return false;
            }
        }
    };

    struct TextureImportOptions
    {
        Texture::Format format = Texture::Format::RGBA8;
        Texture::FilterMode minFilter = Texture::FilterMode::Linear_Mipmap_Linear;
        Texture::FilterMode magFilter = Texture::FilterMode::Linear;
        Texture::WrapMode wrapR = Texture::WrapMode::Repeat;
        Texture::WrapMode wrapS = Texture::WrapMode::Repeat;
        Texture::WrapMode wrapT = Texture::WrapMode::Repeat;
        osg::Vec4f borderColor = osg::Vec4f(0, 0, 0, 1);
        bool mipmapGeneration = true;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture::Format>();
        template <> Type* Reflection::createType<Texture::PixelFormat>();
        template <> Type* Reflection::createType<Texture::PixelType>();
        template <> Type* Reflection::createType<Texture::FilterMode>();
        template <> Type* Reflection::createType<Texture::WrapMode>();
        template <> Type* Reflection::createType<Texture>();
    }
}
