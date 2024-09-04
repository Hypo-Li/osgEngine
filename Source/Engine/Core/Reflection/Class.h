#pragma once
#include "Property.h"
#include "Method.h"

namespace xxx
{
    class Object;
}

namespace xxx::refl
{
    class Reflection;
	class Class : public Type
	{
        friend class Reflection;
	public:
        virtual Kind getKind() const override { return Kind::Class; }

        Class* getBaseClass() const
        {
            return mBaseClass;
        }

        Property* getProperty(std::string_view name) const
        {
            for (auto prop : mProperties)
                if (prop->getName() == name)
                    return prop;
            return nullptr;
        }

        Method* getMethod(std::string_view name) const
        {
            for (auto meth : mMethods)
                if (meth->getName() == name)
                    return meth;
            return nullptr;
        }

        const std::vector<Property*>& getProperties() const
        {
            return mProperties;
        }

        const std::vector<Method*>& getMethods() const
        {
            return mMethods;
        }

        const xxx::Object* getDefaultObject() const
        {
            return mDefaultObject;
        }

    protected:
        template <std::size_t Index = 0, typename Owner, typename Declared>
        Property* addProperty(std::string_view name, Declared Owner::* member)
        {
            Property* newProperty = new PropertyMemberInstance<Owner, Declared, Index>(name, member);
            mProperties.emplace_back(newProperty);
            return newProperty;
        }

        template <typename Getter, typename Setter,
            std::enable_if_t<is_instance_of_v<Getter, std::function>&& is_instance_of_v<Setter, std::function>, int> = 0>
        Property* addProperty(std::string_view name, Getter getter, Setter setter)
        {
            Property* newProperty = new PropertyAccessorInstance(name, getter, setter);
            mProperties.emplace_back(newProperty);
            return newProperty;
        }

        template <typename T>
        Method* addMethod(std::string_view name, T member, std::initializer_list<std::string_view> paramNames)
        {
            Method* newMethod = new MethodInstance(name, member, paramNames);
            mMethods.emplace_back(newMethod);
            return newMethod;
        }

    protected:
        Class(std::string_view name, size_t size) :
            Type(name, size)
        {}

        Class* mBaseClass;
        std::vector<Property*> mProperties;
        std::vector<Method*> mMethods;
        xxx::Object* mDefaultObject;
	};

    template <typename T, typename Base = Object, typename = std::enable_if_t<std::is_base_of_v<Object, Base> && std::is_base_of_v<Base, T>>>
    class ClassInstance : public Class
    {
        friend class Reflection;
    public:
        virtual void* newInstance() const override
        {
            if constexpr (std::is_abstract_v<T>)
                return nullptr;
            else
                return new T;
        }

        virtual void deleteInstance(void* instance) const override
        {
            if constexpr (!std::is_abstract_v<T>)
                delete static_cast<T*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            if constexpr (std::is_abstract_v<T>)
                return nullptr;
            else
                return new T[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            if constexpr (!std::is_abstract_v<T>)
                delete[] static_cast<T*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if constexpr (is_comparable_v<T>)
                return *static_cast<const T*>(instance1) == *static_cast<const T*>(instance2);
            else
            {
                Class* clazz = const_cast<Class*>(dynamic_cast<const Class*>(this));
                while (clazz)
                {
                    const std::vector<Property*>& properties = clazz->getProperties();
                    for (Property* prop : properties)
                    {
                        if (!prop->compare(instance1, instance2))
                            return false;
                    }
                    clazz = clazz->getBaseClass();
                }
                return true;
            }
        }

    protected:
        ClassInstance(std::string_view name) :
            Class(name, sizeof(T))
        {
            if constexpr (std::is_same_v<T, Object>)
                mBaseClass = nullptr;
            else
                mBaseClass = dynamic_cast<Class*>(Type::getType<Base>());

            if constexpr (std::is_abstract_v<T>)
                mDefaultObject = nullptr;
            else
                mDefaultObject = new T;
        }
    };
}
