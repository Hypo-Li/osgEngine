#include "Entity.h"
#include "Component.h"

namespace xxx
{
    Entity::Entity(std::string&& name) :
        mName(name),
        mParent(nullptr),
        mOsgEntityNode(new EntityNode(this)),
        mOsgChildrenGroup(new osg::Group),
        mOsgComponentsGroup(new osg::Group)
	{
        mOsgEntityNode->addChild(mOsgChildrenGroup);
        mOsgEntityNode->addChild(mOsgComponentsGroup);
	}

	void Entity::appendChild(Entity* child)
	{
		if (child->mParent == this)
			return;
		if (child->mParent != nullptr)
			child->mParent->removeChild(child);
		child->mParent = this;
        mChildren.emplace_back(child);
        mOsgChildrenGroup->addChild(child->mOsgEntityNode);
	}

	void Entity::removeChild(Entity* child)
	{
		if (child->mParent != this)
			return;
		child->mParent = nullptr;
        for (auto it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            if (*it == child)
            {
                mChildren.erase(it);
                break;
            }
        }
        mOsgChildrenGroup->removeChild(child->mOsgEntityNode);
	}

	Entity* Entity::getChildByIndex(uint32_t index)
	{
		return dynamic_cast<Entity*>(mChildren.at(index).get());
	}

	void Entity::appendComponent(Component* component)
	{
		if (component->mOwner == this)
			return;
		if (component->mOwner != nullptr)
			component->mOwner->removeComponent(component);
		component->mOwner = this;
		mComponents.emplace_back(component);
        mOsgComponentsGroup->addChild(component->mOsgComponentGroup);
	}

	void Entity::removeComponent(Component* component)
	{
		if (component->mOwner != this)
			return;
		component->mOwner = nullptr;
        for (auto it = mComponents.begin(); it != mComponents.end(); ++it)
        {
            if (*it == component)
            {
                mComponents.erase(it);
                break;
            }
        }
		mOsgComponentsGroup->removeChild(component->mOsgComponentGroup);
	}

    Component* Entity::getComponentByIndex(uint32_t index)
    {
        return mComponents.at(index);
    }
}
