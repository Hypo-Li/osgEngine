#if 1

#include <Engine/Render/Pipeline.h>
#include <Engine/Core/Asset.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Component/MeshRenderer.h>

#include <osgViewer/CompositeViewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>
#include <osg/Texture3D>
#include <osg/DispatchCompute>
#include <osg/BindImageTexture>

#include <osgViewer/ViewerEventHandlers>
#include "../Core/DebugCallback.h"

#include <filesystem>

using namespace xxx;

int main()
{
    const int width = 1024, height = 768;
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

    Asset* asset = AssetManager::get().getAsset("Engine/TestEntity.xast");
    asset->load();

    osg::ref_ptr<osgViewer::CompositeViewer> viewer = new osgViewer::CompositeViewer;

    osg::ref_ptr<osgViewer::View> view1 = new osgViewer::View;
    viewer->addView(view1);
    view1->setSceneData(dynamic_cast<Entity*>(asset->getRootObject())->getOsgNode());
    view1->setCameraManipulator(new osgGA::TrackballManipulator);
    view1->addEventHandler(new osgViewer::StatsHandler);
    osg::ref_ptr<osg::Camera> camera1 = view1->getCamera();
    camera1->setGraphicsContext(gc);
    camera1->setViewport(0, 0, width, height);
    camera1->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);

    osg::ref_ptr<xxx::Pipeline> pipeline1 = new xxx::Pipeline(view1, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    xxx::Pipeline::Pass* inputPass1 = pipeline1->addInputPass("Input1", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    inputPass1->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass1->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

    osg::ref_ptr<osgViewer::View> view2 = new osgViewer::View;
    viewer->addView(view2);
    view2->setSceneData(dynamic_cast<Entity*>(asset->getRootObject())->getOsgNode());
    view2->setCameraManipulator(new osgGA::TrackballManipulator);
    view2->addEventHandler(new osgViewer::StatsHandler);
    osg::ref_ptr<osg::Camera> camera2 = view2->getCamera();
    camera2->setGraphicsContext(gc);
    camera2->setViewport(0, 0, width * 2, height * 2);
    camera2->setProjectionMatrixAsPerspective(30.0, double(width) / double(height), 0.1, 400.0);

    osg::ref_ptr<xxx::Pipeline> pipeline2 = new xxx::Pipeline(view2, gc);
    xxx::Pipeline::Pass* inputPass2 = pipeline2->addInputPass("Input2", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    inputPass2->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass2->attach(BufferType::COLOR_BUFFER1, GL_RGBA8);
    inputPass2->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);
    inputPass2->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);
    inputPass2->attach(BufferType::COLOR_BUFFER4, GL_RGBA8);
    inputPass2->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

    //osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, (currentPath / "Shader/ScreenQuad.vert.glsl").string());

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setRealizeOperation(new EnableGLDebugOperation);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}

#endif // 0
