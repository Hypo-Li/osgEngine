#pragma once
#include "Object.h"

#include <osg/ref_ptr>
#include <osg/Group>

namespace xxx
{
    class Entity;
    class Component : public Object
    {
        friend class Entity;
        friend class refl::Reflection;
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Component>());
        }

        virtual Component* clone() const override
        {
            return nullptr;
        }

    public:
        enum class Type
        {
            MeshRenderer,
            Count,
        };

        Component();
        Component(const Component& other);
        virtual ~Component() = default;

        virtual Type getType() const = 0;

        inline Entity* getOwner() const
        {
            return mOwner;
        }

    protected:
        Entity* mOwner;

        osg::ref_ptr<osg::Group> mOsgComponentGroup;
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<Component>()
        {
            Class* clazz = new ClassInstance<Component>("Component");
            Property* propOwner = clazz->addProperty("Owner", &Component::mOwner);
            sRegisteredClassMap.emplace("Component", clazz);
            return clazz;
        }
    }
}
