#pragma once
#include "Reflection/Reflection.h"
//#include "Serialization/Serializer.h"

#include <osg/Referenced>
#include <osg/ref_ptr>

namespace xxx
{
    struct Guid
    {
        union
        {
            struct
            {
                uint32_t A;
                uint32_t B;
                uint32_t C;
                uint32_t D;
            };
            struct
            {
                uint64_t data[2];
            };
        };

        bool operator==(const Guid& rhs) const
        {
            return data[0] == rhs.data[0] && data[1] == rhs.data[1];
        }
        bool operator!=(const Guid& rhs) const
        {
            return data[0] != rhs.data[0] || data[1] != rhs.data[1];
        }

        static Guid newGuid();
    };

    class Asset;
    class Object : public osg::Referenced
    {
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Object>());
        }

    public:
        Object() : mGuid(Guid::newGuid()) {}
        virtual ~Object() = default;

        Guid getGuid() const
        {
            return mGuid;
        }

        // load something from osg object when serialization
        virtual void preSerialize() {}

        // store something to osg object when deserialization and release
        virtual void postSerialize() {}

    private:
        Guid mGuid;
        Asset* mOwnerAsset;
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<Guid>()
        {
            Struct* structGuid = new StructInstance<Guid>("Guid");
            Property* propA = structGuid->addProperty("A", &Guid::A);
            Property* propB = structGuid->addProperty("B", &Guid::B);
            Property* propC = structGuid->addProperty("C", &Guid::C);
            Property* propD = structGuid->addProperty("D", &Guid::D);
            return structGuid;
        }

        template <>
        inline Type* Reflection::createType<Object>()
        {
            Class* clazz = new ClassInstance<Object>("Object");
            // Guid is a special property, cannot serialize directly.
            //Property* propGuid = clazz->addProperty("Guid", &Object::mGuid);
            sRegisteredClassMap.emplace("Object", clazz);
            return clazz;
        }
    }
}

namespace std
{
    template <>
    struct hash<xxx::Guid>
    {
        std::size_t operator()(const xxx::Guid& guid) const
        {
            return std::hash<uint64_t>()(guid.data[0]) ^ std::hash<uint64_t>()(guid.data[1]);
        }
    };
}
