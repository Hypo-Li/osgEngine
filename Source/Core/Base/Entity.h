#pragma once
#include <osg/MatrixTransform>

namespace xxx
{
	class Component;
	class Entity : protected osg::MatrixTransform
	{
	public:
		Entity(std::string_view name);
		virtual ~Entity() = default;

        osg::MatrixTransform* asMatrixTransform() { return dynamic_cast<osg::MatrixTransform*>(this); }
        std::string_view getName() const { return _name; }
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
        std::string_view _name;
		Entity* _parent;
		osg::ref_ptr<osg::Group> _childrenGroup;
		osg::ref_ptr<osg::Group> _componentsGroup;
	};

    static Entity* castNodeToEntity(osg::Node* node)
    {
        static const type_info& typeidOfEntity = typeid(Entity);
        if (typeid(*node) == typeidOfEntity)
            return reinterpret_cast<Entity*>(node);
        else
            return nullptr;
    }
}
