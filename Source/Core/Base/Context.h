#pragma once
#include <Core/Base/Entity.h>
#include <osg/ref_ptr>
namespace xxx
{
    class Context
    {
    public:
        static Context& get()
        {
            static Context context;
            return context;
        }

        Entity* getActivedEntity() const
        {
            return _activedEntity;
        }

        void setActivedEntity(Entity* entity)
        {
            _activedEntity = entity;
        }

    private:
        osg::ref_ptr<Entity> _activedEntity;
    };
}
