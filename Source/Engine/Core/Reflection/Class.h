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

        xxx::Object* newInstance() const
        {
            return mConstructor();
        }

        void deleteInstance(xxx::Object* instance) const
        {
            mDestructor(instance);
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

        template <std::size_t Index = 0, typename ClassType, typename ObjectType>
        Property* addProperty(std::string_view name, ObjectType ClassType::* member)
        {
            Property* newProperty = new PropertyValueInstance<ClassType, ObjectType, Index>(name, member);
            mProperties.push_back(newProperty);
            return newProperty;
        }

        template <typename Getter, typename Setter,
            std::enable_if_t<is_instance_of_v<Getter, std::function>&& is_instance_of_v<Setter, std::function>, int> = 0>
        Property* addProperty(std::string_view name, Getter getter, Setter setter)
        {
            Property* newProperty = new PropertyAccessorInstance(name, getter, setter);
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
        using Destructor = std::function<void(xxx::Object*)>;
        Class(std::string_view name, size_t size, Constructor constructor, Destructor destructor) :
            Type(name, size),
            mBaseClass(nullptr),
            mConstructor(constructor),
            mDestructor(destructor),
            mDefaultObject(constructor())
        {}
        virtual ~Class() = default;

        Class* mBaseClass;
        std::vector<Property*> mProperties;
        std::vector<Method*> mMethods;
        Constructor mConstructor;
        Destructor mDestructor;
        const xxx::Object* mDefaultObject;
	};
}
