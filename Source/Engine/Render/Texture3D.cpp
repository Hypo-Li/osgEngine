#include "Texture3D.h"

namespace xxx
{
    namespace refl
    {
        template <> Type* Reflection::createType<Texture3D>()
        {
            Class* clazz = new TClass<Texture3D, Texture>("Texture3D");
            return clazz;
        }
    }
}
