#pragma once
#include "Reflection/Reflection.h"

#include <osg/Referenced>
#include <osg/ref_ptr>

namespace xxx
{
    struct Guid
    {
        uint32_t A = 0;
        uint32_t B = 0;
        uint32_t C = 0;
        uint32_t D = 0;
    };

    class Object : public osg::Referenced
    {
        friend class refl::Reflection;
        static Object* createInstance()
        {
            return new Object;
        }
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Object>());
        }

    public:
        Object() = default;
        virtual ~Object() = default;

    private:
        Guid mGuid;
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<Guid>()
        {
            Struct* structGuid = new Struct("Guid", sizeof(Guid));
            Property* propA = structGuid->addProperty("A", &Guid::A);
            Property* propB = structGuid->addProperty("B", &Guid::B);
            Property* propC = structGuid->addProperty("C", &Guid::C);
            Property* propD = structGuid->addProperty("D", &Guid::D);
            return structGuid;
        }

        template <>
        inline Type* Reflection::createType<Object>()
        {
            Class* clazz = new Class("Object", sizeof(Object), Object::createInstance);
            Property* propGuid = clazz->addProperty("Guid", &Object::mGuid);
            sRegisteredClassMap.emplace("Object", clazz);
            return clazz;
        }
    }
}
