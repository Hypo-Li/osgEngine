#pragma once
#include <Engine/Core/Object.h>
#include <Engine/Core/Context.h>

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

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
            osg::State* state = Context::get().getGraphicsContext()->getState();
            osg::ref_ptr<osg::Image> image = osgDB::readImageFile(imagePath);
            mWidth = image->s();
            mHeight = image->t();
            mFormat = image->getInternalTextureFormat();
            mPixelFormat = image->getPixelFormat();
            mPixelType = image->getDataType();
            osg::Texture2D* texture2d = new osg::Texture2D((image));
            mOsgTexture = texture2d;
            apply();
            texture2d->apply(*state);
            texture2d->setImage(nullptr);
        }
        virtual ~Texture2D() = default;

        virtual void preSerialize(Serializer* serializer) override
        {
            if (serializer->isSaver())
            {
                osg::State* state = Context::get().getGraphicsContext()->getState();
                mOsgTexture->apply(*state);
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->readImageFromCurrentTexture(state->getContextID(), false, mPixelType);
                uint32_t dataSize = image->getTotalSizeInBytes();
                const uint8_t* dataPtr = static_cast<const uint8_t*>(image->data());
                mData.assign(dataPtr, dataPtr + dataSize);
            }
        }

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                osg::State* state = Context::get().getGraphicsContext()->getState();
                osg::ref_ptr<osg::Image> image = new osg::Image;
                image->allocateImage(mWidth, mHeight, 1, mPixelFormat, mPixelType);
                std::memcpy(image->data(), mData.data(), mData.size());
                osg::Texture2D* texture2d = new osg::Texture2D;
                texture2d->setImage(image);
                mOsgTexture = texture2d;
                apply();
                texture2d->apply(*state);
                texture2d->setImage(nullptr);
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
            mFormat = GL_RGBA16F;// image->getInternalTextureFormat();
            mPixelFormat = GL_RGBA;
            mPixelType = GL_FLOAT;
            mMinFilter = mipmapLevels == 0 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR;

            osg::Texture2D* hdrTexture = new osg::Texture2D(image);

            osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
            mOsgTexture = textureCubemap;

            apply();

            osg::Program* envCubemapProgram = new osg::Program;
            envCubemapProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "IBL/EnvCubemap.comp.glsl"));
            osg::ref_ptr<osg::BindImageTexture> cubemapImage = new osg::BindImageTexture(0, textureCubemap, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F/*image->getInternalTextureFormat()*/, 0, true);
            osg::StateSet* stateSet = new osg::StateSet;
            stateSet->setAttributeAndModes(envCubemapProgram);
            stateSet->setAttributeAndModes(cubemapImage);
            stateSet->addUniform(new osg::Uniform("uEnvMapTexture", 0));
            stateSet->setTextureAttributeAndModes(0, hdrTexture);

            osg::State* state = Context::get().getGraphicsContext()->getState();
            osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
            state->apply(stateSet);
            extensions->glDispatchCompute(16, 16, 6);
            extensions->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        virtual void preSerialize(Serializer* serializer) override
        {
            if (serializer->isSaver())
            {
                osg::State* state = Context::get().getGraphicsContext()->getState();
                mOsgTexture->apply(*state);
                for (int i = 0; i < 6; ++i)
                {
                    osg::ref_ptr<osg::Image> image = new osg::Image;
                    image->readImageFromCurrentTexture(state->getContextID(), false, mPixelType, i);
                    uint32_t faceDataSize = image->getTotalSizeInBytes();
                    if (i == 0)
                        mData.resize(faceDataSize * 6);
                    std::memcpy(mData.data() + i * faceDataSize, image->data(), faceDataSize);
                }
            }
        }

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                osg::State* state = Context::get().getGraphicsContext()->getState();
                size_t faceDataSize = mData.size() / 6;
                osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
                for (int i = 0; i < 6; ++i)
                {
                    osg::ref_ptr<osg::Image> image = new osg::Image;
                    image->allocateImage(mWidth, mHeight, 1, mPixelFormat, mPixelType);
                    size_t mySize = image->getTotalDataSize();
                    std::memcpy(image->data(), mData.data() + i * faceDataSize, faceDataSize);
                    textureCubemap->setImage(i, image);
                }
                mOsgTexture = textureCubemap;
                apply();
                textureCubemap->apply(*state);
                for (int i = 0; i < 6; ++i)
                    textureCubemap->setImage(i, nullptr);

            }
            mData.clear();
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
            clazz->addProperty("MipmapLevels", &TextureCubemap::mMipmapLevels);
            return clazz;
        }
    }
}
