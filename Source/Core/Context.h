#pragma once
#include "Entity.h"
namespace xxx
{
	class Context
	{
	public:
		enum EngineMode
		{
			Edit_Mode,
			Runtime_Mode,
		};
		osg::ref_ptr<Entity> _activeEntity;
		std::vector<osg::ref_ptr<Entity>> _selectedEntities;
	};
}