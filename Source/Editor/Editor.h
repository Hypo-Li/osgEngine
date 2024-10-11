#pragma once
#include "ImGuiHandler.h"
#include "PickEventHandler.h"
#include <Engine/Core/Engine.h>

#include <osgGA/TrackballManipulator>

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
            mViewer = new osgViewer::Viewer;

            EngineSetupConfig engineSetupConfig;
            engineSetupConfig.width = 1024;
            engineSetupConfig.height = 768;
            engineSetupConfig.glContextVersion = "4.6";
            engineSetupConfig.fullScreen = false;
            engineSetupConfig.runMode = RunMode::Edit;
            mEngine = new Engine(static_cast<osgViewer::View*>(mViewer.get()), engineSetupConfig);

            mViewer->setRealizeOperation(new ImGuiInitOperation);
            osg::ref_ptr<ImGuiHandler> imguiHandler = new ImGuiHandler(mViewer, mEngine->getPipeline());
            mViewer->addEventHandler(imguiHandler);
            mViewer->addEventHandler(new PickEventHandler(mViewer->getCamera(), imguiHandler->getSceneViewViewport()));

            mViewer->setCameraManipulator(new osgGA::TrackballManipulator);
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
        osg::ref_ptr<osgViewer::Viewer> mViewer;

        //void drawSceneView();
        //void drawInspector();
        //void drawHierarchy();
        //void drawAssetBrowser();
    };
}
