#pragma once
#include "Texture.h"

namespace xxx
{
    class Texture2DArray : public Texture
    {
        REFLECT_CLASS(Texture2DArray)
    public:
        Texture2DArray() = default;
        Texture2DArray(const std::vector<osg::Image*>& images, const TextureImportOptions& options)
        {

        }
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Texture2DArray>();
    }
}
