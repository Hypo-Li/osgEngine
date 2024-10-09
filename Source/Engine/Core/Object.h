#pragma once
#include "Reflection/Reflection.h"
#include "Serialization/Serializer.h"
#include "Guid.h"

#include <osg/Referenced>
#include <osg/ref_ptr>

template <typename T>
T* cloneObject(const T* object)
{
    if constexpr (std::is_abstract_v<T>)
        return nullptr;
    else
        return new T(*object);
}

#define REFLECT_CLASS(ClassName) \
    friend class refl::Reflection; \
    inline static refl::Class* sClass = refl::Reflection::getClass<ClassName>(); \
public: \
    virtual refl::Class* getClass() const { return sClass; } \
    virtual ClassName* clone() const { return cloneObject<ClassName>(this); }

namespace xxx
{
    class Asset;
    class AssetManager;

    class Object : public osg::Referenced
    {
        friend class Asset;
        friend class AssetManager;

        REFLECT_CLASS(Object)
    public:
        Object();
        Object(const Object& other);
        virtual ~Object() = default;

        Guid getGuid() const
        {
            return mGuid;
        }

        void setGuid(Guid guid)
        {
            mGuid = guid;
        }

        void setOwner(Object* owner)
        {
            mOwner = owner;
        }

        Object* getOwner() const
        {
            return mOwner;
        }

        Object* getRoot()
        {
            Object* owner = this;
            while (owner->getOwner())
                owner = owner->getOwner();
            return owner;
        }

        const Object* getRoot() const
        {
            const Object* owner = this;
            while (owner->getOwner())
                owner = owner->getOwner();
            return owner;
        }

        Asset* getAsset() const;

        // load something from osg object when serialization
        virtual void preSerialize(Serializer* serializer)
        {

        }

        // store something to osg object when deserialization and release
        virtual void postSerialize(Serializer* serializer)
        {

        }

    private:
        Guid mGuid;
        Object* mOwner;
    };
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Object>();
}

namespace std
{
    template <typename T>
    struct hash<osg::ref_ptr<T>>
    {
        std::size_t operator()(osg::ref_ptr<T> refptr) const
        {
            return std::hash<T*>()(refptr.get());
        }
    };
}
