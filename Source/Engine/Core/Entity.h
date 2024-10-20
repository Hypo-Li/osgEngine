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

    enum class TransformMode
    {
        Local,
        World,
    };

    struct Transform
    {
        osg::Vec3d translation = osg::Vec3d(0, 0, 0);
        osg::Quat rotation = osg::Quat(0, 0, 0, 1);
        osg::Vec3d scale = osg::Vec3d(1, 1, 1);

        void setRotationAsEulerAngles(osg::Vec3d eulerAngles)
        {
            rotation = osg::Quat(
                osg::DegreesToRadians(eulerAngles.x()), osg::X_AXIS,
                osg::DegreesToRadians(eulerAngles.y()), osg::Y_AXIS,
                osg::DegreesToRadians(eulerAngles.z()), osg::Z_AXIS
            );
        }

        osg::Matrixd getMatrix() const
        {
            osg::Matrixd m;
            osg::Vec4d* vecs = reinterpret_cast<osg::Vec4d*>(m.ptr());

            double qxx(rotation.x() * rotation.x());
            double qyy(rotation.y() * rotation.y());
            double qzz(rotation.z() * rotation.z());
            double qxz(rotation.x() * rotation.z());
            double qxy(rotation.x() * rotation.y());
            double qyz(rotation.y() * rotation.z());
            double qwx(rotation.w() * rotation.x());
            double qwy(rotation.w() * rotation.y());
            double qwz(rotation.w() * rotation.z());

            vecs[0][0] = (1.0 - 2.0 * (qyy + qzz)) * scale.x();
            vecs[0][1] = (2.0 * (qxy + qwz)) * scale.x();
            vecs[0][2] = (2.0 * (qxz - qwy)) * scale.x();

            vecs[1][0] = (2.0 * (qxy - qwz)) * scale.y();
            vecs[1][1] = (1.0 - 2.0 * (qxx + qzz)) * scale.y();
            vecs[1][2] = (2.0 * (qyz + qwx)) * scale.y();

            vecs[2][0] = (2.0 * (qxz + qwy)) * scale.z();
            vecs[2][1] = (2.0 * (qyz - qwx)) * scale.z();
            vecs[2][2] = (1.0 - 2.0 * (qxx + qyy)) * scale.z();

            vecs[3][0] = translation.x();
            vecs[3][1] = translation.y();
            vecs[3][2] = translation.z();

            return m;
        }
    };

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
                    mOsgComponentsGroup->addChild(component->getOsgNode());
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

        Entity* getChild(uint32_t index);

        const std::vector<osg::ref_ptr<Entity>>& getChildren() const
        {
            return mChildren;
        }

        uint32_t getChildrenCount() const
        {
            return mChildren.size();
        }

		void removeChild(Entity* child);

        void removeChild(uint32_t index);

        void removeChildren(uint32_t beginIndex, uint32_t count);

        void clearChildren();

        using Components = std::vector<osg::ref_ptr<Component>>;

		void addComponent(Component* component);

        template <typename T, typename = std::enable_if_t<std::is_base_of_v<Component, T>>>
        T* addComponent()
        {
            T* component = new T;
            this->addComponent(component);
            return component;
        }
        
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

        Components::const_iterator removeComponent(Component* component);

        Components::const_iterator removeComponent(uint32_t index);

        void clearComponents();

        void setTranslation(osg::Vec3d translation)
        {
            mTranslation = translation;
            osg::Matrix matrix = mOsgEntityNode->getMatrix();
            matrix.setTrans(mTranslation);
            mOsgEntityNode->setMatrix(matrix);
        }

        void translate(osg::Vec3 delta, TransformMode transformMode)
        {
            if (transformMode == TransformMode::Local)
            {
                setTranslation(mTranslation += delta);
            }
        }

        osg::Vec3d getTranslation() const
        {
            return mTranslation;
        }

        void setRotation(osg::Vec3d rotation)
        {
            mRotation = rotation;
            osg::Quat quat(
                osg::DegreesToRadians(mRotation.x()), osg::Vec3d(1, 0, 0),
                osg::DegreesToRadians(mRotation.y()), osg::Vec3d(0, 1, 0),
                osg::DegreesToRadians(mRotation.z()), osg::Vec3d(0, 0, 1)
            );
            osg::Matrix matrix = mOsgEntityNode->getMatrix();
            matrix.setRotate(quat);
            mOsgEntityNode->setMatrix(matrix);
        }

        void rotate(osg::Vec3d delta, TransformMode transformMode)
        {
            
        }

        osg::Vec3d getRotation()
        {
            return mRotation;
        }

        void setScale(osg::Vec3d scale)
        {
            mScale = scale;
            osg::Matrix matrix = mOsgEntityNode->getMatrix();
            osg::Vec4d* vecs = reinterpret_cast<osg::Vec4d*>(matrix.ptr());
            vecs[0].normalize();
            vecs[1].normalize();
            vecs[2].normalize();
            vecs[0] *= mScale.x();
            vecs[1] *= mScale.y();
            vecs[2] *= mScale.z();
            mOsgEntityNode->setMatrix(matrix);
        }

        void scale(osg::Vec3d delta)
        {

        }

        osg::Vec3d getScale() const
        {
            return mScale;
        }

        void setMatrix(const osg::Matrixd& matrix)
        {
            osg::Matrixd mat = matrix;
            osg::Vec4d* vecs = reinterpret_cast<osg::Vec4d*>(mat.ptr());

            mScale.x() = vecs[0].normalize();
            mScale.y() = vecs[1].normalize();
            mScale.z() = vecs[2].normalize();

            mRotation.x() = osg::RadiansToDegrees(std::atan2(mat(1, 2), mat(2, 2)));
            mRotation.y() = osg::RadiansToDegrees(std::atan2(-mat(0, 2), std::sqrt(mat(1, 2) * mat(1, 2) + mat(2, 2) * mat(2, 2))));
            mRotation.z() = osg::RadiansToDegrees(std::atan2(mat(0, 1), mat(0, 0)));

            mTranslation.x() = vecs[3].x();
            mTranslation.y() = vecs[3].y();
            mTranslation.z() = vecs[3].z();

            mOsgEntityNode->setMatrix(matrix);
        }

        const osg::Matrixd& getMatrix()
        {
            return mOsgEntityNode->getMatrix();
        }

        osg::Matrixd getWorldToLocalMatrix()
        {
            return osg::computeWorldToLocal(mOsgEntityNode->getParentalNodePaths()[0]);
        }

        osg::Matrixd getLocalToWorldMatrix()
        {
            return osg::computeLocalToWorld(mOsgEntityNode->getParentalNodePaths()[0]);
        }

        const osg::Matrixd& getPreFrameTransformMatrix() const
        {
            return mPreFrameTransformMatrix;
        }

        void updateTransform()
        {
            osg::Quat quat(
                osg::DegreesToRadians(mRotation.x()), osg::Vec3d(1, 0, 0),
                osg::DegreesToRadians(mRotation.y()), osg::Vec3d(0, 1, 0),
                osg::DegreesToRadians(mRotation.z()), osg::Vec3d(0, 0, 1)
            );
            mOsgEntityNode->setMatrix(
                osg::Matrixd::scale(mScale) *
                osg::Matrixd::rotate(quat) *
                osg::Matrixd::translate(mTranslation)
            );
        }

        void hide()
        {
            mOsgEntityNode->setNodeMask(0);
            for (Component* component : mComponents)
                component->hide();
        }

        void show()
        {
            mOsgEntityNode->setNodeMask(0xFFFFFFFF);
            for (Component* component : mComponents)
                component->show();
        }

	protected:
        std::string mName;
        uint32_t mFlags = 0;
		Entity* mParent = nullptr;
		std::vector<osg::ref_ptr<Entity>> mChildren;
        std::vector<osg::ref_ptr<Component>> mComponents;

        osg::Vec3d mTranslation = osg::Vec3d(0, 0, 0);
        osg::Vec3d mRotation = osg::Vec3d(0, 0, 0);
        osg::Vec3d mScale = osg::Vec3d(1, 1, 1);

        osg::Matrixd mPreFrameTransformMatrix;

        osg::ref_ptr<EntityNode> mOsgEntityNode;
        osg::ref_ptr<osg::Group> mOsgChildrenGroup;
        osg::ref_ptr<osg::Group> mOsgComponentsGroup;
	};
}

namespace xxx::refl
{
    template <> Type* Reflection::createType<Entity>();
}
