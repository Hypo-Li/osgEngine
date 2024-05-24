#include "Entity.h"
#include "Component.h"

namespace xxx
{
    Entity::Entity(std::string&& name) :
        osg::MatrixTransform(),
        _name(name), _parent(nullptr),
        _childrenGroup(new osg::Group),
        _componentsGroup(new osg::Group),
        _position(osg::Vec3d(0.0, 0.0, 0.0)),
        _rotation(osg::Quat(0.0, 0.0, 0.0, 1.0)),
        _scale(osg::Vec3d(1.0, 1.0, 1.0)),
        _needUpdateMatrix(false)
	{
		osg::Group::addChild(_childrenGroup);
		osg::Group::addChild(_componentsGroup);
	}

	void Entity::appendChild(Entity* child)
	{
		if (child->_parent == this)
			return;
		if (child->_parent != nullptr)
			child->_parent->removeChild(child);
		child->_parent = this;
		_childrenGroup->addChild(child);
	}

	void Entity::removeChild(Entity* child)
	{
		if (child->_parent != this)
			return;
		child->_parent = nullptr;
		_childrenGroup->removeChild(child);
	}

	Entity* Entity::getChild(uint32_t index)
	{
		return dynamic_cast<Entity*>(_childrenGroup->getChild(index));
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
}
