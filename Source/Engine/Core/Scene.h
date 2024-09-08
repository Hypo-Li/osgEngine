#pragma once
#include "Entity.h"
#include "Asset.h"

namespace xxx
{
    class Scene : public Object, public AssetRoot
    {
        static void forEachEntity(Entity* entity, std::function<void(Entity*)> func)
        {
            func(entity);
            for (Entity* child : entity->getChildren())
                forEachEntity(child, func);
        }

    public:
        virtual refl::Class* getClass() const
        {
            return static_cast<refl::Class*>(refl::Reflection::getType<Scene>());
        }

        virtual Scene* clone() const override
        {
            return new Scene(*this);
        }
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
    };
}
