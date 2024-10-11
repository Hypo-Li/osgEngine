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

        virtual void postSerializer(Serializer* serializer)
        {
            if (serializer->isLoader())
            {
                for (Entity* entity : mEntities)
                {
                    mOsgSceneRoot->addChild(entity->getOsgNode());
                }
            }
        }

        void addEntity(Entity* entity)
        {
            entity->setOwner(this);
            entity->setParent(nullptr);
            mEntities.emplace_back(entity);
            mOsgSceneRoot->addChild(entity->getOsgNode());
        }

        void removeEntity(Entity* entity)
        {
            mOsgSceneRoot->removeChild(entity->getOsgNode());
            auto findResult = std::find(mEntities.begin(), mEntities.end(), entity);
            if (findResult != mEntities.end())
            {
                entity->setOwner(nullptr);
                mEntities.erase(findResult);
            }

        }

    protected:
        std::vector<osg::ref_ptr<Entity>> mEntities;

        osg::ref_ptr<osg::Group> mOsgSceneRoot;
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<Scene>()
        {
            Class* clazz = new ClassInstance<Scene>("Scene");
            clazz->addProperty("Entities", &Scene::mEntities);
            return clazz;
        }
    }
}
