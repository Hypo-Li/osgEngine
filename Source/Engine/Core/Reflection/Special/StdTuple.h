#pragma once
#include "../Special.h"

#include <tuple>

namespace xxx::refl
{
    class StdTuple : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Tuple;
        }

        virtual std::vector<Type*> getTypes() const = 0;
        virtual size_t getElementCount() const = 0;
        virtual std::vector<void*> getElementPtrs(void* instance) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::tuple>, int> = 0>
    class StdTupleInstance : public StdTuple
    {
        friend class Reflection;
        static constexpr size_t ElementCount = std::tuple_size_v<T>;

        template <std::size_t... Is>
        static std::vector<Type*> getTypesImpl(std::index_sequence<Is...>)
        {
            return { Type::getType<std::tuple_element_t<Is, T>>()... };
        }

        template <std::size_t... Is>
        static std::vector<void*> getElementPtrsImpl(std::index_sequence<Is...>, void* instance)
        {
            T* tuple = static_cast<T*>(instance);
            return { &std::get<Is>(*tuple)... };
        }

    public:
        virtual void* newInstance() const override
        {
            return new T;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<T*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new T[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<T*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return false;
        }

        virtual std::vector<Type*> getTypes() const override
        {
            return getTypesImpl(std::make_index_sequence<ElementCount>());
        }

        virtual size_t getElementCount() const override
        {
            return ElementCount;
        }

        virtual std::vector<void*> getElementPtrs(void* instance) const override
        {
            return getElementPtrsImpl(std::make_index_sequence<ElementCount>(), instance);
        }

    protected:
        template <std::size_t... Is>
        static std::string createTupleArgsString(std::index_sequence<Is...>)
        {
            std::string result;
            std::vector<std::string_view> typeNames = { Reflection::getType<std::tuple_element_t<Is, T>>()->getName()... };
            for (uint32_t i = 0; i < typeNames.size(); ++i)
            {
                result += typeNames[i];
                if (i != typeNames.size() - 1)
                    result += ", ";
            }
            return result;
        }

        StdTupleInstance()
        {
            static std::string name = "std::tuple<" + createTupleArgsString(std::make_index_sequence<std::tuple_size_v<T>>{}) + ">";
            mName = name;
            mSize = sizeof(T);
        }
    };
}
