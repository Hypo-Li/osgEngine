#pragma once
#include "Entity.h"

namespace xxx
{
	class Component : public osg::Group
	{
		friend class Entity;
	public:
        enum class Type
        {
            MeshRenderer,
            Count,
        };

        Component(Type type) : _type(type), _owner(nullptr) {}
		virtual ~Component() = default;

        inline Type getType() const { return _type; }

        inline Entity* getOwner() const { return _owner; }

        /*virtual void onInspector() = 0;*/
	private:
        Type _type;
		Entity* _owner;
	};
}
