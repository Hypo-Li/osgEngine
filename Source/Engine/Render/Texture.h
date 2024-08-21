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
        friend class refl::Reflection;
        static Object* createInstance()
        {
            return new Texture;
        }
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Texture>());
        }
    public:
        Texture() = default;
        virtual ~Texture() = default;

    protected:
        int mWidth, mHeight, mDepth;
        GLenum mFormat;
        GLenum mPixelFormat;
        GLenum mPixelType;
        GLenum mMinFilter, mMaxFilter;
        GLenum mWrapR, mWrapS, mWrapT;
        std::vector<uint8_t> mData;
        osg::ref_ptr<osg::Texture> mOsgTexture;
    };

    class Texture2D : public Texture
    {
    public:

    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<Texture>()
        {
            Class* clazz = new Class("Texture", sizeof(Texture), Texture::createInstance);
            clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
            Property* propWidth = clazz->addProperty("Width", &Texture::mWidth);
            Property* propHeight = clazz->addProperty("Height", &Texture::mHeight);
            Property* propFormat = clazz->addProperty("Format", &Texture::mFormat);
            Property* propPixelFormat = clazz->addProperty("PixelFormat", &Texture::mPixelFormat);
            Property* propPixelType = clazz->addProperty("PixelType", &Texture::mPixelType);
            Property* propMinFilter = clazz->addProperty("MinFilter", &Texture::mMinFilter);
            Property* propMaxFilter = clazz->addProperty("MaxFilter", &Texture::mMaxFilter);
            Property* propWrapR = clazz->addProperty("WrapR", &Texture::mWrapR);
            Property* propWrapS = clazz->addProperty("WrapS", &Texture::mWrapS);
            Property* propWrapT = clazz->addProperty("WrapT", &Texture::mWrapT);
            Property* propData = clazz->addProperty("Data", &Texture::mData);
            propData->addMetadata(MetaKey::Hidden, true);
            sRegisteredClassMap.emplace("Texture", clazz);
            return clazz;
        }
    }
}
