#pragma once
#include "Texture.h"

namespace xxx
{
    class TextureCubemap : public Texture
    {
        REFLECT_CLASS(TextureCubemap)
    public:
        TextureCubemap() = default;
        TextureCubemap(osg::Image* image, const TextureImportOptions& options, uint32_t cubemapSize = 512) : Texture(options)
        {
            mWidth = mHeight = cubemapSize;
            mPixelFormat = PixelFormat(image->getPixelFormat());
            mPixelType = PixelType(image->getDataType());
            osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
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

            std::tie(mPixelFormat, mPixelType) = getPixelFormatAndTypeFromFormat(mFormat);
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
                    image->setInternalTextureFormat(mFormat);
                    image->setPixelFormat(mPixelFormat);
                    image->setDataType(mPixelType);
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

        virtual bool apply() override
        {
            if (Texture::apply())
            {
                osg::TextureCubeMap* textureCubemap = dynamic_cast<osg::TextureCubeMap*>(mOsgTexture.get());
                textureCubemap->setTextureWidth(mWidth);
                textureCubemap->setTextureHeight(mHeight);
                return true;
            }
            return false;
        }

    protected:
        uint32_t mWidth = 0, mHeight = 0;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<TextureCubemap>();
    }
}
