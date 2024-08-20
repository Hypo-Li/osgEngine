#pragma once
#include "Entity.h"

namespace xxx
{
    class Component : public Object
    {
        friend class refl::Reflection;
        static Object* createInstance()
        {
            return nullptr;
        }
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Component>());
        }
        static const Component* getDefaultObject()
        {
            return nullptr;
        }

    private:
        friend class Entity;
    public:
        enum class Type
        {
            MeshRenderer,
            Count,
        };

        Component();
        virtual ~Component() = default;

        virtual Type getType() const = 0;

        inline Entity* getOwner() const { return mOwner; }
    private:
        Entity* mOwner;

        osg::ref_ptr<osg::Group> mOsgComponentGroup;
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<Component>()
        {
            Class* clazz = new Class("Component", sizeof(Component), Component::createInstance);
            clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
            Property* propOwner = clazz->addProperty("Owner", &Component::mOwner);
            sRegisteredClassMap.emplace("Component", clazz);
            return clazz;
        }
    }
}
