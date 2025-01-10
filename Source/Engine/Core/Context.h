#pragma once
#include <Engine/Core/Entity.h>
#include <Engine/Core/Scene.h>
#include <osg/ref_ptr>
#include <osg/GraphicsContext>
#include <osg/Camera>
#include <filesystem>
#include <queue>
#include <functional>
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

    template <typename T>
    class ThreadSafeQueue
    {
    public:
        ThreadSafeQueue() {}
        ~ThreadSafeQueue() {}

        void push(T&& item)
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mQueue.push(item);
            lock.unlock();
            mCV.notify_one();
        }

        std::optional<T> pop()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mCV.wait(lock, [this]() {return !mQueue.empty() || mShutdown; });
            if (mQueue.empty())
            {
                return {};
            }
            else
            {
                T res(std::move(mQueue.front()));
                mQueue.pop();
                return res;
            }
        }

        void shutdown()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            while (!mQueue.empty())
                mQueue.pop();
            mShutdown = true;
            lock.unlock();
            mCV.notify_one();
        }

        bool isShutdown()
        {
            return mShutdown;
        }

        size_t size()
        {
            return mQueue.size();
        }

    private:
        std::queue<T> mQueue;
        std::mutex mMutex;
        std::condition_variable mCV;
        bool mShutdown = false;
    };

    using ThreadSafeCommandQueue = ThreadSafeQueue<std::function<void()>>;
    using ThreadId = std::thread::id;

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

        void setLogicThreadId(ThreadId id)
        {
            mLogicThreadId = id;
        }

        ThreadId getLogicThreadId()
        {
            return mLogicThreadId;
        }

        void setRenderingThreadId(ThreadId id)
        {
            mRenderingThreadId = id;
        }

        ThreadId getRenderingThreadId()
        {
            return mRenderingThreadId;
        }

        ThreadSafeCommandQueue& getLogicCommandQueue()
        {
            return mLogicCommandQueue;
        }

        ThreadSafeCommandQueue& getRenderingCommandQueue()
        {
            return mRenderingCommandQueue;
        }

    private:
        osg::ref_ptr<osg::GraphicsContext> mGraphicsContext;
        Engine* mEngine;

        // Assets search paths
        std::filesystem::path mEnginePath;
        std::filesystem::path mProjectPath;
        std::filesystem::path mEngineAssetPath;
        std::filesystem::path mProjectAssetPath;

        Entity* mActivedEntity;
        Scene* mScene;

        ThreadId mLogicThreadId;
        ThreadId mRenderingThreadId;

        ThreadSafeQueue<std::function<void()>> mLogicCommandQueue;
        ThreadSafeQueue<std::function<void()>> mRenderingCommandQueue;
    };
}
