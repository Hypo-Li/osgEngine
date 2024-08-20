#pragma once
#include "../Special.h"

#include <utility>

namespace xxx::refl
{
    class StdPair : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Map;
        }

        virtual Type* getFirstType() const = 0;
        virtual Type* getSecondType() const = 0;
        virtual void* getFirstPtr(void* instance) const = 0;
        virtual void* getSecondPtr(void* instance) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::pair>, int> = 0>
    class StdPairInstance : public StdPair
    {
        friend class Reflection;
        using FirstType = container_traits_t1<T>;
        using SecondType = container_traits_t2<T>;
    public:
        virtual Type* getFirstType() const override
        {
            return Type::getType<FirstType>();
        }
        virtual Type* getSecondType() const override
        {
            return Type::getType<SecondType>();
        }
        virtual void* getFirstPtr(void* instance) const override
        {
            std::pair<FirstType, SecondType>* pair = static_cast<std::pair<FirstType, SecondType>*>(instance);
            return &pair->first;
        }
        virtual void* getSecondPtr(void* instance) const override
        {
            std::pair<FirstType, SecondType>* pair = static_cast<std::pair<FirstType, SecondType>*>(instance);
            return &pair->second;
        }

    protected:
        StdPairInstance()
        {
            mName = typeid(std::pair<FirstType, SecondType>).name();
            mSize = sizeof(std::pair<FirstType, SecondType>);
        }
    };
}
