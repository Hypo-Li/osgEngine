#pragma once
#include <Core/Base/Entity.h>
#include <osg/ref_ptr>
namespace xxx
{
    class Engine;
    class Context
    {
        friend class Engine;
    public:
        static Context& get()
        {
            static Context context;
            return context;
        }

        void appendAssetsPath(const std::string& path)
        {
            _assetsPaths.insert(path);
        }

        void removeAssetsPath(const std::string& path)
        {
            _assetsPaths.erase(path);
        }

        void appendEntity(Entity* entity, Entity* parent = nullptr)
        {
            if (parent)
                parent->appendChild(entity);
            else
                _sceneRoot->addChild(entity->asMatrixTransform());
        }

        void removeEntity(Entity* entity, Entity* parent = nullptr)
        {
            if (parent)
                parent->removeChild(entity);
            else
                _sceneRoot->removeChild(entity->asMatrixTransform());
        }

        osg::ref_ptr<Entity> getActivedEntity() const
        {
            osg::ref_ptr<Entity> result;
            _activedEntity.lock(result);
            return result;
        }

        void setActivedEntity(Entity* entity)
        {
            _activedEntity = entity;
        }

        std::set<osg::ref_ptr<Entity>> getSelectedEntities()
        {
            std::set<osg::ref_ptr<Entity>> result;
            for (auto itr = _selectedEntities.begin(); itr != _selectedEntities.end(); itr++)
            {
                osg::ref_ptr<Entity> entity;
                itr->lock(entity);
                if (entity)
                    result.insert(entity);
            }
            return result;
        }

        void appendSelectedEntity(Entity* entity)
        {
            _selectedEntities.insert(entity);
        }

    private:
        bool _isEditorMode;
        // Assets search paths
        std::set<std::string> _assetsPaths;

        osg::observer_ptr<Entity> _activedEntity;
        std::set<osg::observer_ptr<Entity>> _selectedEntities;

        
        // current scene

    };
}
