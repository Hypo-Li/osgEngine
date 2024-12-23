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
        REFLECT_CLASS(Component)
    public:
        enum class Type
        {
            MeshRenderer,
            InstancedMeshRenderer,
            DirectionLight,
            ImageBasedLight,
            Count,
        };

        Component();
        Component(const Component& other);
        virtual ~Component() = default;

        virtual Type getType() const = 0;

        virtual void onEnable() {}

        virtual void onDisable() {}

        virtual void postLoad() override
        {
            onEnable();
        }

        inline Entity* getEntity() const
        {
            return mEntity;
        }

        inline osg::Node* getOsgNode() const
        {
            return mOsgComponentGroup;
        }

    protected:
        Entity* mEntity;

        osg::ref_ptr<osg::Group> mOsgComponentGroup;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Component>();
    }
}
