#include "TextureCubemap.h"

namespace xxx
{
    namespace refl
    {
        template <> Type* Reflection::createType<TextureCubemap>()
        {
            Class* clazz = new TClass<TextureCubemap, Texture>("TextureCubemap");
            clazz->addProperty("Size", &TextureCubemap::mSize);
            return clazz;
        }
    }
}
