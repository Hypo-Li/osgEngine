#pragma once
#include "../Special.h"

namespace xxx::refl
{
    template <typename T>
    struct container_traits<std::list<T>> {
        using type = T;
    };

    class StdList : public Special
    {
    public:
        virtual Type* getElementType() const = 0;
        virtual size_t getElementCount(const void* instance) const = 0;
        virtual std::vector<void*> getElementPtrs(void* instance) const = 0;
        virtual std::vector<const void*> getElementPtrs(const void* instance) const = 0;
        virtual void addElement(void* instance, const void* data) const = 0;

    protected:
        StdList(std::string_view name, size_t size) : Special(name, size, Case::StdList) {}
    };

    template <typename T> requires is_instance_of_v<T, std::list>
    class TStdList : public StdList
    {
        friend class Reflection;
        using Element = container_traits_t<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::list<Element>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::list<Element>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::list<Element>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::list<Element>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if (static_cast<const std::list<Element>*>(instance1)->size() == 0 &&
                static_cast<const std::list<Element>*>(instance2)->size() == 0)
                return true;
            return false;
        }

        virtual Type* getElementType() const override
        {
            return Reflection::getType<Element>();
        }

        virtual size_t getElementCount(const void* instance) const override
        {
            return static_cast<const std::list<Element>*>(instance)->size();
        }

        virtual std::vector<void*> getElementPtrs(void* instance) const override
        {
            std::list<Element>* list = static_cast<std::list<Element>*>(instance);
            std::vector<void*> result(list->size());
            size_t i = 0;
            for (Element& element : *list)
                result[i++] = &element;
            return result;
        }

        virtual std::vector<const void*> getElementPtrs(const void* instance) const override
        {
            const std::list<Element>* list = static_cast<const std::list<Element>*>(instance);
            std::vector<const void*> result(list->size());
            size_t i = 0;
            for (const Element& element : *list)
                result[i++] = &element;
            return result;
        }

        virtual void addElement(void* instance, const void* data) const override
        {
            static_cast<std::list<Element>*>(instance)->push_back(*static_cast<const Element*>(data));
        }

    private:
        static std::string_view getName()
        {
            static std::string name = "std::list<" + std::string(Reflection::getType<Element>()->getName()) + ">";
            return name;
        }

        TStdList() : StdList(getName(), sizeof(std::list<Element>)) {}
    };
}
