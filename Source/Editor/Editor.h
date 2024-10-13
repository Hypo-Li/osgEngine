#pragma once
#include "ImGuiHandler.h"
#include "PickEventHandler.h"
#include <Engine/Core/Engine.h>

#include <osgViewer/CompositeViewer>

namespace xxx
{
    struct EditorSetupConfig
    {

    };

    class Editor
    {
    public:
        Editor()
        {
            mViewer = new osgViewer::CompositeViewer;

            EngineSetupConfig engineSetupConfig;
            engineSetupConfig.width = 1024;
            engineSetupConfig.height = 768;
            engineSetupConfig.glContextVersion = "4.6";
            engineSetupConfig.fullScreen = false;
            engineSetupConfig.runMode = RunMode::Edit;
            mEngine = new Engine(engineSetupConfig);

            mViewer->setRealizeOperation(new ImGuiInitOperation);
            mViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

            Pipeline* enginePipeline = mEngine->getPipeline();
            uint32_t passCount = enginePipeline->getPassCount();
            osg::Camera* imguiCamera = enginePipeline->getPass(passCount - 1)->getCamera();
            osg::Texture2D* sceneColorTexture = dynamic_cast<osg::Texture2D*>(enginePipeline->getPass(passCount - 2)->getBufferTexture(Pipeline::Pass::BufferType::COLOR_BUFFER0));
            osg::ref_ptr<ImGuiHandler> imguiHandler = new ImGuiHandler(imguiCamera, sceneColorTexture, [this](int w, int h) {mEngine->getPipeline()->resize(w, h, false); });
            osgViewer::View* engineView = mEngine->getView();
            engineView->addEventHandler(imguiHandler);
            engineView->addEventHandler(new PickEventHandler(engineView->getCamera(), imguiHandler->getSceneViewViewport()));

            mViewer->addView(engineView);
        }

        void run()
        {
            mViewer->realize();
            while (!mViewer->done())
            {
                mViewer->frame();
            }
        }

    protected:
        Engine* mEngine;
        osg::ref_ptr<osgViewer::CompositeViewer> mViewer;

        //void drawSceneView();
        //void drawInspector();
        //void drawHierarchy();
        //void drawAssetBrowser();
    };
}
