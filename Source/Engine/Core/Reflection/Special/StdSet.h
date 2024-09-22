#pragma once
#include "../Special.h"

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
        using Element = container_traits_t<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::set<Element>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::set<Element>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::set<Element>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::set<Element>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return false;
        }

        virtual Type* getElementType() const override
        {
            return Type::getType<Element>();
        }

        virtual size_t getElementCount(void* instance) const override
        {
            return static_cast<std::set<Element>*>(instance)->size();
        }

        virtual std::vector<const void*> getElementPtrs(void* instance) const override
        {
            std::vector<const void*> result;
            std::set<Element>* set = static_cast<std::set<Element>*>(instance);
            for (auto it = set->begin(); it != set->end(); ++it)
            {
                result.emplace_back(&(*it));
            }
            return result;
        }

        virtual void insertElement(void* instance, void* element) const override
        {
            std::set<Element>* set = static_cast<std::set<Element>*>(instance);
            set->insert(*(Element*)(element));
        }

        virtual void removeElement(void* instance, void* element) const override
        {
            std::set<Element>* set = static_cast<std::set<Element>*>(instance);
            auto findResult = set->find(*(Element*)(element));
            if (findResult != set->end())
                set->erase(findResult);
        }

    protected:
        StdSetInstance()
        {
            static std::string name = "std::set<" + std::string(Reflection::getType<Element>()->getName()) + ">";
            mName = name;
            mSize = sizeof(std::set<Element>);
        }
    };
}
