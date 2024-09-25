#include "Entity.h"
#include "Asset.h"
namespace xxx
{
    class Prefab : public Entity
    {
        REFLECT_CLASS(Prefab)
    public:
        Prefab();
        Prefab(const Prefab& other);
        virtual ~Prefab() = default;

        void appendPackedEntity(Entity* entity)
        {
            if (entity->mParent == this)
                return;
            if (entity->mParent != nullptr)
                entity->mParent->removeChild(entity);
            entity->mParent = this;
            mPackedEntities.emplace_back(entity);
            mOsgPackedEntitiesGroup->addChild(entity->mOsgEntityNode);
        }

        static Entity* copyEntitiesRecursive(Entity* entity)
        {
            Entity* newEntity = entity->clone();
            for (Entity* child : entity->getChildren())
                newEntity->addChild(copyEntitiesRecursive(child));

            Prefab* prefab = dynamic_cast<Prefab*>(newEntity);
            if (prefab)
            {
                for (Entity* packedEntity : prefab->mPackedEntities)
                    prefab->appendPackedEntity(copyEntitiesRecursive(packedEntity));
            }
            
            return newEntity;
        }

        void syncWithAsset()
        {
            Prefab* prefab = getAsset()->getRootObject<Prefab>();
            if (prefab)
            {
                clearPackedEntities();

                for (Entity* packedEntity : prefab->mPackedEntities)
                    appendPackedEntity(copyEntitiesRecursive(packedEntity));
            }
            else
            {
                // ERROR: Asset's primary object is not a prefab!
            }
        }

        static Entity* unpack(Prefab* prefab)
        {
            Entity* entity = new Entity(prefab->getName());

            for (Entity* packedEntity : prefab->mPackedEntities)
            {
                prefab->mOsgPackedEntitiesGroup->removeChild(packedEntity->mOsgEntityNode);
                entity->addChild(packedEntity);
            }
            prefab->mPackedEntities.clear();

            for (Entity* child : prefab->getChildren())
                entity->addChild(child);

            return entity;
        }

    protected:
        std::vector<osg::ref_ptr<Entity>> mPackedEntities;

        osg::ref_ptr<osg::Group> mOsgPackedEntitiesGroup;

        void clearPackedEntities()
        {
            for (auto it : mPackedEntities)
            {
                it->mParent = nullptr;
                mOsgPackedEntitiesGroup->removeChild(it->mOsgEntityNode);
            }
            mPackedEntities.clear();
        }
    };

    namespace refl
    {
        template<> Type* Reflection::createType<Prefab>();
    }
}
