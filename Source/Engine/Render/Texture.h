#pragma once
#include <Engine/Core/Object.h>

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>

namespace xxx
{
    class Texture : public Object
    {
    public:
        enum Type : uint8_t
        {
            Texture2D,
            Texture2DArray,
            Texture3D,
            TextureCubemap,
        };
        Texture();
        virtual ~Texture();

    private:
        Type mType;
        int mWidth, mHeight, mDepth;
        GLenum mFormat;
        GLenum mPixelFormat;
        GLenum mPixelType;
        GLenum mMinFilter, mMaxFilter;
        GLenum mWrapR, mWrapS, mWrapT;
        std::vector<uint8_t> mData;
        osg::ref_ptr<osg::Texture> mOsgTexture;
    };
}
