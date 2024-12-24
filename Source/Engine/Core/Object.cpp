#include "Object.h"
#include "AssetManager.h"

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

    Asset* Object::getAsset() const
    {
        return AssetManager::get().getAsset(getRoot()->getGuid());
    }

    namespace refl
    {
        template <> Type* Reflection::createType<Object>()
        {
            Class* clazz = new TClass<Object>("Object");
            clazz->addProperty("Guid", &Object::mGuid);
            clazz->addProperty("Owner", &Object::mOwner);
            return clazz;
        }
    }
}
