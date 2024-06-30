#pragma once
#include <Core/Render/Pipeline.h>
namespace xxx
{
    class Editor
    {
    public:
        void initialize()
        {
            
        }
        void run();
        void terminate();

    private:

        void drawSceneView();
        void drawInspector();
        void drawHierarchy();
        void drawAssetBrowser();
    };
}
