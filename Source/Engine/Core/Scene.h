#pragma once
#include "Entity.h"

namespace xxx
{
    class Scene : public Object
    {
    public:
        void appendEntity(Entity* entity);

        void removeEntity(Entity* entity);

    private:
        std::vector<osg::ref_ptr<Entity>> mEntities;

        osg::ref_ptr<osg::Group> mOsgSceneRoot;
    };
}
