#pragma once
#include "../Special.h"
#include "../Argument.h"

#include <vector>

template <typename T>
struct container_traits<std::vector<T>> {
    using type = T;
};

namespace xxx::refl
{
    class StdVector : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Vector;
        }

        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount(void* vector) const = 0;
        virtual void* getElementPtrByIndex(void* vector, size_t index) const = 0;
        virtual void appendElement(void* vector, void* newElement) const = 0;
        virtual void removeElementByIndex(void* vector, size_t index) const = 0;
        virtual void resize(void* vector, size_t size) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::vector>, int> = 0>
    class StdVectorInstance : public StdVector
    {
        friend class Reflection;
        using ElementType = container_traits_t<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::vector<ElementType>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::vector<ElementType>*>(instance);
        }

        virtual Type* getElementType() const override
        {
            return Type::getType<ElementType>();
        }

        virtual size_t getElementCount(void* vector) const override
        {
            return static_cast<std::vector<ElementType>*>(vector)->size();
        }

        virtual void* getElementPtrByIndex(void* vector, size_t index) const override
        {
            return &(static_cast<std::vector<ElementType>*>(vector)->at(index));
        }

        virtual void appendElement(void* vector, void* element) const override
        {
            static_cast<std::vector<ElementType>*>(vector)->emplace_back(*(ElementType*)(element));
        }

        virtual void removeElementByIndex(void* vector, size_t index) const override
        {
            std::vector<ElementType>* vec = static_cast<std::vector<ElementType>*>(vector);
            vec->erase(vec->begin() + index);
        }

        virtual void resize(void* vector, size_t size) const override
        {
            static_cast<std::vector<ElementType>*>(vector)->resize(size);
        }


    protected:
        StdVectorInstance()
        {
            static std::string name = "std::vector<" + std::string(Reflection::getType<ElementType>()->getName()) + ">";
            mName = name; // typeid(std::vector<ElementType>).name();
            mSize = sizeof(std::vector<ElementType>);
        }
    };
}
