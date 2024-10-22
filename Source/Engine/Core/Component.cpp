#include "Component.h"
#include "Entity.h"

namespace xxx
{
    Component::Component() :
        mEntity(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }

    Component::Component(const Component& other) :
        mEntity(nullptr),
        mOsgComponentGroup(new osg::Group)
    {

    }

    bool Component::onAddToEntity(Entity* entity)
    {
        if (mEntity == entity)
            return false;
        if (mEntity != nullptr)
            mEntity->removeComponent(this);

        setOwner(entity);
        mEntity = entity;
        return true;
    }

    bool Component::onRemoveFromEntity(Entity* entity)
    {
        if (mEntity != entity)
            return false;
        setOwner(nullptr);
        mEntity = nullptr;
        return true;
    }

    namespace refl
    {
        template <> Type* Reflection::createType<Component>()
        {
            Class* clazz = new ClassInstance<Component>("Component");
            clazz->addProperty("Entity", &Component::mEntity);
            return clazz;
        }
    }
}
