#include "Object.h"

#include <objbase.h>

namespace xxx
{
    Object::Object() :
        mGuid(Guid::newGuid())
    {

    }

    Object::Object(const Object& other) :
        mGuid(Guid::newGuid())
    {

    }

    Object::Object(Object&& other) noexcept :
        mGuid(other.mGuid)
    {

    }

    Object& Object::operator=(const Object& other)
    {
        return *this;
    }

    Object& Object::operator=(Object&& other) noexcept
    {
        return *this;
    }

    Guid Guid::newGuid()
    {
        Guid result;
        if (CoCreateGuid((GUID*)&result) == S_OK)
            return result;
        return result;
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
            // Guid is a special property, cannot serialize directly.
            //Property* propGuid = clazz->addProperty("Guid", &Object::mGuid);
            sRegisteredClassMap.emplace("Object", clazz);
            return clazz;
        }
    }
}
