#pragma once
#include "../Special.h"

#include <vector>

namespace xxx::refl
{
    template <typename T>
    struct container_traits<std::vector<T>> {
        using type = T;
    };

    class StdVector : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Vector;
        }

        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount(const void* vector) const = 0;
        virtual void* getElementPtrByIndex(void* vector, size_t index) const = 0;
        virtual const void* getElementPtrByIndex(const void* vector, size_t index) const = 0;
        virtual void appendElement(void* vector, void* newElement) const = 0;
        virtual void removeElementByIndex(void* vector, size_t index) const = 0;
        virtual void resize(void* vector, size_t size) const = 0;

    protected:
        StdVector(std::string_view name, size_t size) : Special(name, size) {}
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_instance_of_v<T, std::vector>>>
    class StdVectorInstance : public StdVector
    {
        friend class Reflection;
        using Element = container_traits_t<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::vector<Element>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::vector<Element>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::vector<Element>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::vector<Element>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if (getElementCount(instance1) == 0 && getElementCount(instance2) == 0)
                return true;
            return false;
        }

        virtual Type* getElementType() const override
        {
            return Reflection::getType<Element>();
        }

        virtual size_t getElementCount(const void* vector) const override
        {
            return static_cast<const std::vector<Element>*>(vector)->size();
        }

        virtual void* getElementPtrByIndex(void* vector, size_t index) const override
        {
            return &(static_cast<std::vector<Element>*>(vector)->at(index));
        }

        virtual const void* getElementPtrByIndex(const void* vector, size_t index) const override
        {
            return &(static_cast<const std::vector<Element>*>(vector)->at(index));
        }

        virtual void appendElement(void* vector, void* element) const override
        {
            static_cast<std::vector<Element>*>(vector)->emplace_back(*(Element*)(element));
        }

        virtual void removeElementByIndex(void* vector, size_t index) const override
        {
            std::vector<Element>* vec = static_cast<std::vector<Element>*>(vector);
            vec->erase(vec->begin() + index);
        }

        virtual void resize(void* vector, size_t size) const override
        {
            static_cast<std::vector<Element>*>(vector)->resize(size);
        }

    protected:
        std::string_view genName() const
        {
            static std::string name = "std::vector<" + std::string(Reflection::getType<Element>()->getName()) + ">";
            return name;
        }

        StdVectorInstance() : StdVector(genName(), sizeof(std::vector<Element>)) {}
    };
}
