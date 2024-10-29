#include "Texture2D.h"

namespace xxx
{
    namespace refl
    {
        template <> Type* Reflection::createType<Texture2D>()
        {
            Class* clazz = new TClass<Texture2D, Texture>("Texture2D");
            clazz->addProperty("Width", &Texture2D::mWidth);
            clazz->addProperty("Height", &Texture2D::mHeight);
            return clazz;
        }
    }
}
