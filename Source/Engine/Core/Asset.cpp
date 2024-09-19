#include "Asset.h"
#include "AssetManager.h"

namespace xxx
{
    using namespace refl;

    Asset::Asset(const std::string& path) :
        mPath(path)
    {

    }

    void Asset::setObjectAssetRecursively(Object* object)
    {
        if (!object)
            return;
        object->mAsset = this;
        Class* clazz = object->getClass();
        while (clazz)
        {
            for (Property* prop : clazz->getProperties())
            {
                if (prop->getDeclaredType()->getKind() == Type::Kind::Class)
                {
                    Object* propObject;
                    prop->getValue(object, &propObject);
                    if (propObject->getAsset() == nullptr)
                        setObjectAssetRecursively(propObject);
                }
            }
            clazz = clazz->getBaseClass();
        }
    }

    void Asset::recordObjects(AssetSerializer* serializer)
    {

    }
}
