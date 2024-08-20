#pragma once
#include "Type.h"
#include "Any.h"
#include "Argument.h"

#include <functional>

namespace xxx::refl
{
    // member object traits
    template <typename T>
    struct member_object_traits;

    template < typename Class, typename Object>
    struct member_object_traits<Object Class::*> {
        using class_type = Class;
        using object_type = Object;
    };

	class Property
	{
	public:
        Property(std::string_view name) : mName(name) {}
        virtual ~Property() = default;

        std::string_view getName() const
        {
            return mName;
        }

        virtual Type* getType() const = 0;
        virtual Type* getOwnerType() const = 0;
        virtual void getValue(void* instance, void* value) const = 0;
        virtual void setValue(void* instance, Argument value) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual const void* getValuePtr(const void* instance) const = 0;

    protected:
        std::string_view mName;
	};

    //template <typename T, std::enable_if_t<std::is_member_object_pointer_v<T>, int> = 0>
    template <typename ClassType, typename ObjectType>
    class PropertyInstance : public Property
    {
        //using ClassType = typename member_object_traits<T>::class_type;
        //using ObjectType = typename member_object_traits<T>::object_type;
        using PropertyType = ObjectType ClassType::*;
        using GetterType = std::function<void(ClassType&, ObjectType&)>;
        using SetterType = std::function<void(ClassType&, const ObjectType&)>;
    public:
        PropertyInstance(std::string_view name, PropertyType property) :
            Property(name),
            mProperty(property) {}
        PropertyInstance(std::string_view name, GetterType getter, SetterType setter) :
            Property(name),
            mProperty(nullptr),
            mGetter(getter),
            mSetter(setter) {}
        virtual ~PropertyInstance() = default;

        virtual Type* getType() const override
        {
            return Type::getType<ObjectType>();
        }

        virtual Type* getOwnerType() const override
        {
            return Type::getType<ClassType>();
        }

        virtual void setValue(void* instance, Argument value) const override
        {
            if (mSetter)
                mSetter(*static_cast<ClassType*>(instance), value.getValue<ObjectType>());
            else
                static_cast<ClassType*>(instance)->*mProperty = value.getValue<ObjectType>();
        }

        virtual void getValue(void* instance, void* value) const override
        {
            if (mGetter)
                mGetter(*static_cast<ClassType*>(instance), *static_cast<ObjectType*>(value));
            else
                *static_cast<ObjectType*>(value) = static_cast<ClassType*>(instance)->*mProperty;
        }

        virtual void* getValuePtr(void* instance) const override
        {
            if (mProperty)
                return &(static_cast<ClassType*>(instance)->*mProperty);
            else
                return nullptr;
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            if (mProperty)
                return &(static_cast<const ClassType*>(instance)->*mProperty);
            else
                return nullptr;
        }

    private:
        PropertyType mProperty;
        GetterType mGetter;
        SetterType mSetter;
    };
}
