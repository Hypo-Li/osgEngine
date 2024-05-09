// Core.cpp: 定义应用程序的入口点。
//

#include "Core.h"
#include "ImGuiHandler.h"
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>

using namespace std;

class ImGuiInitOperation : public osg::Operation
{
    const char* _glslVersion;
public:
    ImGuiInitOperation(const char* glslVersion = nullptr) : osg::Operation("ImGuiInitOperation", false), _glslVersion(glslVersion) {}

    void operator()(osg::Object* object) override
    {
        osg::GraphicsContext* context = dynamic_cast<osg::GraphicsContext*>(object);
        if (!context)
            return;

        ImGui_ImplOpenGL3_Init(_glslVersion);
    }
};

int main()
{
    const int width = 1280, height = 800;
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
    traits->width = width; traits->height = height;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->glContextVersion = "4.6";
    traits->readDISPLAY();
    traits->setUndefinedScreenDetailsToDefaultScreen();
    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);
    gc->getState()->setUseModelViewAndProjectionUniforms(true);
    gc->getState()->setUseVertexAttributeAliasing(true);

    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);

    viewer->setSceneData(rootGroup);
    viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    //viewer->setKeyEventSetsDone(0); // prevent exit when press esc key
    viewer->setRealizeOperation(new ImGuiInitOperation);
    viewer->addEventHandler(new xxx::ImGuiHandler(viewer));
    //viewer->addEventHandler(new osgViewer::StatsHandler);
    //viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    return viewer->run();
}
