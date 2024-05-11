#pragma once
#include <osg/MatrixTransform>

namespace xxx
{
	class Component;
	class Entity : private osg::MatrixTransform
	{
	public:
		Entity();
		virtual ~Entity() = default;

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

	private:
		Entity* _parent;
		osg::ref_ptr<osg::Group> _childrenGroup;
		osg::ref_ptr<osg::Group> _componentsGroup;
	};
}