#include "ImGuiHandler.h"
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/StateSetManipulator>

//int main()
//{
//    const int width = 1280, height = 800;
//    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
//    traits->width = width; traits->height = height;
//    traits->windowDecoration = true;
//    traits->doubleBuffer = true;
//    traits->glContextVersion = "4.6";
//    traits->readDISPLAY();
//    traits->setUndefinedScreenDetailsToDefaultScreen();
//    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);
//    gc->getState()->setUseModelViewAndProjectionUniforms(true);
//    gc->getState()->setUseVertexAttributeAliasing(true);
//
//    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
//
//    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
//    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
//    camera->setGraphicsContext(gc);
//    camera->setViewport(0, 0, width, height);
//
//    viewer->setSceneData(rootGroup);
//    //viewer->setKeyEventSetsDone(0); // prevent exit when press esc key
//    viewer->setRealizeOperation(new xxx::ImGuiInitOperation);
//    viewer->addEventHandler(new xxx::ImGuiHandler(viewer));
//    //viewer->addEventHandler(new osgViewer::StatsHandler);
//    //viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
//    return viewer->run();
//}
