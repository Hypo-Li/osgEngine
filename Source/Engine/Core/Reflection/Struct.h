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

        virtual void* newInstance() const override
        {
            return mConstructor();
        }

        virtual void deleteInstance(void* instance) const override
        {
            mDestructor(instance);
        }

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
        template <std::size_t Index = 0, typename ClassType, typename ObjectType>
        Property* addProperty(std::string_view name, ObjectType ClassType::* member)
        {
            Property* newProperty = new PropertyValueInstance<ClassType, ObjectType, Index>(name, member);
            mProperties.push_back(newProperty);
            return newProperty;
        }

        template <typename Getter, typename Setter,
            std::enable_if_t<is_instance_of_v<Getter, std::function> && is_instance_of_v<Setter, std::function>, int> = 0>
        Property* addProperty(std::string_view name, Getter getter, Setter setter)
        {
            Property* newProperty = new PropertyAccessorInstance(name, getter, setter);
            mProperties.push_back(newProperty);
            return newProperty;
        }

    private:
        using Constructor = std::function<void* (void)>;
        using Destructor = std::function<void(void*)>;
        Struct(std::string_view name, size_t size, Constructor constructor, Destructor destructor) :
            Type(name, size),
            mConstructor(constructor),
            mDestructor(destructor)
        {}
        virtual ~Struct() = default;

        std::vector<Property*> mProperties;
        Constructor mConstructor;
        Destructor mDestructor;
    };
}
