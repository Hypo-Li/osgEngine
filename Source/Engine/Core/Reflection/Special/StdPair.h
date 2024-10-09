#pragma once
#include "../Special.h"

#include <utility>

template <typename T1, typename T2>
struct container_traits<std::pair<T1, T2>> {
    using type1 = T1;
    using type2 = T2;
};

namespace xxx::refl
{
    class StdPair : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Pair;
        }

        virtual Type* getFirstType() const = 0;
        virtual Type* getSecondType() const = 0;
        virtual void* getFirstPtr(void* instance) const = 0;
        virtual const void* getFirstPtr(const void* instance) const = 0;
        virtual void* getSecondPtr(void* instance) const = 0;
        virtual const void* getSecondPtr(const void* instance) const = 0;
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_instance_of_v<T, std::pair>>>
    class StdPairInstance : public StdPair
    {
        friend class Reflection;
        using First = container_traits_t1<T>;
        using Second = container_traits_t2<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::pair<First, Second>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::pair<First, Second>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::pair<First, Second>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::pair<First, Second>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            const void* first1 = &(static_cast<const std::pair<First, Second>*>(instance1)->first);
            const void* first2 = &(static_cast<const std::pair<First, Second>*>(instance2)->first);
            const void* second1 = &(static_cast<const std::pair<First, Second>*>(instance1)->second);
            const void* second2 = &(static_cast<const std::pair<First, Second>*>(instance2)->second);
            return Reflection::getType<First>()->compare(first1, first2) && Type::getType<Second>()->compare(second1, second2);
        }

        virtual Type* getFirstType() const override
        {
            return Reflection::getType<First>();
        }

        virtual Type* getSecondType() const override
        {
            return Reflection::getType<Second>();
        }

        virtual void* getFirstPtr(void* instance) const override
        {
            return &(static_cast<std::pair<First, Second>*>(instance)->first);
        }

        virtual const void* getFirstPtr(const void* instance) const override
        {
            return &(static_cast<const std::pair<First, Second>*>(instance)->first);
        }

        virtual void* getSecondPtr(void* instance) const override
        {
            return &(static_cast<std::pair<First, Second>*>(instance)->second);
        }

        virtual const void* getSecondPtr(const void* instance) const override
        {
            return &(static_cast<const std::pair<First, Second>*>(instance)->second);
        }

    protected:
        StdPairInstance()
        {
            static std::string name = "std::pair<" + std::string(Reflection::getType<First>()->getName()) + ", " + std::string(Reflection::getType<Second>()->getName()) + ">";
            mName = name;
            mSize = sizeof(std::pair<First, Second>);
        }
    };
}
