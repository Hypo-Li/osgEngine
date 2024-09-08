#include "Entity.h"

namespace xxx
{
    Entity::Entity(const std::string& name) :
        mName(name),
        mParent(nullptr),
        mOsgEntityNode(new EntityNode(this)),
        mOsgChildrenGroup(new osg::Group),
        mOsgComponentsGroup(new osg::Group)
	{
        mOsgEntityNode->addChild(mOsgChildrenGroup);
        mOsgEntityNode->addChild(mOsgComponentsGroup);
	}

    Entity::Entity(const Entity& other) :
        mName(other.mName + "_copy"),
        mParent(nullptr),
        mOsgEntityNode(new EntityNode(this)),
        mOsgChildrenGroup(new osg::Group),
        mOsgComponentsGroup(new osg::Group)
    {
        mOsgEntityNode->addChild(mOsgChildrenGroup);
        mOsgEntityNode->addChild(mOsgComponentsGroup);

        // copy components
        for (Component* component : other.mComponents)
            appendComponent(component->clone());
    }

    Entity& Entity::operator=(const Entity& other)
    {
        clearComponents();

        // copy components
        for (Component* component : other.mComponents)
            appendComponent(component->clone());
        return *this;
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
        mOsgChildrenGroup->removeChild(child->mOsgEntityNode);
        for (auto it = mChildren.begin(); it != mChildren.end(); ++it)
        {
            if (*it == child)
            {
                mChildren.erase(it);
                break;
            }
        }
	}

    void Entity::removeChild(uint32_t index)
    {
        if (index >= mChildren.size())
            return;
        Entity* child = mChildren[index];
        child->mParent = nullptr;
        mOsgChildrenGroup->removeChild(child->mOsgEntityNode);

        mChildren.erase(mChildren.begin() + index);
    }

	Entity* Entity::getChildByIndex(uint32_t index)
	{
        if (index >= mChildren.size())
            return nullptr;
        return mChildren[index].get();

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
		mOsgComponentsGroup->removeChild(component->mOsgComponentGroup);

        for (auto it = mComponents.begin(); it != mComponents.end(); ++it)
        {
            if (*it == component)
            {
                mComponents.erase(it);
                break;
            }
        }
	}

    void Entity::removeComponent(uint32_t index)
    {
        if (index >= mComponents.size())
            return;
        Component* component = mComponents[index];
        component->mOwner = nullptr;
        mOsgComponentsGroup->removeChild(component->mOsgComponentGroup);

        mComponents.erase(mComponents.begin() + index);
    }

    Component* Entity::getComponentByIndex(uint32_t index)
    {
        if (index >= mComponents.size())
            return nullptr;
        return mComponents[index].get();
    }

    void Entity::clearComponents()
    {
        for (auto it : mComponents)
        {
            it->mOwner = nullptr;
            mOsgComponentsGroup->removeChild(it->mOsgComponentGroup);
        }
        mComponents.clear();
    }

    void Entity::clearChildren()
    {
        for (auto it : mChildren)
        {
            it->mParent = nullptr;
            mOsgChildrenGroup->removeChild(it->mOsgEntityNode);
        }
        mChildren.clear();
    }
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Entity>()
    {
        Class* clazz = new ClassInstance<Entity>("Entity");
        Property* propName = clazz->addProperty("Name", &Entity::mName);
        Property* propParent = clazz->addProperty("Parent", &Entity::mParent);
        Property* propChildren = clazz->addProperty("Children", &Entity::mChildren);
        Property* propComponents = clazz->addProperty("Components", &Entity::mComponents);
        sRegisteredClassMap.emplace("Entity", clazz);
        return clazz;
    }
}
