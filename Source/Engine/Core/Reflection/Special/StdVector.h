#pragma once
#include "../Special.h"
#include "../Argument.h"

#include <vector>

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
        virtual void* getElementPtrByIndex(void* vector, uint32_t index) const = 0;
        virtual void appendElement(void* vector, Argument newElement) const = 0;
        virtual void removeElementByIndex(void* vector, uint32_t index) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::vector>, int> = 0>
    class StdVectorInstance : public StdVector
    {
        friend class Reflection;
        using ElementType = container_traits_t<T>;
    public:
        virtual Type* getElementType() const override
        {
            return Type::getType<ElementType>();
        }

        virtual size_t getElementCount(void* vector) const override
        {
            return static_cast<std::vector<ElementType>*>(vector)->size();
        }

        virtual void* getElementPtrByIndex(void* vector, uint32_t index) const override
        {
            return &(static_cast<std::vector<ElementType>*>(vector)->at(index));
        }

        virtual void appendElement(void* vector, Argument newElement) const override
        {
            //if constexpr (std::is_base_of_v<Object, std::remove_pointer_t<ElementType>>)
            static_cast<std::vector<ElementType>*>(vector)->emplace_back(newElement.getValue<ElementType>());
        }

        virtual void removeElementByIndex(void* vector, uint32_t index) const override
        {
            std::vector<ElementType>* vec = static_cast<std::vector<ElementType>*>(vector);
            vec->erase(vec->begin() + index);
        }

    protected:
        StdVectorInstance()
        {
            mName = typeid(std::vector<ElementType>).name();
            mSize = sizeof(std::vector<ElementType>);
        }
    };
}
