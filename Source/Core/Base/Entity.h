#pragma once
#include <osg/MatrixTransform>
#include <osg/ref_ptr>
#include <osg/observer_ptr>

namespace xxx
{
	class Component;
	class Entity : public osg::MatrixTransform
	{
	public:
		Entity(std::string&& name);
		virtual ~Entity() = default;

        const std::string& getEntityName() const { return _entityName; }
		Entity* getParent() const { return _entityParent; }
		void appendChildEntity(Entity* child);
		void removeChildEntity(Entity* child);
		Entity* getChildEntity(uint32_t index);
        uint32_t getChildrenEntitiesCount() { return _childrenEntitiesGroup->getNumChildren(); }
		void appendComponent(Component* component);
		void removeComponent(Component* component);
        uint32_t getComponentsCount() { return _componentsGroup->getNumChildren(); }
        Component* getComponent(uint32_t index);
        
		template<typename T, typename = typename std::enable_if_t<std::is_base_of_v<Component, T>>>
		T* getComponent(uint32_t index)
		{
			uint32_t count = 0;
			const uint32_t childrenNum = _componentsGroup->getNumChildren();
			for (uint32_t i = 0; i < childrenNum; ++i)
			{
				T* component = dynamic_cast<T*>(_componentsGroup->getChild(i));
				if (component && count++ == index)
					return component;
			}
			return nullptr;
		}

        void setPosition(osg::Vec3d position)
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

        void updateMatrix()
        {
            if (_needUpdateMatrix)
            {
                double halfRoll = osg::DegreesToRadians(_rotation.x()) * 0.5;
                double halfPitch = osg::DegreesToRadians(_rotation.y()) * 0.5;
                double halfYaw = osg::DegreesToRadians(_rotation.z()) * 0.5;
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

                osg::MatrixTransform::setMatrix(
                    osg::Matrixd::scale(_scale) *
                    osg::Matrixd::rotate(q) *
                    osg::Matrixd::translate(_position)
                );
                _needUpdateMatrix = false;
            }
        }

	private:
        std::string _entityName;
		Entity* _entityParent;
		osg::ref_ptr<osg::Group> _childrenEntitiesGroup;
		osg::ref_ptr<osg::Group> _componentsGroup;

        // Transform
        osg::Vec3d _position;
        osg::Vec3d _rotation;
        osg::Vec3d _scale;
        bool _needUpdateMatrix;

        // disabled methods
        using Group::addChild;
        using Group::insertChild;
        using Group::removeChild;
        using Group::removeChildren;
        using Group::replaceChild;
        using Group::getNumChildren;
        using Group::setChild;
        using Group::getChild;
        using Group::containsNode;
        using Group::getChildIndex;
        using Node::setNodeMask;
        using Node::getNodeMask;
        // void setStateSet(osg::StateSet* stateset);
        // template<class T> void setStateSet(const osg::ref_ptr<T>& stateset);
        // osg::StateSet* getOrCreateStateSet();
        // osg::StateSet* getStateSet();
        // const osg::StateSet* getStateSet() const;
	};
}
