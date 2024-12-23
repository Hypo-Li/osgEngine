#pragma once
#include "Prefab.h"
#include "Asset.h"

namespace xxx
{
    class Scene : public Object
    {
        REFLECT_CLASS(Scene)
    public:
        Scene(const std::string& name = "") :
            mRootEntity(new Entity(name))
        {
            mRootEntity->setOwner(this);
        }
        virtual ~Scene() = default;

        Entity* getRootEntity() const
        {
            return mRootEntity;
        }

        Entity* addPrefabEntity(Prefab* prefab)
        {
            if (!prefab)
                return nullptr;
            Entity* entity = prefab->generateEntity();
            mEntityPrefabMap.emplace(entity, prefab);
            return entity;
        }

        Prefab* getPrefab(Entity* entity)
        {
            auto findResult = mEntityPrefabMap.find(entity);
            if (findResult != mEntityPrefabMap.end())
                return findResult->second;
            return nullptr;
        }

        bool addEntity(Entity* entity, Entity* parent = nullptr)
        {
            if (!entity)
                return false;

            if (parent == nullptr)
            {
                mRootEntity->addChild(entity);
                return true;
            }
            else if (parent->getRoot() == this)
            {
                parent->addChild(entity);
                return true;
            }
            else
                return false;
        }

    protected:
        osg::ref_ptr<Entity> mRootEntity;
        std::unordered_map<osg::ref_ptr<Entity>, osg::ref_ptr<Prefab>> mEntityPrefabMap;
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<Scene>()
        {
            Class* clazz = new TClass<Scene>("Scene");
            clazz->addProperty("RootEntity", &Scene::mRootEntity);
            clazz->addProperty("EntityPrefabMap", &Scene::mEntityPrefabMap);
            return clazz;
        }
    }
}
