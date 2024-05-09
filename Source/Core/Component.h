#pragma once
#include "Entity.h"

namespace xxx
{
	class Component : public osg::Group
	{
		friend class Entity;
	public:
		Component();
		virtual ~Component() = default;
	private:
		Entity* _owner;
	};
}