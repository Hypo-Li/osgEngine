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
    class StdTupleInstance : public Special
    {
        friend class Reflection;
        static constexpr size_t ElementCount = std::tuple_size_v<T>;

        template <std::size_t... Is>
        std::vector<Type*> getTypesImpl(std::index_sequence<Is...>) const
        {
            return { Type::getType<std::tuple_element_t<Is, T>>()... };
        }

        template <std::size_t... Is>
        std::vector<void*> getElementPtrsImpl(std::index_sequence<Is...>, void* instance) const
        {
            T* tuple = static_cast<T*>(instance);
            return { &std::get<Is>(*tuple)... };
        }

    public:
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
        StdTupleInstance()
        {
            mName = typeid(T).name();
            mSize = sizeof(T);
        }
    };
}
