#pragma once
#include "Texture.h"

namespace xxx
{
    class Texture3D : public Texture
    {
        REFLECT_CLASS(Texture3D)
    public:

    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture3D>();
    }
}
