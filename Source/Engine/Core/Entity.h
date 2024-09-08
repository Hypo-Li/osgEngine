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

        friend class refl::Reflection;
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Entity>());
        }

        virtual Entity* clone() const override
        {
            return new Entity(*this);
        }

	public:
		Entity(const std::string& name = "");
        Entity(const Entity& other);
        Entity& operator=(const Entity& other);
		virtual ~Entity() = default;

        void setName(const std::string& name)
        {
            mName = name;
        }

        const std::string& getName() const
        {
            return mName;
        }

		Entity* getParent() const
        {
            return mParent;
        }

        const std::vector<osg::ref_ptr<Entity>>& getChildren() const
        {
            return mChildren;
        }

        uint32_t getChildrenCount() const
        {
            return mChildren.size();
        }

		void appendChild(Entity* child);

		void removeChild(Entity* child);

        void removeChild(uint32_t index);

        Entity* getChildByIndex(uint32_t index);

        void clearChildren();

        const std::vector<osg::ref_ptr<Component>>& getComponents() const
        {
            return mComponents;
        }

        uint32_t getComponentsCount() const
        {
            return mComponents.size();
        }
        
		template<typename T, std::enable_if_t<std::is_base_of_v<Component, T>, int> = 0>
		T* getComponent(uint32_t index)
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

		void appendComponent(Component* component);

		void removeComponent(Component* component);

        void removeComponent(uint32_t index);

        Component* getComponentByIndex(uint32_t index);

        void clearComponents();

        /*void setPosition(osg::Vec3d position)
        {
            _position = position;
            _needUpdateMatrix = true;
        }

        osg::Vec3d getPosition() const
        {
            return _position;
        }

        void translate(osg::Vec3d delta, osg::Transform::ReferenceFrame mode)
        {
            if (mode == osg::Transform::ReferenceFrame::RELATIVE_RF)
            {
                _position += delta;
                _needUpdateMatrix = true;
            }
            else
            {

            }
        }

        void setRotation(osg::Vec3d rotation)
        {
            _rotation = osg::Vec3d(
                std::fmod(rotation.x() + 360.0, 360.0),
                std::fmod(rotation.y() + 360.0, 360.0),
                std::fmod(rotation.z() + 360.0, 360.0)
            );
            _needUpdateMatrix = true;
        }

        osg::Vec3d getRotation()
        {
            return _rotation;
        }

        void setScale(osg::Vec3d scale)
        {
            _scale = scale;
            _needUpdateMatrix = true;
        }

        osg::Vec3d getScale() const
        {
            return _scale;
        }

        void setMatrix(const osg::Matrixd& matrix)
        {
            osg::MatrixTransform::setMatrix(matrix);
        }

        const osg::Matrixd& getMatrix()
        {
            updateMatrix();
            return osg::MatrixTransform::getMatrix();
        }*/

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

        //void updateMatrix()
        //{
        //    if (_needUpdateMatrix)
        //    {
        //        double halfRoll = osg::DegreesToRadians(_rotation.x()) * 0.5;
        //        double halfPitch = osg::DegreesToRadians(_rotation.y()) * 0.5;
        //        double halfYaw = osg::DegreesToRadians(_rotation.z()) * 0.5;
        //        double cr = std::cos(halfRoll);
        //        double sr = std::sin(halfRoll);
        //        double cp = std::cos(halfPitch);
        //        double sp = std::sin(halfPitch);
        //        double cy = std::cos(halfYaw);
        //        double sy = std::sin(halfYaw);

        //        osg::Quat q(
        //            cy * cp * sr - sy * sp * cr, // x
        //            sy * cp * sr + cy * sp * cr, // y
        //            sy * cp * cr - cy * sp * sr, // z
        //            cy * cp * cr + sy * sp * sr  // w
        //        );

        //        osg::MatrixTransform::setMatrix(
        //            osg::Matrixd::scale(_scale) *
        //            osg::Matrixd::rotate(q) *
        //            osg::Matrixd::translate(_position)
        //        );
        //        _needUpdateMatrix = false;
        //    }
        //}

	protected:
        std::string mName;
		Entity* mParent;
		std::vector<osg::ref_ptr<Entity>> mChildren;
        std::vector<osg::ref_ptr<Component>> mComponents;

        osg::ref_ptr<EntityNode> mOsgEntityNode;
        osg::ref_ptr<osg::Group> mOsgChildrenGroup;
        osg::ref_ptr<osg::Group> mOsgComponentsGroup;
	};
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Entity>();
}
