#pragma once
#include <Core/Base/Context.h>
#include <Core/Asset/AssetManager.h>
#include <Core/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <set>
namespace xxx
{
    struct EngineSetupConfig
    {
        int width, height;
        std::string glContextVersion;
        bool fullScreen;

        bool isEditorMode;
        std::set<std::string> assetPaths;
    };

    class Engine
    {
    public:
        Engine(osgViewer::ViewerBase* viewer, const EngineSetupConfig& setupConfig) : _viewer(viewer)
        {
            initContext(setupConfig);
            initPipeline(setupConfig);
        }

        void run()
        {
            while (!_viewer->done())
            {
                _viewer->frame();
            }
        }

    private:
        osg::ref_ptr<osgViewer::ViewerBase> _viewer;
        osg::ref_ptr<Pipeline> _pipeline;

        static osg::ref_ptr<osg::GraphicsContext> createGraphicsContext(int width, int height, const char* glContextVersion)
        {
            osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
            traits->width = width; traits->height = height;
            traits->windowDecoration = true;
            traits->doubleBuffer = true;
            traits->glContextVersion = glContextVersion;
            traits->readDISPLAY();
            traits->setUndefinedScreenDetailsToDefaultScreen();
            osg::ref_ptr<osg::GraphicsContext> graphicsContext = osg::GraphicsContext::createGraphicsContext(traits);
            graphicsContext->getState()->setUseModelViewAndProjectionUniforms(true);
            graphicsContext->getState()->setUseVertexAttributeAliasing(true);
            return graphicsContext;
        }

        void initContext(const EngineSetupConfig& setupConfig)
        {
            Context& context = Context::get();
            context._isEditorMode = setupConfig.isEditorMode;
            context._assetsPaths = setupConfig.assetPaths;
        }

        void initPipeline(const EngineSetupConfig& setupConfig)
        {
            osg::ref_ptr<osg::GraphicsContext> graphicsContext = createGraphicsContext(setupConfig.width, setupConfig.height, setupConfig.glContextVersion.c_str());
            osgViewer::Viewer::Views views;
            _viewer->getViews(views);
            assert(views.size() && "Can't find valid view");
            _pipeline = new Pipeline(views.at(0), graphicsContext);
        }
    };
}
