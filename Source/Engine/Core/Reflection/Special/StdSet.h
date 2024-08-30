#pragma once
#include "../Special.h"
#include "../Argument.h"

#include <set>

template <typename T>
struct container_traits<std::set<T>> {
    using type = T;
};

namespace xxx::refl
{
    class StdSet : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Set;
        }

        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount(void* instance) const = 0;
        virtual std::vector<const void*> getElementPtrs(void* instance) const = 0;

        virtual void insertElement(void* instance, void* element) const = 0;
        virtual void removeElement(void* instance, void* element) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::set>, int> = 0>
    class StdSetInstance : public StdSet
    {
        friend class Reflection;
        using ElementType = container_traits_t<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::set<ElementType>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::set<ElementType>*>(instance);
        }

        virtual Type* getElementType() const override
        {
            return Type::getType<ElementType>();
        }

        virtual size_t getElementCount(void* instance) const override
        {
            return static_cast<std::set<ElementType>*>(instance)->size();
        }

        virtual std::vector<const void*> getElementPtrs(void* instance) const override
        {
            std::vector<const void*> result;
            std::set<ElementType>* set = static_cast<std::set<ElementType>*>(instance);
            for (auto it = set->begin(); it != set->end(); ++it)
            {
                result.emplace_back(&(*it));
            }
            return result;
        }

        virtual void insertElement(void* instance, void* element) const override
        {
            std::set<ElementType>* set = static_cast<std::set<ElementType>*>(instance);
            set->insert(*(ElementType*)(element));
        }

        virtual void removeElement(void* instance, void* element) const override
        {
            std::set<ElementType>* set = static_cast<std::set<ElementType>*>(instance);
            auto findResult = set->find(*(ElementType*)(element));
            if (findResult != set->end())
                set->erase(findResult);
        }

    protected:
        StdSetInstance()
        {
            static std::string name = "std::set<" + std::string(Reflection::getType<ElementType>()->getName()) + ">";
            mName = name; // typeid(std::set<ElementType>).name();
            mSize = sizeof(std::set<ElementType>);
        }
    };
}
