#pragma once
#include "../Special.h"

#include <array>

namespace xxx::refl
{
    template <typename T, size_t N>
    struct container_traits<std::array<T, N>> {
        using type = T;
        static constexpr size_t Count = N;
    };

    class StdArray : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Array;
        }
        virtual void setElementValue(void* instance, size_t index, void* data) const = 0;
        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount() const = 0;
        virtual void* getElementPtrByIndex(void* instance, size_t index) const = 0;

    protected:
        StdArray(std::string_view name, size_t size) : Special(name, size) {}
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_std_array_v<T>>>
    class TStdArray : public StdArray
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

        virtual void setElementValue(void* instance, size_t index, void* data) const override
        {
            static_cast<std::array<Element, Count>*>(instance)->at(index) = *(Element*)(data);
        }

        virtual Type* getElementType() const override
        {
            return Reflection::getType<Element>();
        }

        virtual size_t getElementCount() const override
        {
            return Count;
        }

        virtual void* getElementPtrByIndex(void* instance, size_t index) const override
        {
            return &(static_cast<std::array<Element, Count>*>(instance)->at(index));
        }

    protected:
        std::string_view genName() const
        {
            static std::string name = "std::array<" + std::string(Reflection::getType<Element>()->getName()) + ", " + std::to_string(Count) + ">";
            return name;
        }

        TStdArray() : StdArray(genName(), sizeof(std::array<Element, Count>)) {}
        virtual ~TStdArray() = default;
    };
}
