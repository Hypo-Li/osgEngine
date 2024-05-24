#pragma once
#include <osg/MatrixTransform>
#include <osg/ref_ptr>

namespace xxx
{
	class Component;
	class Entity : protected osg::MatrixTransform
	{
        template<class T>
        friend class osg::ref_ptr;
	public:
		Entity(std::string&& name);
		virtual ~Entity() = default;

        osg::MatrixTransform* asMatrixTransform() { return static_cast<osg::MatrixTransform*>(this); }
        const std::string& getName() const { return _name; }
		Entity* getParent() const { return _parent; }
		void appendChild(Entity* child);
		void removeChild(Entity* child);
		Entity* getChild(uint32_t index);
		void appendComponent(Component* component);
		void removeComponent(Component* component);

		template<typename T, typename = typename std::enable_if<std::is_base_of<Component, T>::value>::type>
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

        void setRotation(osg::Quat rotation)
        {
            _rotation = rotation;
            _needUpdateMatrix = true;
        }

        void setRotation(osg::Vec3d eulerAngles)
        {
            _rotationEulerAngles = osg::Vec3d(std::fmod(eulerAngles.x(), 360.0), std::fmod(eulerAngles.y(), 360.0), std::fmod(eulerAngles.z(), 360.0));
            if (_rotationEulerAngles.x() < 0.0) _rotationEulerAngles.x() += 360.0;
            if (_rotationEulerAngles.y() < 0.0) _rotationEulerAngles.y() += 360.0;
            if (_rotationEulerAngles.z() < 0.0) _rotationEulerAngles.z() += 360.0;
            double halfRoll = osg::DegreesToRadians(_rotationEulerAngles.x()) * 0.5;
            double halfPitch = osg::DegreesToRadians(_rotationEulerAngles.y()) * 0.5;
            double halfYaw = osg::DegreesToRadians(_rotationEulerAngles.z()) * 0.5;
            double cr = std::cos(halfRoll);
            double sr = std::sin(halfRoll);
            double cp = std::cos(halfPitch);
            double sp = std::sin(halfPitch);
            double cy = std::cos(halfYaw);
            double sy = std::sin(halfYaw);

            _rotation.x() = cy * cp * sr - sy * sp * cr;
            _rotation.y() = sy * cp * sr + cy * sp * cr;
            _rotation.z() = sy * cp * cr - cy * sp * sr;
            _rotation.w() = cy * cp * cr + sy * sp * sr;
            _needUpdateMatrix = true;
        }

        osg::Quat getRotation() const
        {
            return _rotation;
        }

        osg::Vec3d getRotationAsEulerAngles()
        {
            /*osg::Matrixd m(_rotation);
            double t1 = std::atan2(m(2, 1), m(2, 2));
            double c2 = std::sqrt(m(0, 0) * m(0, 0) + m(1, 0) * m(1, 0));
            double t2 = std::atan2(-m(2, 0), c2);
            double s1 = std::sin(t1);
            double c1 = std::cos(t1);
            double t3 = std::atan2(s1 * m(0, 2) - c1 * m(0, 1), c1 * m(1, 1) - s1 * m(1, 2));
            return osg::Vec3d(-osg::RadiansToDegrees(t1), -osg::RadiansToDegrees(t2), -osg::RadiansToDegrees(t3));*/
            return _rotationEulerAngles;
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
            updateTransform();
            return osg::MatrixTransform::getMatrix();
        }

	private:
        std::string _name;
		Entity* _parent;
		osg::ref_ptr<osg::Group> _childrenGroup;
		osg::ref_ptr<osg::Group> _componentsGroup;

        // Transform
        osg::Vec3d _position;
        osg::Quat _rotation;
        osg::Vec3d _rotationEulerAngles;
        osg::Vec3d _scale;
        bool _needUpdateMatrix;

        void updateTransform()
        {
            if (_needUpdateMatrix)
            {
                osg::MatrixTransform::setMatrix(
                    osg::Matrixd::scale(_scale) *
                    osg::Matrixd::rotate(_rotation) *
                    osg::Matrixd::translate(_position)
                );
                _needUpdateMatrix = false;
            }
        }
	};

    template <typename T>
    static T* castNodeTo(osg::Node* node)
    {
        return typeid(*node) == typeid(T) ? reinterpret_cast<T*>(node) : nullptr;
    }
}
