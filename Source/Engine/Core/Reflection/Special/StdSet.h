#pragma once
#include "../Special.h"

#include <set>

namespace xxx::refl
{
    template <typename T>
    struct container_traits<std::set<T>> {
        using type = T;
    };

    class StdSet : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Set;
        }

        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount(const void* instance) const = 0;
        virtual std::vector<const void*> getElementPtrs(const void* instance) const = 0;

        virtual void insertElement(void* instance, void* element) const = 0;
        virtual void removeElement(void* instance, void* element) const = 0;

    protected:
        StdSet(std::string_view name, size_t size) : Special(name, size) {}
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_instance_of_v<T, std::set>>>
    class TStdSet : public StdSet
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
            if (static_cast<const std::set<Element>*>(instance1)->size() == 0 && static_cast<const std::set<Element>*>(instance2)->size() == 0)
                return true;
            return false;
        }

        virtual Type* getElementType() const override
        {
            return Reflection::getType<Element>();
        }

        virtual size_t getElementCount(const void* instance) const override
        {
            return static_cast<const std::set<Element>*>(instance)->size();
        }

        virtual std::vector<const void*> getElementPtrs(const void* instance) const override
        {
            std::vector<const void*> result;
            const std::set<Element>* set = static_cast<const std::set<Element>*>(instance);
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
        std::string_view genName() const
        {
            static std::string name = "std::set<" + std::string(Reflection::getType<Element>()->getName()) + ">";
            return name;
        }

        TStdSet() : StdSet(genName(), sizeof(std::set<Element>)) {}
    };
}
