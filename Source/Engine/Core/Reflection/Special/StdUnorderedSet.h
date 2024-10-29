#pragma once
#include "../Special.h"

#include <unordered_set>

namespace xxx::refl
{
    template <typename T>
    struct container_traits<std::unordered_set<T>> {
        using type = T;
    };

    class StdUnorderedSet : public Special
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
        StdUnorderedSet(std::string_view name, size_t size) : Special(name, size) {}
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_instance_of_v<T, std::unordered_set>>>
    class TStdUnorderedSet : public StdUnorderedSet
    {
        friend class Reflection;
        using Element = container_traits_t<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::unordered_set<Element>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::unordered_set<Element>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::unordered_set<Element>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::unordered_set<Element>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if (static_cast<const std::unordered_set<Element>*>(instance1)->size() == 0 && static_cast<const std::unordered_set<Element>*>(instance2)->size() == 0)
                return true;
            return false;
        }

        virtual Type* getElementType() const override
        {
            return Reflection::getType<Element>();
        }

        virtual size_t getElementCount(const void* instance) const override
        {
            return static_cast<const std::unordered_set<Element>*>(instance)->size();
        }

        virtual std::vector<const void*> getElementPtrs(const void* instance) const override
        {
            std::vector<const void*> result;
            const std::unordered_set<Element>* unorderedSet = static_cast<const std::unordered_set<Element>*>(instance);
            for (auto it = unorderedSet->begin(); it != unorderedSet->end(); ++it)
            {
                result.emplace_back(&(*it));
            }
            return result;
        }

        virtual void insertElement(void* instance, void* element) const override
        {
            std::unordered_set<Element>* unorderedSet = static_cast<std::unordered_set<Element>*>(instance);
            unorderedSet->insert(*(Element*)(element));
        }

        virtual void removeElement(void* instance, void* element) const override
        {
            std::unordered_set<Element>* unorderedSet = static_cast<std::unordered_set<Element>*>(instance);
            auto findResult = unorderedSet->find(*(Element*)(element));
            if (findResult != unorderedSet->end())
                unorderedSet->erase(findResult);
        }

    protected:
        std::string_view genName() const
        {
            static std::string name = "std::unordered_set<" + std::string(Reflection::getType<Element>()->getName()) + ">";
            return name;
        }

        TStdUnorderedSet() : StdUnorderedSet(genName(), sizeof(std::unordered_set<Element>)) {}
    };
}
