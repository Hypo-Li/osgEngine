#pragma once
#include "Type.h"
#include "Any.h"
#include "Argument.h"

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
        virtual void setValue(void* instance, Argument value) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual const void* getValuePtr(const void* instance) const = 0;

    protected:
        std::string_view mName;
	};

    template <typename T, std::enable_if_t<std::is_member_object_pointer_v<T>, int> = 0>
    class PropertyInstance : public Property
    {
        using ClassType = typename member_object_traits<T>::class_type;
        using ObjectType = typename member_object_traits<T>::object_type;
    public:
        PropertyInstance(std::string_view name, T property) :
            Property(name),
            mProperty(property) {}
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
            static_cast<ClassType*>(instance)->*mProperty = value.getValue<ObjectType>();
        }

        virtual void* getValuePtr(void* instance) const override
        {
            return &(static_cast<ClassType*>(instance)->*mProperty);
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            return &(static_cast<const ClassType*>(instance)->*mProperty);
        }

    private:
        T mProperty;
    };
}
