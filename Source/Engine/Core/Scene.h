#pragma once
#include "Entity.h"
#include "Asset.h"

namespace xxx
{
    class Scene : public Object
    {
        REFLECT_CLASS(Scene)
    public:
        Scene() = default;

        void appendEntity(Entity* entity);

        void removeEntity(Entity* entity);

        void forEach(std::function<void(Entity*)> func)
        {
            for (Entity* entity : mEntities)
                forEachEntity(entity, func);
        }

    protected:
        std::vector<osg::ref_ptr<Entity>> mEntities;

        osg::ref_ptr<osg::Group> mOsgSceneRoot;

        static void forEachEntity(Entity* entity, std::function<void(Entity*)> func)
        {
            func(entity);
            for (Entity* child : entity->getChildren())
                forEachEntity(child, func);
        }
    };
}
