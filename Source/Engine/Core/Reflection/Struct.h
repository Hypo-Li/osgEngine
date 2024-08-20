#pragma once
#include "Property.h"

#include <vector>

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
        template <typename T>
        Property* addProperty(std::string_view name, T member)
        {
            Property* newProperty = new PropertyInstance(name, member);
            mProperties.push_back(newProperty);
            return newProperty;
        }

        template <typename Getter, typename Setter>
        Property* addProperty(std::string_view name, Getter getter, Setter setter)
        {
            Property* newProperty = new PropertyInstance(name, getter, setter);
            mProperties.push_back(newProperty);
            return newProperty;
        }

    private:
        Struct(std::string_view name, size_t size) : Type(name, size) {}
        virtual ~Struct() = default;

        std::vector<Property*> mProperties;
    };
}
