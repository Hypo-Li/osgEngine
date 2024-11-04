#pragma once
#include "Texture.h"

namespace xxx
{
    class Texture2D : public Texture
    {
        REFLECT_CLASS(Texture2D)
    public:
        Texture2D() = default;
        Texture2D(osg::Texture2D* texture2D) : Texture(texture2D)
        {
            if (!texture2D)
                return;
            mWidth = texture2D->getTextureWidth();
            mHeight = texture2D->getTextureHeight();
            mMipmapCount = texture2D->getNumMipmapLevels() - 1;
        }
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
            texture2d->setImage(nullptr);
            std::tie(mPixelFormat, mPixelType) = choosePixelFormatAndTypeByFormat(mFormat);
        }
        virtual ~Texture2D() = default;

        virtual void preSave() override
        {
            osg::State* state = Context::get().getGraphicsContext()->getState();
            bindOsgTexture(mOsgTexture);

            bool saveMipmap = false;
            if (mMinFilter != FilterMode::Linear && mMinFilter != FilterMode::Nearest && !mMipmapGeneration)
                saveMipmap = true;
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->readImageFromCurrentTexture(state->getContextID(), saveMipmap, mPixelType);
            if (saveMipmap)
            {
                mMipmapDataOffsets = image->getMipmapLevels();
                if (mMipmapCount < mMipmapDataOffsets.size())
                {
                    mMipmapDataOffsets.resize(mMipmapCount);
                    image->setMipmapLevels(mMipmapDataOffsets);
                }
            }
            uint32_t dataSize = saveMipmap ? image->getTotalSizeInBytesIncludingMipmaps() : image->getTotalSizeInBytes();
            const uint8_t* dataPtr = static_cast<const uint8_t*>(image->data());
            mData.assign(dataPtr, dataPtr + dataSize);

            compressData();
        }

        virtual void postLoad() override
        {
            decompressData();
            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->setImage(mWidth, mHeight, 1, mFormat, mPixelFormat, mPixelType, mData.data(), osg::Image::NO_DELETE);
            image->setMipmapLevels(mMipmapDataOffsets);

            // 如果是压缩纹理, 图像的PixelFormat需要设置为相同的格式
            if (isCompressedFormat(mFormat))
                image->setPixelFormat(mFormat);

            osg::Texture2D* texture2d = new osg::Texture2D;
            texture2d->setImage(image);
            mOsgTexture = texture2d;
            apply();
            texture2d->setImage(nullptr);

            mData.clear();
            mData.shrink_to_fit();
        }

        virtual void apply() override
        {
            if (!mOsgTexture)
                mOsgTexture = new osg::Texture2D;
            osg::Texture2D* texture2d = dynamic_cast<osg::Texture2D*>(mOsgTexture.get());
            texture2d->setTextureWidth(mWidth);
            texture2d->setTextureHeight(mHeight);
            Texture::apply();
        }

    protected:
        uint32_t mWidth = 0, mHeight = 0;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture2D>();
    }
}
