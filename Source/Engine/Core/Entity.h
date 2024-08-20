#pragma once
#include "Object.h"

#include <osg/MatrixTransform>
#include <osg/ref_ptr>

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

	class Component;
	class Entity : public Object
	{
        friend class refl::Reflection;
        static Object* createInstance()
        {
            return new Entity;
        }
    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Entity>());
        }
        static const Entity* getDefaultObject()
        {
            static osg::ref_ptr<Entity> defaultEntity = new Entity;
            return defaultEntity.get();
        }

	public:
		Entity(std::string&& name = "");
		virtual ~Entity() = default;

        const std::string& getName() const { return mName; }
		Entity* getParent() const { return mParent; }
		void appendChild(Entity* child);
		void removeChild(Entity* child);
		Entity* getChildByIndex(uint32_t index);
        uint32_t getChildrenCount() { return mChildren.size(); }
		void appendComponent(Component* component);
		void removeComponent(Component* component);
        uint32_t getComponentsCount() { return mComponents.size(); }
        Component* getComponentByIndex(uint32_t index);
        
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

	private:
        std::string mName;
		Entity* mParent;
		std::vector<osg::ref_ptr<Entity>> mChildren;
        std::vector<osg::ref_ptr<Component>> mComponents;

        osg::ref_ptr<EntityNode> mOsgEntityNode;
        osg::ref_ptr<osg::Group> mOsgChildrenGroup;
        osg::ref_ptr<osg::Group> mOsgComponentsGroup;
	};

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<Entity>()
        {
            Class* clazz = new Class("Entity", sizeof(Entity), Entity::createInstance);
            clazz->setBaseClass(dynamic_cast<Class*>(Reflection::getType<Object>()));
            Property* propName = clazz->addProperty("Name", &Entity::mName);
            Property* propParent = clazz->addProperty("Parent", &Entity::mParent);
            Property* propChildren = clazz->addProperty("Children", &Entity::mChildren);
            Property* propComponents = clazz->addProperty("Components", &Entity::mComponents);
            sRegisteredClassMap.emplace("Entity", clazz);
            return clazz;
        }
    }
}
