#pragma once
#include <Engine/Core/Context.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Render/Pipeline.h>

#include <osgViewer/Viewer>

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

    class Engine
    {
    public:
        Engine(const EngineSetupConfig& setupConfig) :
            mViewer(new osgViewer::Viewer)
        {
            initContext(setupConfig);
            initPipeline(setupConfig);
        }

        void run()
        {
            while (!mViewer->done())
            {
                mViewer->frame();
            }
        }

        osgViewer::ViewerBase* getViewer() const
        {
            return mViewer;
        }

    private:
        osg::ref_ptr<osgViewer::ViewerBase> mViewer;
        osg::ref_ptr<Pipeline> mPipeline;

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
        }

        void initPipeline(const EngineSetupConfig& setupConfig)
        {
            osg::ref_ptr<osg::GraphicsContext> graphicsContext = createGraphicsContext(setupConfig.width, setupConfig.height, setupConfig.glContextVersion);
            osgViewer::Viewer::Views views;
            mViewer->getViews(views);
            mPipeline = new Pipeline(views.at(0), graphicsContext);
        }
    };
}
