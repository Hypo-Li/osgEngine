#include "Entity.h"

namespace xxx
{
    class UpdatePreFrameTransformMatrixCallback : public osg::NodeCallback
    {
    public:
        UpdatePreFrameTransformMatrixCallback(osg::Matrixd* preFrameTransformMatrix) :
            mPreFrameTransformMatrix(preFrameTransformMatrix) {}

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            *mPreFrameTransformMatrix = mTempMatrix;
            mTempMatrix = dynamic_cast<osg::MatrixTransform*>(node)->getMatrix();
        }

    protected:
        osg::Matrixd* mPreFrameTransformMatrix;
        osg::Matrixd mTempMatrix;
    };

    Entity::Entity(const std::string& name) :
        mName(name),
        mParent(nullptr),
        mOsgEntityNode(new EntityNode(this)),
        mOsgChildrenGroup(new osg::Group),
        mOsgComponentsGroup(new osg::Group)
	{
        mOsgEntityNode->addChild(mOsgChildrenGroup);
        mOsgEntityNode->addChild(mOsgComponentsGroup);
        mOsgEntityNode->addEventCallback(new UpdatePreFrameTransformMatrixCallback(&mPreFrameTransformMatrix));
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
        mOsgEntityNode->addEventCallback(new UpdatePreFrameTransformMatrixCallback(&mPreFrameTransformMatrix));

        // copy components
        for (Component* component : other.mComponents)
            addComponent(component->clone());
    }

    void Entity::setParent(Entity* entity)
    {
        if (mParent == entity)
            return;
        if (mParent)
            mParent->removeChild(this);
        if (entity)
            entity->addChild(this);
        else
            mParent = nullptr;
    }

	void Entity::addChild(Entity* child)
	{
		if (child->mParent == this)
			return;
		if (child->mParent != nullptr)
			child->mParent->removeChild(child);

        child->setOwner(this);
		child->mParent = this;
        mChildren.emplace_back(child);
        mOsgChildrenGroup->addChild(child->mOsgEntityNode);
	}

    Entity* Entity::getChild(uint32_t index)
    {
        if (index >= mChildren.size())
            return nullptr;
        return mChildren[index].get();
    }

	void Entity::removeChild(Entity* child)
	{
		if (child->mParent != this)
			return;
		child->mParent = nullptr;
        child->setOwner(nullptr);
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
        child->setOwner(nullptr);
        child->mParent = nullptr;
        mOsgChildrenGroup->removeChild(child->mOsgEntityNode);

        mChildren.erase(mChildren.begin() + index);
    }

    void Entity::removeChildren(uint32_t beginIndex, uint32_t count)
    {
        uint32_t i;
        for (i = 0; i < count; ++i)
        {
            if (beginIndex + i >= mChildren.size())
                break;
            Entity* child = mChildren[beginIndex + i];
            child->setOwner(nullptr);
            child->mParent = nullptr;
        }
        mOsgChildrenGroup->removeChildren(beginIndex, i);
        mChildren.erase(mChildren.begin() + beginIndex, mChildren.begin() + beginIndex + i);
    }

    void Entity::clearChildren()
    {
        for (auto it : mChildren)
        {
            it->setOwner(nullptr);
            it->mParent = nullptr;
            mOsgChildrenGroup->removeChild(it->mOsgEntityNode);
        }
        mChildren.clear();
    }

	void Entity::addComponent(Component* component)
	{
        if (component->mEntity == this)
            return;
        if (component->mEntity)
            component->mEntity->removeComponent(component);
        component->mEntity = this;
        component->setOwner(this);
        component->onEnable();
		mComponents.emplace_back(component);
        mOsgComponentsGroup->addChild(component->getOsgNode());
	}

    Entity::Components::const_iterator Entity::removeComponent(Component* component)
	{
        if (component->mEntity == this)
        {
            component->onDisable();
            component->mEntity = nullptr;
            component->setOwner(nullptr);
            mOsgComponentsGroup->removeChild(component->getOsgNode());
            for (auto it = mComponents.begin(); it != mComponents.end(); ++it)
                if (*it == component)
                    return mComponents.erase(it);
        }
        return mComponents.end();
	}

    Entity::Components::const_iterator Entity::removeComponent(uint32_t index)
    {
        if (index >= mComponents.size())
            return mComponents.end();
        Component* component = mComponents[index];
        component->onDisable();
        component->mEntity = nullptr;
        component->setOwner(nullptr);
        mOsgComponentsGroup->removeChild(component->getOsgNode());

        return mComponents.erase(mComponents.begin() + index);
    }

    void Entity::clearComponents()
    {
        for (auto it : mComponents)
        {
            it->onDisable();
            it->mEntity = nullptr;
            it->setOwner(nullptr);
            mOsgComponentsGroup->removeChild(it->getOsgNode());
        }
        mComponents.clear();
    }
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Entity>()
    {
        Class* clazz = new ClassInstance<Entity>("Entity");
        clazz->addProperty("Name", &Entity::mName);
        clazz->addProperty("Parent", &Entity::mParent);
        clazz->addProperty("Children", &Entity::mChildren);
        clazz->addProperty("Components", &Entity::mComponents);
        return clazz;
    }
}
