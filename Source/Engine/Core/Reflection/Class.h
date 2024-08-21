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

        xxx::Object* createInstance() const
        {
            return mConstructor();
        }

        const xxx::Object* getDefaultObject() const
        {
            return mDefaultObject;
        }

    protected:
        void setBaseClass(Class* baseClass)
        {
            mBaseClass = baseClass;
        }

        template <typename T>
        Property* addProperty(std::string_view name, T member)
        {
            Property* newProperty = new PropertyInstance(name, member);
            mProperties.push_back(newProperty);
            return newProperty;
        }

        template <typename T>
        Method* addMethod(std::string_view name, T member, std::initializer_list<std::string_view> paramNames)
        {
            Method* newMethod = new MethodInstance(name, member, paramNames);
            mMethods.push_back(newMethod);
            return newMethod;
        }

    private:
        using Constructor = std::function<xxx::Object* (void)>;
        Class(std::string_view name, size_t size, Constructor constructor) :
            Type(name, size),
            mBaseClass(nullptr),
            mConstructor(constructor),
            mDefaultObject(constructor())
        {}
        virtual ~Class() = default;

        Class* mBaseClass;
        std::vector<Property*> mProperties;
        std::vector<Method*> mMethods;
        Constructor mConstructor;
        const xxx::Object* mDefaultObject;
	};
}
