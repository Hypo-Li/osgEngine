#pragma once
#include "Object.h"

#include <osg/ref_ptr>
#include <osg/Group>

namespace xxx
{
    class Entity;
    class Component : public Object
    {
        REFLECT_CLASS(Component)
    public:
        enum class Type
        {
            MeshRenderer,
            DirectionLight,
            ImageBasedLight,
            Count,
        };

        Component();
        Component(const Component& other);
        virtual ~Component() = default;

        virtual Type getType() const = 0;

        virtual bool onAddToEntity(Entity* entity);

        virtual bool onRemoveFromEntity(Entity* entity);

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                Entity* entity = mEntity;
                mEntity = nullptr;
                onAddToEntity(entity);
            }
        }

        inline Entity* getEntity() const
        {
            return mEntity;
        }

        osg::Node* getOsgNode() const
        {
            return mOsgComponentGroup;
        }

        virtual void hide()
        {
            mOsgComponentGroup->setNodeMask(0);
        }

        virtual void show()
        {
            mOsgComponentGroup->setNodeMask(0xFFFFFFFF);
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
