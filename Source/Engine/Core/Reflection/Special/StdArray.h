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
    template <typename T, std::enable_if_t<is_std_array_v<T>, int> = 0>
    class StdArrayInstance : public StdArray
    {
        friend class Reflection;
        using ElementType = container_traits_t<T>;
        static constexpr size_t ElementCount = container_traits<T>::Count;
    public:
        virtual Type* getElementType() const override
        {
            return Type::getType<ElementType>();
        }

        virtual size_t getElementCount() const override
        {
            return ElementCount;
        }

        virtual void* getElementPtrByIndex(void* array, uint32_t index) const override
        {
            return &(static_cast<std::array<ElementType, ElementCount>*>(array)->at(index));
        }

    protected:
        StdArrayInstance()
        {
            static std::string name = "std::array<" + std::string(Reflection::getType<ElementType>()->getName()) + ", " + std::to_string(ElementCount) + ">";
            mName = name; // typeid(std::array<ElementType, ElementCount>).name();
            mSize = sizeof(std::array<ElementType, ElementCount>);
        }
        virtual ~StdArrayInstance() = default;
    };
}
