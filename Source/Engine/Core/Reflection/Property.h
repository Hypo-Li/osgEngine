#pragma once
#include "Reflection.h"
#include "Metadata.h"

namespace xxx::refl
{
	class Property : public MetadataBase
	{
	public:
        std::string_view getName() const
        {
            return mName;
        }

        virtual Type* getDeclaredType() const = 0;
        //virtual Type* getOwnerType() const = 0;
        virtual void setValue(void* instance, const void* value) const = 0;
        virtual void getValue(const void* instance, void* value) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual const void* getValuePtr(const void* instance) const = 0;
        virtual bool compare(const void* instance1, const void* instance2) const = 0;

    protected:
        Property(std::string_view name) :
            mName(name) {}

    private:
        std::string_view mName;
	};

    template <typename Declared, typename Owner>
    class TProperty : public Property
    {
        using MemberPtr = Declared Owner::*;
    public:
        TProperty(std::string_view name, MemberPtr memberPtr) :
            Property(name), mMemberPtr(memberPtr) {}

        virtual Type* getDeclaredType() const override
        {
            return Reflection::getType<Declared>();
        }

        /*virtual Type* getOwnerType() const override
        {
            return Reflection::getType<Owner>();
        }*/

        virtual void setValue(void* instance, const void* value) const override
        {
            static_cast<Owner*>(instance)->*mMemberPtr = *static_cast<const Declared*>(value);
        }

        virtual void getValue(const void* instance, void* value) const override
        {
            *static_cast<Declared*>(value) = static_cast<const Owner*>(instance)->*mMemberPtr;
        }

        virtual void* getValuePtr(void* instance) const override
        {
            return &(static_cast<Owner*>(instance)->*mMemberPtr);
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            return &(static_cast<const Owner*>(instance)->*mMemberPtr);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if constexpr (SpecialType<Declared> || !Comparable<Declared>)
                return Reflection::getType<Declared>()->compare(getValuePtr(instance1), getValuePtr(instance2));
            else
                return *static_cast<const Declared*>(getValuePtr(instance1)) == *static_cast<const Declared*>(getValuePtr(instance2));
        }

    private:
        MemberPtr mMemberPtr;
    };

    /*template <typename Owner, typename FakeDeclared, std::size_t Index = 0>
    class TProperty : public Property
    {
        using MemberObject = FakeDeclared Owner::*;
        static constexpr bool IsArrayMember = std::is_array_v<FakeDeclared>;
        using Declared = std::conditional_t<IsArrayMember, array_element_t<FakeDeclared>, FakeDeclared>;
    public:
        TProperty(std::string_view name, MemberObject memberObject) :
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
            if constexpr (!is_comparable_v<Declared> || is_special_v<Declared>)
                return Reflection::getType<Declared>()->compare(getValuePtr(instance1), getValuePtr(instance2));
            else
                return *static_cast<const Declared*>(getValuePtr(instance1)) == *static_cast<const Declared*>(getValuePtr(instance2));
        }

    protected:
        MemberObject mMemberObject;
    };*/
}
