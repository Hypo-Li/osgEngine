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

    void Component::onAddToEntity(Entity* entity)
    {
        if (mEntity == entity)
            return;
        if (mEntity != nullptr)
            mEntity->removeComponent(this);

        setOwner(entity);
        mEntity = entity;
    }

    void Component::onRemoveFromEntity(Entity* entity)
    {
        if (mEntity != entity)
            return;
        setOwner(nullptr);
        mEntity = nullptr;
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
