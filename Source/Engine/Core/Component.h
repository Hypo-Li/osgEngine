#pragma once
#include "Entity.h"

namespace xxx
{
    class Component : public Object
    {
        friend class refl::Reflection;
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Component>());
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
            Class* clazz = new ClassInstance<Component>("Component");
            Property* propOwner = clazz->addProperty("Owner", &Component::mOwner);
            sRegisteredClassMap.emplace("Component", clazz);
            return clazz;
        }
    }
}
