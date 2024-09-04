#pragma once
#include "Type.h"
#include "Argument.h"
#include "Metadata.h"

#include <functional>

#include <osg/ref_ptr>

template <typename, typename = std::void_t<>>
struct is_comparable : std::false_type {};

template <typename T>
struct is_comparable<T, std::void_t<decltype(std::declval<const T>() == std::declval<const T>())>> : std::true_type {};

template <typename T>
static constexpr bool is_comparable_v = is_comparable<T>::value;

template <typename T>
struct array_element;

template <typename T, std::size_t Size>
struct array_element<T[Size]> {
    using type = T;
};

template <typename T>
using array_element_t = typename array_element<T>::type;

template <typename T>
struct remove_osg_ref_ptr {
    using type = T;
};

template <typename T>
struct remove_osg_ref_ptr<osg::ref_ptr<T>> {
    using type = T;
};

template <typename T>
using remove_osg_ref_ptr_t = typename remove_osg_ref_ptr<T>::type;

namespace xxx::refl
{
	class Property : public MetadataBase
	{
	public:
        Property(std::string_view name) : mName(name) {}
        virtual ~Property() = default;

        std::string_view getName() const
        {
            return mName;
        }

        virtual size_t getSize() const = 0;
        virtual bool isMemberProperty() const = 0;
        virtual bool isAccessorProperty() const = 0;
        virtual Type* getDeclaredType() const = 0;
        virtual Type* getOwnerType() const = 0;
        virtual void getValue(const void* instance, void* value) const = 0;
        virtual void setValue(void* instance, Argument value) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual const void* getValuePtr(const void* instance) const = 0;
        virtual bool compare(const void* instance1, const void* instance2) const = 0;

    protected:
        std::string_view mName;
	};

    template <typename Owner, typename Declared, std::size_t Index = 0>
    class PropertyMemberInstance : public Property
    {
        using MemberType = Declared Owner::*;
        static constexpr bool IsArrayMember = std::is_array_v<Declared>;
        static constexpr bool IsObjectProperty = std::is_base_of_v<Object, std::remove_pointer_t<remove_osg_ref_ptr_t<Declared>>>;
    public:
        PropertyMemberInstance(std::string_view name, MemberType member) :
            Property(name),
            mMember(member)
        {}
        
        virtual ~PropertyMemberInstance() = default;

        virtual size_t getSize() const override
        {
            return sizeof(Declared);
        }

        virtual bool isMemberProperty() const override
        {
            return true;
        }

        virtual bool isAccessorProperty() const override
        {
            return false;
        }

        virtual Type* getDeclaredType() const override
        {
            if constexpr (IsArrayMember)
                return Type::getType<array_element_t<Declared>>();
            else
                return Type::getType<Declared>();
        }

        virtual Type* getOwnerType() const override
        {
            return Type::getType<Owner>();
        }

        virtual void setValue(void* instance, Argument value) const override
        {
            if constexpr (IsArrayMember)
                (static_cast<Owner*>(instance)->*mMember)[Index] = value.getValue<array_element_t<Declared>>();
            else
                static_cast<Owner*>(instance)->*mMember = value.getValue<Declared>();
        }

        virtual void getValue(const void* instance, void* value) const override
        {
            if constexpr (IsArrayMember)
                *static_cast<array_element_t<Declared>*>(value) = (static_cast<const Owner*>(instance)->*mMember)[Index];
            else
                *static_cast<Declared*>(value) = static_cast<const Owner*>(instance)->*mMember;
        }

        virtual void* getValuePtr(void* instance) const override
        {
            if constexpr (IsArrayMember)
                return &((static_cast<Owner*>(instance)->*mMember)[Index]);
            else
                return &(static_cast<Owner*>(instance)->*mMember);
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            if constexpr (IsArrayMember)
                return &((static_cast<const Owner*>(instance)->*mMember)[Index]);
            else
                return &(static_cast<const Owner*>(instance)->*mMember);
        }

        virtual bool compare(const void* instance1, const void* instance2) const
        {
            if constexpr (is_comparable_v<Declared>)
                return *static_cast<const Declared*>(getValuePtr(instance1)) == *static_cast<const Declared*>(getValuePtr(instance2));
            else
                return mType->compare(getValuePtr(instance1), getValuePtr(instance2));
        }

    private:
        MemberType mMember;
    };

    template <typename Owner, typename Declared>
    class PropertyAccessorInstance : public Property
    {
        using GetterType = std::function<void(const Owner&, Declared&)>;
        using SetterType = std::function<void(Owner&, const Declared&)>;
        static constexpr bool IsObjectProperty = std::is_base_of_v<Object, std::remove_pointer_t<remove_osg_ref_ptr_t<Declared>>>;
    public:
        PropertyAccessorInstance(std::string_view name, GetterType getter, SetterType setter) :
            Property(name),
            mGetter(getter),
            mSetter(setter)
        {}

        virtual ~PropertyAccessorInstance() = default;

        virtual size_t getSize() const override
        {
            return sizeof(Declared);
        }

        virtual bool isMemberProperty() const override
        {
            return false;
        }

        virtual bool isAccessorProperty() const override
        {
            return true;
        }

        virtual Type* getDeclaredType() const override
        {
            return Type::getType<Declared>();
        }

        virtual Type* getOwnerType() const override
        {
            return Type::getType<Owner>();
        }

        virtual void setValue(void* instance, Argument value) const override
        {
            mSetter(*static_cast<Owner*>(instance), value.getValue<Declared>());
        }

        virtual void getValue(const void* instance, void* value) const override
        {
            mGetter(*static_cast<const Owner*>(instance), *static_cast<Declared*>(value));
        }

        virtual void* getValuePtr(void* instance) const override
        {
            return nullptr;
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            mGetter(*static_cast<const Owner*>(instance), &mTemp);
            return nullptr;
        }

        virtual bool compare(const void* instance1, const void* instance2) const
        {
            Declared objects[2];
            getValue(instance1, &objects[0]);
            getValue(instance2, &objects[1]);
            if constexpr (is_comparable_v<Declared>)
                return objects[0] == objects[1];
            else
                return mType->compare(&objects[0], &objects[1]);
        }

    private:
        GetterType mGetter;
        SetterType mSetter;
        Declared mTemp;
    };
}
