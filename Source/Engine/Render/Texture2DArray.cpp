#include "Texture2DArray.h"

namespace xxx
{
    namespace refl
    {
        template <> Type* Reflection::createType<Texture2DArray>()
        {
            Class* clazz = new TClass<Texture2DArray, Texture>("Texture2DArray");
            return clazz;
        }
    }
}
