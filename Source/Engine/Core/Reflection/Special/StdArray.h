#pragma once
#include "../Special.h"

#include <array>

template <typename T, size_t N>
struct container_traits<std::array<T, N>> {
    using type = T;
    static constexpr size_t Count = N;
};

namespace xxx::refl
{
    class StdArray : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Array;
        }
        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount() const = 0;
        virtual void* getElementPtrByIndex(void* array, uint32_t index) const = 0;
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_std_array_v<T>>>
    class StdArrayInstance : public StdArray
    {
        friend class Reflection;
        using Element = container_traits_t<T>;
        static constexpr size_t Count = container_traits<T>::Count;
    public:
        virtual void* newInstance() const override
        {
            return new std::array<Element, Count>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::array<Element, Count>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::array<Element, Count>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::array<Element, Count>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return false;
        }

        virtual Type* getElementType() const override
        {
            return Type::getType<Element>();
        }

        virtual size_t getElementCount() const override
        {
            return Count;
        }

        virtual void* getElementPtrByIndex(void* array, uint32_t index) const override
        {
            return &(static_cast<std::array<Element, Count>*>(array)->at(index));
        }

    protected:
        StdArrayInstance()
        {
            static std::string name = "std::array<" + std::string(Reflection::getType<Element>()->getName()) + ", " + std::to_string(Count) + ">";
            mName = name;
            mSize = sizeof(std::array<Element, Count>);
        }
        virtual ~StdArrayInstance() = default;
    };
}
