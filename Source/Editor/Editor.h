#pragma once
#include "ImGuiHandler.h"
#include <Engine/Core/Engine.h>
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
            EngineSetupConfig engineSetupConfig;
            engineSetupConfig.width = 1920;
            engineSetupConfig.height = 1080;
            engineSetupConfig.glContextVersion = "4.6";
            engineSetupConfig.fullScreen = false;
            engineSetupConfig.runMode = RunMode::Edit;
            mEngine = new Engine(engineSetupConfig);

            mEngine->getViewer()->setRealizeOperation(new xxx::ImGuiInitOperation);
        }

        void run();

    protected:
        Engine* mEngine;

        void drawSceneView();
        void drawInspector();
        void drawHierarchy();
        void drawAssetBrowser();
    };
}
