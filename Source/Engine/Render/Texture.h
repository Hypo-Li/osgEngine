#pragma once
#include <Engine/Core/Object.h>

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osgDB/ReadFile>

namespace xxx
{
    class Texture : public Object
    {
        REFLECT_CLASS(Texture)
    public:
        Texture() = default;
        virtual ~Texture() = default;

        void fillTextureParameters()
        {
            if (!mOsgTexture)
                return;
            mOsgTexture->setInternalFormat(mFormat);
            mOsgTexture->setSourceFormat(mPixelFormat);
            mOsgTexture->setSourceType(mPixelType);
            mOsgTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::FilterMode(mMinFilter));
            mOsgTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::FilterMode(mMagFilter));
            mOsgTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::WrapMode(mWrapR));
            mOsgTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode(mWrapS));
            mOsgTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::WrapMode(mWrapT));
        }

    protected:
        GLenum mFormat = GL_RGBA;
        GLenum mPixelFormat = GL_RGBA;
        GLenum mPixelType = GL_UNSIGNED_BYTE;
        GLenum mMinFilter = GL_LINEAR_MIPMAP_LINEAR;
        GLenum mMagFilter = GL_LINEAR;
        GLenum mWrapR = GL_CLAMP;
        GLenum mWrapS = GL_CLAMP;
        GLenum mWrapT = GL_CLAMP;
        std::vector<uint8_t> mData;
        osg::ref_ptr<osg::Texture> mOsgTexture = nullptr;
    };

    class Texture2D : public Texture
    {
        REFLECT_CLASS(Texture2D)
    public:
        Texture2D() = default;
        Texture2D(const std::string& imagePath)
        {
            osg::Image* image = osgDB::readImageFile(imagePath);
            mWidth = image->s();
            mHeight = image->t();
            mFormat = image->getInternalTextureFormat();
            mPixelFormat = image->getPixelFormat();
            mOsgTexture = new osg::Texture2D(image);
            fillTextureParameters();
        }
        virtual ~Texture2D() = default;

        virtual void preSerialize() override
        {
            if (mOsgTexture)
            {
                osg::Texture2D* texture2d = dynamic_cast<osg::Texture2D*>(mOsgTexture.get());
                uint32_t dataSize = texture2d->getImage()->getTotalSizeInBytes();
                const uint8_t* dataPtr = static_cast<const uint8_t*>(texture2d->getImage()->data());
                mData.assign(dataPtr, dataPtr + dataSize);
            }
        }

        virtual void postSerialize() override
        {
            if (!mOsgTexture)
            {
                osg::Image* image = new osg::Image;
                image->allocateImage(mWidth, mHeight, 1, mPixelFormat, mPixelType);
                std::memcpy(image->data(), mData.data(), mData.size());
                mData.clear();
                osg::Texture2D* texture2d = new osg::Texture2D;
                texture2d->setImage(image);
                mOsgTexture = texture2d;
                fillTextureParameters();
            }
        }

    protected:
        int mWidth = 0, mHeight = 0;
        
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture>();
        template <> Type* Reflection::createType<Texture2D>();
    }
}
