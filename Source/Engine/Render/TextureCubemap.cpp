#include "TextureCubemap.h"

namespace xxx
{
    namespace refl
    {
        template <> Type* Reflection::createType<TextureCubemap>()
        {
            Class* clazz = new ClassInstance<TextureCubemap, Texture>("TextureCubemap");
            clazz->addProperty("Width", &TextureCubemap::mWidth);
            clazz->addProperty("Height", &TextureCubemap::mHeight);
            return clazz;
        }
    }
}
