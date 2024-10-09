#pragma once
#include "Reflection.h"
#include "Metadata.h"

#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

template <typename T>
static constexpr bool is_template_instance =
    is_std_array_v<T> ||
    is_instance_of_v<T, std::map> ||
    is_instance_of_v<T, std::pair> ||
    is_instance_of_v<T, std::set> ||
    is_instance_of_v<T, std::tuple> ||
    is_instance_of_v<T, std::unordered_map> ||
    is_instance_of_v<T, std::unordered_set> ||
    is_instance_of_v<T, std::variant> ||
    is_instance_of_v<T, std::vector>;

template <typename, typename = void>
struct is_comparable : std::false_type {};

template <typename T>
struct is_comparable<T, std::void_t<decltype(std::declval<const T>() == std::declval<const T>()), std::enable_if_t<!is_template_instance<T>>>> : std::true_type {};

template <typename T1, typename T2>
struct is_comparable<std::map<T1, T2>, std::void_t<std::enable_if_t<is_comparable<T1>::value>, std::enable_if_t<is_comparable<T2>::value>>> : std::true_type {};

template <typename T1, typename T2>
struct is_comparable<std::pair<T1, T2>, std::void_t<std::enable_if_t<is_comparable<T1>::value>, std::enable_if_t<is_comparable<T2>::value>>> : std::true_type {};

template <typename T>
struct is_comparable<std::set<T>, std::enable_if_t<is_comparable<T>::value>> : std::true_type {};

template <typename T1, typename T2>
struct is_comparable<std::unordered_map<T1, T2>, std::void_t<std::enable_if_t<is_comparable<T1>::value>, std::enable_if_t<is_comparable<T2>::value>>> : std::true_type {};

template <typename T>
struct is_comparable<std::unordered_set<T>, std::enable_if_t<is_comparable<T>::value>> : std::true_type {};

template <typename T>
struct is_comparable<std::vector<T>, std::enable_if_t<is_comparable<T>::value>> : std::true_type {};

template <typename T>
static constexpr bool is_comparable_v = is_comparable<T>::value;

template <typename T>
struct array_element {
    using type = T;
};

template <typename T, std::size_t Size>
struct array_element<T[Size]> {
    using type = T;
};

template <typename T>
using array_element_t = typename array_element<T>::type;

namespace xxx::refl
{
	class Property : public MetadataBase
	{
	public:
        Property(std::string_view name) :
            mName(name)
        {}

        std::string_view getName() const
        {
            return mName;
        }

        virtual Type* getDeclaredType() const = 0;
        virtual Type* getOwnerType() const = 0;
        virtual void getValue(const void* instance, void* value) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual const void* getValuePtr(const void* instance) const = 0;
        virtual void setValue(void* instance, const void* value) const = 0;
        virtual bool compare(const void* instance1, const void* instance2) const = 0;

    protected:
        std::string_view mName;
	};

    template <typename Owner, typename FakeDeclared, std::size_t Index = 0>
    class PropertyInstance : public Property
    {
        using MemberObject = FakeDeclared Owner::*;
        static constexpr bool IsArrayMember = std::is_array_v<FakeDeclared>;
        using Declared = std::conditional_t<IsArrayMember, array_element_t<FakeDeclared>, FakeDeclared>;
    public:
        PropertyInstance(std::string_view name, MemberObject memberObject) :
            Property(name),
            mMemberObject(memberObject)
        {}

        virtual Type* getDeclaredType() const override
        {
            return Reflection::getType<Declared>();
        }

        virtual Type* getOwnerType() const override
        {
            return Reflection::getType<Owner>();
        }

        virtual void getValue(const void* instance, void* value) const override
        {
            if constexpr (IsArrayMember)
                *static_cast<array_element_t<Declared>*>(value) = (static_cast<const Owner*>(instance)->*mMemberObject)[Index];
            else
                *static_cast<Declared*>(value) = static_cast<const Owner*>(instance)->*mMemberObject;
        }

        virtual void* getValuePtr(void* instance) const override
        {
            if constexpr (IsArrayMember)
                return &((static_cast<Owner*>(instance)->*mMemberObject)[Index]);
            else
                return &(static_cast<Owner*>(instance)->*mMemberObject);
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            if constexpr (IsArrayMember)
                return &((static_cast<const Owner*>(instance)->*mMemberObject)[Index]);
            else
                return &(static_cast<const Owner*>(instance)->*mMemberObject);
        }

        virtual void setValue(void* instance, const void* value) const override
        {
            if constexpr (IsArrayMember)
                (static_cast<Owner*>(instance)->*mMemberObject)[Index] = *static_cast<const Declared*>(value);
            else
                static_cast<Owner*>(instance)->*mMemberObject = *static_cast<const Declared*>(value);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if constexpr (is_comparable_v<Declared>)
                return *static_cast<const Declared*>(getValuePtr(instance1)) == *static_cast<const Declared*>(getValuePtr(instance2));
            else
                return Reflection::getType<Declared>()->compare(getValuePtr(instance1), getValuePtr(instance2));
        }

    protected:
        MemberObject mMemberObject;
    };
}
