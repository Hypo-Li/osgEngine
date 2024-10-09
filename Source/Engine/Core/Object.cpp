#include "Object.h"
#include "AssetManager.h"

#include <objbase.h>

namespace xxx
{
    Object::Object() :
        mGuid(Guid::newGuid()),
        mOwner(nullptr)
    {

    }

    Object::Object(const Object& other) :
        mGuid(Guid::newGuid()),
        mOwner(other.mOwner)
    {

    }

    Guid Guid::newGuid()
    {
        Guid result;
        if (CoCreateGuid((GUID*)&result) == S_OK)
            return result;
        return result;
    }

    Asset* Object::getAsset() const
    {
        return AssetManager::get().getAsset(getRoot()->getGuid());
    }

    namespace refl
    {
        template <> Type* Reflection::createType<Object>()
        {
            Class* clazz = new ClassInstance<Object>("Object");
            clazz->addProperty("Owner", &Object::mOwner);
            getClassMap().emplace("Object", clazz);
            return clazz;
        }
    }
}
