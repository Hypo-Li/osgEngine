#include <Engine/Render/Texture.h>

namespace xxx::refl
{
    template <> Type* Reflection::createType<Texture>()
    {
        Class* clazz = new ClassInstance<Texture>("Texture");
        clazz->addProperty("Format", &Texture::mFormat);
        clazz->addProperty("PixelFormat", &Texture::mPixelFormat);
        clazz->addProperty("PixelType", &Texture::mPixelType);
        clazz->addProperty("MinFilter", &Texture::mMinFilter);
        clazz->addProperty("MagFilter", &Texture::mMagFilter);
        clazz->addProperty("WrapS", &Texture::mWrapS);
        clazz->addProperty("WrapT", &Texture::mWrapT);
        Property* propData = clazz->addProperty("Data", &Texture::mData);
        propData->addMetadata(MetaKey::Hidden, true);
        getClassMap().emplace("Texture", clazz);
        return clazz;
    }

    template <> Type* Reflection::createType<Texture2D>()
    {
        Class* clazz = new ClassInstance<Texture2D, Texture>("Texture2D");
        clazz->addProperty("Width", &Texture2D::mWidth);
        clazz->addProperty("Height", &Texture2D::mHeight);
        getClassMap().emplace("Texture2D", clazz);
        return clazz;
    }
}
