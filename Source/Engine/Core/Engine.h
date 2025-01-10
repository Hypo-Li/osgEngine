#pragma once
#include <Engine/Core/Context.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Render/SceneRendering.h>

#include <osg/BlendFunc>
#include <osg/BufferIndexBinding>
#include <osgViewer/View>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>

#include <thread>

namespace xxx
{
    enum class RunMode
    {
        Edit,
        Run,
    };

    struct EngineSetupConfig
    {
        int width, height;
        std::string glContextVersion;
        bool fullScreen;
        RunMode runMode;
    };

    class ResizedCallback : public osg::GraphicsContext::ResizedCallback
    {
        xxx::Pipeline* mPipeline;
        int mWidth, mHeight;
        bool mIsEditorMode;
    public:
        ResizedCallback(xxx::Pipeline* pipeline, bool isEditorMode) : mPipeline(pipeline), mIsEditorMode(isEditorMode)
        {
            osg::Viewport* viewport = pipeline->getView()->getCamera()->getViewport();
            mWidth = viewport->width();
            mHeight = viewport->height();
        }

        virtual void resizedImplementation(osg::GraphicsContext* gc, int x, int y, int width, int height)
        {
            if ((width == mWidth && height == mHeight) || (width == 1 && height == 1))
                return;
            mWidth = width, mHeight = height;
            mPipeline->resize(width, height, !mIsEditorMode, true);
        }
    };

    class Engine
    {
    public:
        Engine(osgViewer::View* view, const EngineSetupConfig& setupConfig) :
            mView(view)
        {
            initContext(setupConfig);
            initPipeline(setupConfig);

            mView->setCameraManipulator(new osgGA::TrackballManipulator);
        }

        osgViewer::View* getView() const
        {
            return mView;
        }

        Pipeline* getPipeline() const
        {
            return mPipeline;
        }

        void run()
        {
            Context& ctx = Context::get();
            osgViewer::ViewerBase* viewer = mView->getViewerBase();
            while (!viewer->done())
            {
                // processs logic command

                viewer->frame();

                /*if (viewer->isRealized())
                    processRenderingCommand();*/
            }
        }

        void terminate()
        {

        }

    private:
        osg::ref_ptr<osgViewer::View> mView;
        osg::ref_ptr<Pipeline> mPipeline;
        osg::ref_ptr<osg::UniformBufferBinding> mViewDataUBB;

        static osg::GraphicsContext* createGraphicsContext(int width, int height, const std::string& glContextVersion)
        {
            osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
            traits->width = width; traits->height = height;
            traits->windowDecoration = true;
            traits->doubleBuffer = true;
            traits->glContextVersion = glContextVersion;
            traits->readDISPLAY();
            traits->setUndefinedScreenDetailsToDefaultScreen();
            osg::GraphicsContext* graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
            graphicsContext->getState()->setUseModelViewAndProjectionUniforms(true);
            graphicsContext->getState()->setUseVertexAttributeAliasing(true);
            return graphicsContext;
        }

        void initContext(const EngineSetupConfig& setupConfig)
        {
            Context& ctx = Context::get();
            ctx.setGraphicsContext(createGraphicsContext(setupConfig.width, setupConfig.height, setupConfig.glContextVersion));
            ctx.setEngine(this);
            ctx.setLogicThreadId(std::this_thread::get_id());
            ctx.setRenderingThreadId(std::this_thread::get_id());
        }

        void initPipeline(const EngineSetupConfig& setupConfig)
        {
            osg::ref_ptr<osg::Camera> camera = mView->getCamera();
            camera->setViewport(0, 0, setupConfig.width, setupConfig.height);
            //camera->setProjectionMatrixAsPerspective(75.0, double(setupConfig.width) / double(setupConfig.height), 0.1, 400.0);
            double zNear = 0.01;
            double invTanHalfFovy = 1.0 / std::tan(osg::DegreesToRadians(90.0) * 0.5);
            
            camera->setProjectionMatrix(osg::Matrixd(
                invTanHalfFovy / (setupConfig.width / double(setupConfig.height)), 0.0, 0.0, 0.0,
                0.0, invTanHalfFovy, 0.0, 0.0,
                0.0, 0.0, 0.0, -1.0,
                0.0, 0.0, zNear, 0.0
            ));

            mPipeline = createSceneRenderingPipeline(mView, true);
            Context::get().getGraphicsContext()->setResizedCallback(new ResizedCallback(mPipeline, true));
        }

        static void processLogicCommand()
        {
            std::cout << "Logic thread setup." << std::endl;
            Context& ctx = Context::get();
            ThreadSafeCommandQueue& logicCommandQueue = ctx.getLogicCommandQueue();

            while (!logicCommandQueue.isShutdown())
            {
                auto command = logicCommandQueue.pop();
                if (command.has_value())
                    (command.value())();
            }
        }

        static void processRenderingCommand()
        {
            Context& ctx = Context::get();
            ThreadSafeCommandQueue& renderingCommandQueue = ctx.getRenderingCommandQueue();
            size_t commandCount = renderingCommandQueue.size();

            ctx.getGraphicsContext()->makeCurrent();
            while (commandCount--)
            {
                auto command = renderingCommandQueue.pop();
                if (command.has_value())
                    (command.value())();
            }
            ctx.getGraphicsContext()->releaseContext();
        }
    };
}
