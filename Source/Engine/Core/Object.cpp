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
        template <> Type* Reflection::createType<Guid>()
        {
            Struct* structGuid = new StructInstance<Guid>("Guid");
            Property* propA = structGuid->addProperty("A", &Guid::A);
            Property* propB = structGuid->addProperty("B", &Guid::B);
            Property* propC = structGuid->addProperty("C", &Guid::C);
            Property* propD = structGuid->addProperty("D", &Guid::D);
            return structGuid;
        }

        template <> Type* Reflection::createType<Object>()
        {
            Class* clazz = new ClassInstance<Object>("Object");
            clazz->addProperty("Owner", &Object::mOwner);
            getClassMap().emplace("Object", clazz);
            return clazz;
        }
    }
}
