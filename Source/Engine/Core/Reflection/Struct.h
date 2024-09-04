#pragma once
#include "Property.h"

#include <vector>

namespace xxx
{
    class Object;
}

namespace xxx::refl
{
    class Reflection;
    class Struct : public Type
    {
        friend class Reflection;
    public:
        virtual Kind getKind() const override { return Kind::Struct; }

        Property* getProperty(std::string_view name) const
        {
            for (auto prop : mProperties)
                if (prop->getName() == name)
                    return prop;
            return nullptr;
        }

        const std::vector<Property*>& getProperties() const
        {
            return mProperties;
        }

    protected:
        template <std::size_t Index = 0, typename Owner, typename Declared>
        Property* addProperty(std::string_view name, Declared Owner::* member)
        {
            Property* newProperty = new PropertyInstance<Owner, Declared, Index>(name, member);
            mProperties.emplace_back(newProperty);
            return newProperty;
        }

    protected:
        Struct(std::string_view name, size_t size) :
            Type(name, size)
        {}

        std::vector<Property*> mProperties;
    };

    template <typename T, typename = std::enable_if_t<!std::is_base_of_v<Object, T>>>
    class StructInstance : public Struct
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
            if constexpr (is_comparable_v<T>)
                return *static_cast<const T*>(instance1) == *static_cast<const T*>(instance2);
            else
            {
                for (Property* prop : mProperties)
                {
                    if (!prop->compare(instance1, instance2))
                        return false;
                }
                return true;
            }
        }

    protected:
        StructInstance(std::string_view name) :
            Struct(name, sizeof(T))
        {}
    };
}
