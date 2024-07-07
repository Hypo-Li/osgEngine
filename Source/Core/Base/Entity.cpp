#include "Entity.h"
#include "Component.h"

namespace xxx
{
    Entity::Entity(std::string&& name) :
        osg::MatrixTransform(),
        _entityName(name), _entityParent(nullptr),
        _childrenEntitiesGroup(new osg::Group),
        _componentsGroup(new osg::Group),
        _position(osg::Vec3d(0.0, 0.0, 0.0)),
        _rotation(osg::Vec3d(0.0, 0.0, 0.0)),
        _scale(osg::Vec3d(1.0, 1.0, 1.0)),
        _needUpdateMatrix(false)
	{
		osg::Group::addChild(_childrenEntitiesGroup);
		osg::Group::addChild(_componentsGroup);
	}

	void Entity::appendChildEntity(Entity* child)
	{
		if (child->_entityParent == this)
			return;
		if (child->_entityParent != nullptr)
			child->_entityParent->removeChild(child);
		child->_entityParent = this;
        _childrenEntitiesGroup->addChild(child);
	}

	void Entity::removeChildEntity(Entity* child)
	{
		if (child->_entityParent != this)
			return;
		child->_entityParent = nullptr;
        _childrenEntitiesGroup->removeChild(child);
	}

	Entity* Entity::getChildEntity(uint32_t index)
	{
		return dynamic_cast<Entity*>(_childrenEntitiesGroup->getChild(index));
	}

	void Entity::appendComponent(Component* component)
	{
		if (component->_owner == this)
			return;
		if (component->_owner != nullptr)
			component->_owner->removeComponent(component);
		component->_owner = this;
		_componentsGroup->addChild(component);
	}

	void Entity::removeComponent(Component* component)
	{
		if (component->_owner != this)
			return;
		component->_owner = nullptr;
		_componentsGroup->removeChild(component);
	}

    Component* Entity::getComponent(uint32_t index)
    {
        return dynamic_cast<Component*>(_componentsGroup->getChild(index));
    }
}
