#pragma once
#include "Entity.h"

namespace xxx
{
	class Component : protected osg::Group
	{
		friend class Entity;
	public:
        Component() : _owner(nullptr) {}
		virtual ~Component() = default;

        inline Entity* getOwner() const { return _owner; }

        /*virtual void onInspector() = 0;*/
	private:
		Entity* _owner;
	};
}
