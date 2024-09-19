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
    class AssetManager;

    class Object : public osg::Referenced
    {
        friend class Asset;
        friend class AssetManager;
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Object>());
        }

        virtual Object* clone() const
        {
            return new Object(*this);
        }

    public:
        Object();
        Object(const Object& other);
        Object(Object&& other) noexcept;
        virtual ~Object() = default;

        Guid getGuid() const
        {
            return mGuid;
        }

        Asset* getAsset() const
        {
            return mAsset;
        }

        // load something from osg object when serialization
        virtual void preSerialize() {}

        // store something to osg object when deserialization and release
        virtual void postSerialize() {}

    private:
        Guid mGuid;
        Asset* mAsset;
    };
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Guid>();
    template <> Type* Reflection::createType<Object>();
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
