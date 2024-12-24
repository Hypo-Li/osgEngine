#pragma once
#include <Engine/Core/Entity.h>
#include <Engine/Core/Scene.h>
#include <osg/ref_ptr>
#include <osg/GraphicsContext>
#include <osg/Camera>
#include <filesystem>
#include <mutex>
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

    class RenderCommand : public osg::Referenced
    {
    public:
        virtual void execute(osg::RenderInfo& renderInfo) = 0;
    };

    class RenderCommandsCallback : public osg::Camera::DrawCallback
    {
    public:
        virtual void operator () (osg::RenderInfo& renderInfo) const override
        {
            std::unique_lock<std::mutex> lock(mRenderCommandListMutex);
            for (RenderCommand* renderCommand : mRenderCommandList)
                renderCommand->execute(renderInfo);
        }

        void addRenderCommand(RenderCommand* renderCommand)
        {
            std::lock_guard<std::mutex> lock(mRenderCommandListMutex);
            mRenderCommandList.push_back(renderCommand);
        }

    private:
        mutable std::list<osg::ref_ptr<RenderCommand>> mRenderCommandList;
        mutable std::mutex mRenderCommandListMutex;
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

        void setScene(Scene* scene)
        {
            mScene = scene;
        }

        Scene* getScene()
        {
            return mScene;
        }

        void setRenderCommandsCallback(RenderCommandsCallback* callback)
        {
            mRenderCommandsCallback = callback;
        }

        void addRenderCommand(RenderCommand* renderCommand)
        {
            mRenderCommandsCallback->addRenderCommand(renderCommand);
        }

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
        Scene* mScene;

        osg::ref_ptr<RenderCommandsCallback> mRenderCommandsCallback;
    };
}
