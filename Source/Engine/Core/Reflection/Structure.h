#pragma once
#include "Property.h"

namespace xxx
{
    class Object;
}

namespace xxx::refl
{
    class Reflection;
    class Structure : public Type
    {
        friend class Reflection;
    public:
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
        Structure(std::string_view name, size_t size) :
            Type(name, size, Kind::Structure) {}

        template <typename Declared, typename Owner>
        Property* addProperty(std::string_view name, Declared Owner::* member)
        {
            Property* newProperty = new TProperty<Declared, Owner>(name, member);
            mProperties.emplace_back(newProperty);
            return newProperty;
        }

    private:
        std::vector<Property*> mProperties;
    };

    template <typename T> requires (!std::is_base_of_v<xxx::Object, T>)
    class TStructure : public Structure
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
            if constexpr (Comparable<T>)
                return *static_cast<const T*>(instance1) == *static_cast<const T*>(instance2);
            else
            {
                for (Property* prop : getProperties())
                {
                    if (!prop->compare(instance1, instance2))
                        return false;
                }
                return true;
            }
        }

    protected:
        TStructure(std::string_view name) :
            Structure(name, sizeof(T)) {}
    };
}
