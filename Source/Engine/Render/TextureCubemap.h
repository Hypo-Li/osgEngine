#pragma once
#include "Texture.h"

namespace xxx
{
    class TextureCubemap : public Texture
    {
        REFLECT_CLASS(TextureCubemap)
    public:
        TextureCubemap() = default;
        TextureCubemap(osg::TextureCubeMap* textureCubemap) : Texture(textureCubemap)
        {
            if (!textureCubemap)
                return;
            mSize = textureCubemap->getTextureWidth();
            mMipmapCount = textureCubemap->getNumMipmapLevels() - 1;
        }
        TextureCubemap(osg::Image* image, const TextureImportOptions& options, uint32_t cubemapSize = 512) : Texture(options)
        {
            mSize = cubemapSize;
            mPixelFormat = PixelFormat(image->getPixelFormat());
            mPixelType = PixelType(image->getDataType());
            osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
            // 当不从Image构建Texture时, mipmap数量为使用setNumMipmapLevels设置的数量
            uint8_t mipmapCount = 0;
            uint32_t mipmapSize = cubemapSize;
            while (mipmapSize)
            {
                mipmapSize >>= 1;
                mipmapCount++;
            }
            textureCubemap->setNumMipmapLevels(mipmapCount);

            mOsgTexture = textureCubemap;

            apply();

            std::string imageFormatName = getImageFormatName(mFormat);

            osg::ref_ptr<osg::Texture2D> hdrTexture = new osg::Texture2D(image);
            osg::Program* envCubemapProgram = new osg::Program;
            envCubemapProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "IBL/EnvCubemap.comp.glsl"));
            osg::ref_ptr<osg::BindImageTexture> cubemapImage = new osg::BindImageTexture(0, textureCubemap, osg::BindImageTexture::WRITE_ONLY, mFormat, 0, true);
            osg::StateSet* stateSet = new osg::StateSet;
            stateSet->setAttributeAndModes(envCubemapProgram);
            stateSet->setAttributeAndModes(cubemapImage);
            stateSet->addUniform(new osg::Uniform("uEnvMapTexture", 0));
            stateSet->setTextureAttributeAndModes(0, hdrTexture);
            stateSet->setDefine("IMAGE_FORMAT", imageFormatName);

            osg::State* state = Context::get().getGraphicsContext()->getState();
            osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
            state->apply(stateSet);
            extensions->glDispatchCompute(16, 16, 6);
            extensions->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            if (mMipmapGeneration)
            {
                mOsgTexture->getTextureObject(state->getContextID())->bind();
                extensions->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            }

            std::tie(mPixelFormat, mPixelType) = getPixelFormatAndTypeFromFormat(mFormat);
        }

        virtual void preSerialize(Serializer* serializer) override
        {
            if (serializer->isSaver())
            {
                osg::State* state = Context::get().getGraphicsContext()->getState();
                mOsgTexture->getTextureObject(state->getContextID())->bind();
                bool saveMipmap = false;
                if (mMinFilter != FilterMode::Linear && mMinFilter != FilterMode::Nearest && !mMipmapGeneration)
                    saveMipmap = true;
                for (int i = 0; i < 6; ++i)
                {
                    osg::ref_ptr<osg::Image> image = new osg::Image;
                    image->readImageFromCurrentTexture(state->getContextID(), saveMipmap, mPixelType, i);
                    if (saveMipmap)
                    {
                        mMipmapDataOffsets = image->getMipmapLevels();
                        if (mMipmapCount < mMipmapDataOffsets.size())
                        {
                            mMipmapDataOffsets.resize(mMipmapCount);
                            image->setMipmapLevels(mMipmapDataOffsets);
                        }
                    }
                    uint32_t faceDataSize = saveMipmap ? image->getTotalSizeInBytesIncludingMipmaps() : image->getTotalSizeInBytes();
                    if (i == 0)
                        mData.resize(faceDataSize * 6); // every face's dataSize is same
                    std::memcpy(mData.data() + i * faceDataSize, image->data(), faceDataSize);
                }
                compressData();
            }
        }

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                decompressData();
                size_t faceDataSize = mData.size() / 6;
                osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
                for (int i = 0; i < 6; ++i)
                {
                    osg::ref_ptr<osg::Image> image = new osg::Image;
                    image->setImage(mSize, mSize, 1, mFormat, mPixelFormat, mPixelType, mData.data() + i * faceDataSize, osg::Image::NO_DELETE);
                    image->setMipmapLevels(mMipmapDataOffsets);
                    textureCubemap->setImage(i, image);
                }
                mOsgTexture = textureCubemap;
                apply();

                for (int i = 0; i < 6; ++i)
                    textureCubemap->setImage(i, nullptr);
            }
            mData.clear();
        }

        virtual void apply() override
        {
            if (!mOsgTexture)
                mOsgTexture = new osg::TextureCubeMap;
            osg::TextureCubeMap* textureCubemap = dynamic_cast<osg::TextureCubeMap*>(mOsgTexture.get());
            textureCubemap->setTextureWidth(mSize);
            textureCubemap->setTextureHeight(mSize);
            Texture::apply();
        }

        void setSize(uint32_t size)
        {
            mSize = size;
        }

        uint32_t getSize() const
        {
            return mSize;
        }

    protected:
        uint32_t mSize = 0;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<TextureCubemap>();
    }
}
