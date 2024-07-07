#pragma once
#include <Core/Base/Entity.h>
#include <osg/ref_ptr>
#include <filesystem>
namespace xxx
{
    class Engine;
    class Context
    {
        friend class Engine;
    public:
        Context()
        {
            std::filesystem::path currentPath = std::filesystem::current_path();
            _engineAssetPath = currentPath / "../Asset";
        }

        static Context& get()
        {
            static Context context;
            return context;
        }

        const std::filesystem::path& getEngineAssetPath() { return _engineAssetPath; }
        const std::filesystem::path& getProjectAssetPath() { return _projectAssetPath; }

        void setSceneRoot(osg::Node* sceneRoot) { _sceneRoot = sceneRoot; }
        osg::Node* getSceneRoot() const { return _sceneRoot; }

        /*void appendEntity(Entity* entity, Entity* parent = nullptr)
        {
            if (parent)
                parent->appendChildEntity(entity);
            else
                _sceneRoot->addChild(entity->asMatrixTransform());
        }

        void removeEntity(Entity* entity, Entity* parent = nullptr)
        {
            if (parent)
                parent->removeChildEntity(entity);
            else
                _sceneRoot->removeChild(entity->asMatrixTransform());
        }*/

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
        std::filesystem::path _engineAssetPath;
        std::filesystem::path _projectAssetPath;

        osg::observer_ptr<Entity> _activedEntity;
        std::set<osg::observer_ptr<Entity>> _selectedEntities;

        osg::ref_ptr<osg::Node> _sceneRoot;

    };
}
