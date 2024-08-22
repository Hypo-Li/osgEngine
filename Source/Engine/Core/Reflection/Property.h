#pragma once
#include "Type.h"
#include "Argument.h"
#include "Metadata.h"

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

    template <typename T>
    struct array_element;

    template <typename T, std::size_t Size>
    struct array_element<T[Size]> {
        using type = T;
    };

    template <typename T>
    using array_element_t = typename array_element<T>::type;

	class Property : public MetadataBase
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
    template <typename ClassType, typename ObjectType, std::size_t Index = 0>
    class PropertyValueInstance : public Property
    {
        //using ClassType = typename member_object_traits<T>::class_type;
        //using ObjectType = typename member_object_traits<T>::object_type;
        using PropertyType = ObjectType ClassType::*;
        static constexpr bool IsArrayMember = std::is_array_v<ObjectType>;
    public:
        PropertyValueInstance(std::string_view name, PropertyType property) :
            Property(name),
            mProperty(property)
        {
#ifdef _DEBUG
            if constexpr (IsArrayMember)
                mType = Type::getType<array_element_t<ObjectType>>();
            else
                mType = Type::getType<ObjectType>();
#endif
        }
        
        virtual ~PropertyValueInstance() = default;

        virtual Type* getType() const override
        {
            if constexpr (IsArrayMember)
                return Type::getType<array_element_t<ObjectType>>();
            else
                return Type::getType<ObjectType>();
        }

        virtual Type* getOwnerType() const override
        {
            return Type::getType<ClassType>();
        }

        virtual void setValue(void* instance, Argument value) const override
        {
            if constexpr (IsArrayMember)
                (static_cast<ClassType*>(instance)->*mProperty)[Index] = value.getValue<array_element_t<ObjectType>>();
            else
                static_cast<ClassType*>(instance)->*mProperty = value.getValue<ObjectType>();
        }

        virtual void getValue(void* instance, void* value) const override
        {
            if constexpr (IsArrayMember)
                *static_cast<array_element_t<ObjectType>*>(value) = (static_cast<ClassType*>(instance)->*mProperty)[Index];
            else
                *static_cast<ObjectType*>(value) = static_cast<ClassType*>(instance)->*mProperty;
        }

        virtual void* getValuePtr(void* instance) const override
        {
            if constexpr (IsArrayMember)
                return &((static_cast<ClassType*>(instance)->*mProperty)[Index]);
            else
                return &(static_cast<ClassType*>(instance)->*mProperty);
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            if constexpr (IsArrayMember)
                return &((static_cast<const ClassType*>(instance)->*mProperty)[Index]);
            else
                return &(static_cast<const ClassType*>(instance)->*mProperty);
        }

    private:
        PropertyType mProperty;
#ifdef _DEBUG
        Type* mType;
#endif
    };

    template <typename ClassType, typename ObjectType>
    class PropertyAccessorInstance : public Property
    {
        using GetterType = std::function<void(const ClassType&, ObjectType&)>;
        using SetterType = std::function<void(ClassType&, const ObjectType&)>;
    public:
        PropertyAccessorInstance(std::string_view name, GetterType getter, SetterType setter) :
#ifdef _DEBUG
            mType(Type::getType<ObjectType>()),
#endif
            Property(name),
            mGetter(getter),
            mSetter(setter) {}

        virtual ~PropertyAccessorInstance() = default;

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
            mSetter(*static_cast<ClassType*>(instance), value.getValue<ObjectType>());
        }

        virtual void getValue(void* instance, void* value) const override
        {
            mGetter(*static_cast<ClassType*>(instance), *static_cast<ObjectType*>(value));
        }

        virtual void* getValuePtr(void* instance) const override
        {
            return nullptr;
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            return nullptr;
        }

    private:
        GetterType mGetter;
        SetterType mSetter;
#ifdef _DEBUG
        Type* mType;
#endif
    };
}
