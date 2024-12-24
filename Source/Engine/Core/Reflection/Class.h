#pragma once
#include "Property.h"

namespace xxx
{
    class Object;
}

namespace xxx::refl
{
	class Class : public Type
	{
        friend class Reflection;
	public:
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

        const std::vector<Property*>& getProperties() const
        {
            return mProperties;
        }

        const xxx::Object* getDefaultObject() const
        {
            return mDefaultObject;
        }

        bool isDerivedFrom(Class* clazz) const
        {
            Class* baseClass = mBaseClass;
            while (baseClass)
            {
                if (baseClass == clazz)
                    return true;
                baseClass = baseClass->mBaseClass;
            }
            return false;
        }

        bool isBaseOf(Class* clazz) const
        {
            Class* baseClass = clazz->mBaseClass;
            while (baseClass)
            {
                if (baseClass == this)
                    return true;
                baseClass = baseClass->mBaseClass;
            }
            return false;
        }

        const std::vector<Class*>& getDerivedClasses() const
        {
            return mDerivedClasses;
        }

        static void addDerivedToBase(Class* baseClass, Class* derivedClass)
        {
            baseClass->mDerivedClasses.push_back(derivedClass);
        }

    protected:
        template <typename Declared, typename Owner>
        Property* addProperty(std::string_view name, Declared Owner::* member)
        {
            Property* newProperty = new TProperty<Declared, Owner>(name, member);
            mProperties.emplace_back(newProperty);
            return newProperty;
        }

    protected:
        Class(std::string_view name, size_t size, Class* baseClass, Object* defaultObject) :
            Type(name, size, Kind::Class),
            mBaseClass(baseClass),
            mDefaultObject(defaultObject)
        {
            Reflection::registerClass(name, this);
        }

        Class* mBaseClass;
        std::vector<Class*> mDerivedClasses;
        std::vector<Property*> mProperties;
        xxx::Object* mDefaultObject;
	};

    template <typename T, typename Base = Object, typename = std::enable_if_t<std::is_base_of_v<Object, Base> && std::is_base_of_v<Base, T>>>
    class TClass : public Class
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
            if constexpr (Comparable<T>)
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
        static Class* getSelfBaseClass()
        {
            if constexpr (std::is_same_v<T, Object>)
                return nullptr;
            else
                return dynamic_cast<Class*>(Reflection::getType<Base>());
        }

        static Object* getSelfDefaultObject()
        {
            if constexpr (std::is_abstract_v<T>)
                return nullptr;
            else
                return new T;
        }

        TClass(std::string_view name) :
            Class(name, sizeof(T), getSelfBaseClass(), getSelfDefaultObject())
        {
            if constexpr (!std::is_same_v<T, Object>)
                addDerivedToBase(getBaseClass(), this);
        }
    };
}
