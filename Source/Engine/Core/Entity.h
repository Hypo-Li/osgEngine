#pragma once
#include "Component.h"

#include <osg/MatrixTransform>

namespace xxx
{
    class Entity;
    class EntityNode : public osg::MatrixTransform
    {
        friend class Entity;
    public:
        EntityNode(Entity* entity) : mEntity(entity) {}

        Entity* getEntity() const { return mEntity; }

    private:
        Entity* mEntity;
    };

    class Prefab;

	class Entity : public Object
	{
        friend class Prefab;
        REFLECT_CLASS(Entity)
	public:
		Entity(const std::string& name = "");
        Entity(const Entity& other);
		virtual ~Entity() = default;

        virtual void postSerialize(Serializer* serializer) override
        {
            if (serializer->isLoader())
            {
                for (Entity* child : mChildren)
                    mOsgChildrenGroup->addChild(child->mOsgEntityNode);
                for (Component* component : mComponents)
                    mOsgComponentsGroup->addChild(component->mOsgComponentGroup);
                updateTransform();
            }
        }

        void setName(const std::string& name)
        {
            mName = name;
        }

        const std::string& getName() const
        {
            return mName;
        }

        osg::Node* getOsgNode() const
        {
            return mOsgEntityNode;
        }

        void setParent(Entity* entity);

		Entity* getParent() const
        {
            return mParent;
        }

		void addChild(Entity* child);

		void removeChild(Entity* child);

        void removeChild(uint32_t index);

        void removeChildren(uint32_t beginIndex, uint32_t count);

        Entity* getChild(uint32_t index);

        const std::vector<osg::ref_ptr<Entity>>& getChildren() const
        {
            return mChildren;
        }

        uint32_t getChildrenCount() const
        {
            return mChildren.size();
        }

        void clearChildren();

		void addComponent(Component* component);

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
        T* addComponent()
        {
            T* component = new T;
            this->addComponent(component);
            return component;
        }

		void removeComponent(Component* component);

        void removeComponent(uint32_t index);
        
		template<typename T = Component, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
		T* getComponent(uint32_t index)
		{
            if constexpr (std::is_same_v<T, Component>)
            {
                if (index >= mComponents.size())
                    return nullptr;
                return mComponents[index].get();
            }
            else
            {
                uint32_t count = 0;
                const uint32_t componentCount = mComponents.size();
                for (uint32_t i = 0; i < componentCount; ++i)
                {
                    T* component = dynamic_cast<T*>(mComponents[i].get());
                    if (component && count++ == index)
                        return component;
                }
                return nullptr;
            }
		}

        const std::vector<osg::ref_ptr<Component>>& getComponents() const
        {
            return mComponents;
        }

        uint32_t getComponentsCount() const
        {
            return mComponents.size();
        }

        void clearComponents();

        void setPosition(osg::Vec3d position)
        {
            mPosition = position;
            updateTransform();
        }

        osg::Vec3d getPosition() const
        {
            return mPosition;
        }

        void setRotation(osg::Vec3d rotation)
        {
            mRotation = osg::Vec3d(
                std::fmod(rotation.x() + 360.0, 360.0),
                std::fmod(rotation.y() + 360.0, 360.0),
                std::fmod(rotation.z() + 360.0, 360.0)
            );
            updateTransform();
        }

        osg::Vec3d getRotation()
        {
            return mRotation;
        }

        void setScale(osg::Vec3d scale)
        {
            mScale = scale;
            updateTransform();
        }

        osg::Vec3d getScale() const
        {
            return mScale;
        }

        void setMatrix(const osg::Matrixd& matrix)
        {
            mOsgEntityNode->setMatrix(matrix);
        }

        const osg::Matrixd& getMatrix()
        {
            return mOsgEntityNode->getMatrix();
        }

        /*osg::Matrixd getWorldToLocalMatrix()
        {
            osg::NodePath nodePath = this->getParentalNodePaths()[0];
            for (osg::NodePath::const_iterator itr = nodePath.begin(); itr != nodePath.end(); itr++)
            {
                Entity* entity = castNodeTo<Entity>(*itr);
                if (entity)
                    entity->updateMatrix();
            }
            return osg::computeWorldToLocal(nodePath);
        }

        osg::Matrixd getLocalToWorldMatrix()
        {
            osg::NodePath nodePath = this->getParentalNodePaths()[0];
            return osg::computeLocalToWorld(nodePath);
        }*/

        void updateTransform()
        {
            double halfRoll = osg::DegreesToRadians(mRotation.x()) * 0.5;
            double halfPitch = osg::DegreesToRadians(mRotation.y()) * 0.5;
            double halfYaw = osg::DegreesToRadians(mRotation.z()) * 0.5;
            double cr = std::cos(halfRoll);
            double sr = std::sin(halfRoll);
            double cp = std::cos(halfPitch);
            double sp = std::sin(halfPitch);
            double cy = std::cos(halfYaw);
            double sy = std::sin(halfYaw);

            osg::Quat q(
                cy * cp * sr - sy * sp * cr, // x
                sy * cp * sr + cy * sp * cr, // y
                sy * cp * cr - cy * sp * sr, // z
                cy * cp * cr + sy * sp * sr  // w
            );

            mOsgEntityNode->setMatrix(
                osg::Matrixd::scale(mScale) *
                osg::Matrixd::rotate(q) *
                osg::Matrixd::translate(mPosition)
            );
        }

	protected:
        std::string mName;
        uint32_t mFlags = 0;
		Entity* mParent = nullptr;
		std::vector<osg::ref_ptr<Entity>> mChildren;
        std::vector<osg::ref_ptr<Component>> mComponents;

        osg::Vec3d mPosition = osg::Vec3d(0, 0, 0);
        osg::Vec3d mRotation = osg::Vec3d(0, 0, 0);
        osg::Vec3d mScale = osg::Vec3d(1, 1, 1);

        osg::ref_ptr<EntityNode> mOsgEntityNode;
        osg::ref_ptr<osg::Group> mOsgChildrenGroup;
        osg::ref_ptr<osg::Group> mOsgComponentsGroup;
	};
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Entity>();
}
