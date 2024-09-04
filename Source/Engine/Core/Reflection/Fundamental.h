#pragma once
#include "Type.h"

namespace xxx::refl
{
    class Reflection;
    class Fundamental : public Type
    {
        friend class Reflection;
    public:
        virtual Kind getKind() const override
        {
            return Kind::Fundamental;
        }

    protected:
        Fundamental(std::string_view name, size_t size) :
            Type(name, size)
        {}
    };

    template <typename T, typename = std::enable_if_t<std::is_fundamental_v<T>>>
    class FundamentalInstance : public Fundamental
    {
        friend class Reflection;
    public:
        virtual void* newInstance() const override
        {
            return new T;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<T*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new T[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<T*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return *static_cast<const T*>(instance1) == *static_cast<const T*>(instance2);
        }

    protected:
        FundamentalInstance(std::string_view name) :
            Fundamental(name, sizeof(T))
        {}
    };
}
