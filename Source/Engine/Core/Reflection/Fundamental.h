#pragma once
#include "Type.h"

namespace xxx::refl
{
    class Reflection;
    class Fundamental : public Type
    {
    protected:
        Fundamental(std::string_view name, size_t size) :
            Type(name, size, Kind::Fundamental)
        {}
    };

    template <typename T, typename = std::enable_if_t<std::is_fundamental_v<T>>>
    class FundamentalInstance : public Fundamental
    {
        friend class Reflection;
    public:
        virtual void* newInstance() const override
        {
            if constexpr (std::is_void_v<T>)
                return nullptr;
            else
                return new T;
        }

        virtual void deleteInstance(void* instance) const override
        {
            if constexpr (!std::is_void_v<T>)
                delete static_cast<T*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            if constexpr (std::is_void_v<T>)
                return nullptr;
            else
                return new T[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            if constexpr (!std::is_void_v<T>)
                delete[] static_cast<T*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if constexpr (std::is_void_v<T>)
                return false;
            else
                return *static_cast<const T*>(instance1) == *static_cast<const T*>(instance2);
        }

    protected:
        template <typename T>
        static constexpr size_t sizeOf()
        {
            if constexpr (std::is_void_v<T>)
                return 0;
            else
                return sizeof(T);
        }

        FundamentalInstance(std::string_view name) :
            Fundamental(name, sizeOf<T>())
        {}
    };
}
