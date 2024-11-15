#pragma once
#include <Engine/Core/Entity.h>
#include <osg/ref_ptr>
#include <osg/GraphicsContext>
#include <filesystem>
namespace xxx
{
    class Engine;
    class Material;

    enum class EngineMode
    {
        Edit,
        Run,
    };

    enum class EditMode
    {
        Select,
        Terrain,
    };

    class Context
    {
        friend class Engine;
    public:
        Context()
        {
            mEnginePath = std::filesystem::current_path();
            std::string engineAssetPathStr = ASSET_DIR;
            engineAssetPathStr.pop_back();
            mEngineAssetPath = engineAssetPathStr;
        }

        static Context& get()
        {
            static Context context;
            return context;
        }

        void setGraphicsContext(osg::GraphicsContext* gc)
        {
            mGraphicsContext = gc;
        }

        osg::GraphicsContext* getGraphicsContext()
        {
            return mGraphicsContext;
        }

        const std::filesystem::path& getEnginePath() const
        {
            return mEnginePath;
        }

        const std::filesystem::path& getEngineAssetPath() const
        {
            return mEngineAssetPath;
        }

        const std::filesystem::path& getProjectAssetPath() const
        {
            return mProjectAssetPath;
        }

        osg::ref_ptr<Entity> getActivedEntity() const
        {
            return mActivedEntity;
        }

        void setActivedEntity(Entity* entity)
        {
            mActivedEntity = entity;
        }

        void setEngine(Engine* engine)
        {
            mEngine = engine;
        }

        Engine* getEngine() const
        {
            return mEngine;
        }

        Material* getDefaultMaterial() const;

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

        /*osg::ref_ptr<Entity> getActivedEntity() const
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
        }*/

    private:
        osg::ref_ptr<osg::GraphicsContext> mGraphicsContext;
        Engine* mEngine;

        // Assets search paths
        std::filesystem::path mEnginePath;
        std::filesystem::path mProjectPath;
        std::filesystem::path mEngineAssetPath;
        std::filesystem::path mProjectAssetPath;

        Entity* mActivedEntity;
        //std::set<osg::observer_ptr<Entity>> _selectedEntities;


    };
}
