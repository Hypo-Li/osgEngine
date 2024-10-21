#pragma once
#include <Engine/Core/Context.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Render/SceneRendering.h>

#include <osg/BlendFunc>
#include <osg/BufferIndexBinding>
#include <osgViewer/View>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>

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
        Engine(const EngineSetupConfig& setupConfig) :
            mView(new osgViewer::View)
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
            Context& context = Context::get();
            context.setGraphicsContext(createGraphicsContext(setupConfig.width, setupConfig.height, setupConfig.glContextVersion));
            context.setEngine(this);
        }

        void initPipeline(const EngineSetupConfig& setupConfig)
        {
            osg::ref_ptr<osg::Camera> camera = mView->getCamera();
            camera->setViewport(0, 0, setupConfig.width, setupConfig.height);
            camera->setProjectionMatrixAsPerspective(90.0, double(setupConfig.width) / double(setupConfig.height), 0.1, 400.0);

            mPipeline = createSceneRenderingPipeline(mView, true);
            Context::get().getGraphicsContext()->setResizedCallback(new ResizedCallback(mPipeline, true));

            
        }
    };
}
