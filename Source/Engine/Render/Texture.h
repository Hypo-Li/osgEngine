#pragma once
#include <Engine/Core/Object.h>
#include <Engine/Core/Context.h>

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osgDB/ReadFile>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>

namespace xxx
{
    class Texture : public Object
    {
        REFLECT_CLASS(Texture)
    public:
        Texture() = default;
        virtual ~Texture() = default;

        osg::Texture* getOsgTexture() const
        {
            return mOsgTexture;
        }

        void setFormat(GLenum format)
        {
            mFormat = format;
        }

        GLenum getFormat() const
        {
            return mFormat;
        }

        void setPixelFormat(GLenum pixelFormat)
        {
            mPixelFormat = pixelFormat;
        }

        GLenum getPixelFormat() const
        {
            return mPixelFormat;
        }

        void setPixelType(GLenum pixelType)
        {
            mPixelType = pixelType;
        }

        GLenum getPixelType() const
        {
            return mPixelType;
        }

        void setMinFilter(GLenum minFilter)
        {
            mMinFilter = minFilter;
        }

        GLenum getMinFilter() const
        {
            return mMinFilter;
        }

        void setMagFilter(GLenum magFilter)
        {
            mMagFilter = magFilter;
        }

        GLenum getMagFilter() const
        {
            return mMagFilter;
        }

        void setWrapR(GLenum wrapR)
        {
            mWrapR = wrapR;
        }

        GLenum getWrapR() const
        {
            return mWrapR;
        }

        void setWrapS(GLenum wrapS)
        {
            mWrapS = wrapS;
        }

        GLenum getWrapS() const
        {
            return mWrapS;
        }

        void setWrapT(GLenum wrapT)
        {
            mWrapT = wrapT;
        }

        GLenum getWrapT() const
        {
            return mWrapT;
        }

        virtual void apply()
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
            mPixelType = image->getDataType();
            mOsgTexture = new osg::Texture2D(image);
            apply();
        }
        virtual ~Texture2D() = default;

        virtual void preSerialize(Serializer* serializer) override
        {
            if (serializer->isSaver())
            {
                osg::Texture2D* texture2d = dynamic_cast<osg::Texture2D*>(mOsgTexture.get());
                uint32_t dataSize = texture2d->getImage()->getTotalSizeInBytes();
                const uint8_t* dataPtr = static_cast<const uint8_t*>(texture2d->getImage()->data());
                mData.assign(dataPtr, dataPtr + dataSize);
            }
        }

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->allocateImage(mWidth, mHeight, 1, mPixelFormat, mPixelType);
                std::memcpy(image->data(), mData.data(), mData.size());
                osg::Texture2D* texture2d = new osg::Texture2D;
                texture2d->setImage(image);
                mOsgTexture = texture2d;
                apply();
            }
            mData.clear();
        }

        virtual void apply() override
        {
            Texture::apply();
            osg::Texture2D* texture2d = dynamic_cast<osg::Texture2D*>(mOsgTexture.get());
            texture2d->setTextureWidth(mWidth);
            texture2d->setTextureHeight(mHeight);
        }

    protected:
        uint32_t mWidth = 0, mHeight = 0;
    };

    class TextureCubemap : public Texture
    {
        REFLECT_CLASS(TextureCubemap)
    public:
        TextureCubemap() = default;
        TextureCubemap(const std::string& imagePath, uint32_t cubemapSize = 512, uint32_t mipmapLevels = 0)
        {
            osg::ref_ptr<osg::Image> image = osgDB::readImageFile(imagePath);
            mWidth = mHeight = cubemapSize;
            mMipmapLevels = mipmapLevels;
            mFormat = image->getInternalTextureFormat();
            mPixelFormat = image->getPixelFormat();
            mPixelType = image->getDataType();
            mMinFilter = mipmapLevels == 0 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR;

            osg::Texture2D* hdrTexture = new osg::Texture2D(image);

            osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
            mOsgTexture = textureCubemap;

            apply();

            osg::Program* envCubemapProgram = new osg::Program;
            envCubemapProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "IBL/EnvCubemap.comp.glsl"));
            osg::ref_ptr<osg::BindImageTexture> cubemapImage = new osg::BindImageTexture(0, textureCubemap, osg::BindImageTexture::WRITE_ONLY, image->getInternalTextureFormat(), 0, true);
            osg::ref_ptr<osg::DispatchCompute> envCubemapDispatch = new osg::DispatchCompute(16, 16, 6);
            envCubemapDispatch->getOrCreateStateSet()->setAttributeAndModes(envCubemapProgram);
            envCubemapDispatch->getOrCreateStateSet()->setAttributeAndModes(cubemapImage);
            envCubemapDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uEnvMapTexture", 0));
            envCubemapDispatch->getOrCreateStateSet()->setTextureAttributeAndModes(0, hdrTexture);

            osg::State* state = Context::get().getGraphicsContext()->getState();
            state->apply(envCubemapDispatch->getOrCreateStateSet());
            osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
            extensions->glDispatchCompute(16, 16, 6);
            extensions->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        virtual void apply() override
        {
            Texture::apply();
            osg::TextureCubeMap* textureCubemap = dynamic_cast<osg::TextureCubeMap*>(mOsgTexture.get());
            textureCubemap->setTextureWidth(mWidth);
            textureCubemap->setTextureHeight(mHeight);
            textureCubemap->setNumMipmapLevels(mMipmapLevels);
        }

    protected:
        uint32_t mWidth = 0, mHeight = 0;
        uint32_t mMipmapLevels = 0;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture>();
        template <> Type* Reflection::createType<Texture2D>();
        template <> inline Type* Reflection::createType<TextureCubemap>()
        {
            Class* clazz = new ClassInstance<TextureCubemap, Texture>("TextureCubemap");
            clazz->addProperty("Width", &TextureCubemap::mWidth);
            clazz->addProperty("Height", &TextureCubemap::mHeight);
            return clazz;
        }
    }
}
