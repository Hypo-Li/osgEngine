#pragma once
#include "Texture.h"

namespace xxx
{
    class Texture2D : public Texture
    {
        REFLECT_CLASS(Texture2D)
    public:
        Texture2D() = default;
        Texture2D(osg::Image* image, const TextureImportOptions& options) : Texture(options)
        {
            mWidth = image->s();
            mHeight = image->t();
            // in order to correctly read image data, need to set PixelFormat and PixelType based on the image
            mPixelFormat = PixelFormat(image->getPixelFormat());
            mPixelType = PixelType(image->getDataType());
            osg::Texture2D* texture2d = new osg::Texture2D(image);
            mOsgTexture = texture2d;

            // apply and clear image
            apply();
            osg::State* state = Context::get().getGraphicsContext()->getState();
            texture2d->apply(*state);
            texture2d->setImage(nullptr);
            std::tie(mPixelFormat, mPixelType) = getPixelFormatAndTypeFromFormat(mFormat);
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

        virtual bool apply() override
        {
            if (Texture::apply())
            {
                osg::Texture2D* texture2d = dynamic_cast<osg::Texture2D*>(mOsgTexture.get());
                texture2d->setTextureWidth(mWidth);
                texture2d->setTextureHeight(mHeight);
                return true;
            }
            return false;
        }

    protected:
        uint32_t mWidth = 0, mHeight = 0;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture2D>();
    }
}
