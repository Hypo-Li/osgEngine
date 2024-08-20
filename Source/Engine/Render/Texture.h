#pragma once
#include <Engine/Core/Object.h>

#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>

namespace xxx
{
    enum TextureType : uint8_t
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCubemap,
    };

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
        static const Texture* getDefaultObject()
        {
            static osg::ref_ptr<Texture> defaultObject = new Texture;
            return defaultObject.get();
        }
    public:
        Texture() = default;
        virtual ~Texture() = default;

    private:
        TextureType mType;
        int mWidth, mHeight, mDepth;
        GLenum mFormat;
        GLenum mPixelFormat;
        GLenum mPixelType;
        GLenum mMinFilter, mMaxFilter;
        GLenum mWrapR, mWrapS, mWrapT;
        std::vector<uint8_t> mData;
        osg::ref_ptr<osg::Texture> mOsgTexture;
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<TextureType>()
        {
            Enum* enumerate = new Enum("TextureType", sizeof(TextureType));
            enumerate->setValues<TextureType>(
                {
                    {"Texture2D", TextureType::Texture2D},
                    {"Texture2DArray", TextureType::Texture2DArray},
                    {"Texture3D", TextureType::Texture3D},
                    {"TextureCubemap", TextureType::TextureCubemap}
                }
            );
            return enumerate;
        }

        template <>
        inline Type* Reflection::createType<Texture>()
        {
            Class* clazz = new Class("Texture", sizeof(Texture), Texture::createInstance);
            clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
            Property* propType = clazz->addProperty("Type", &Texture::mType);
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
