#if 0

#include <Engine/Render/Pipeline.h>
#include <osgViewer/Viewer>
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

int main()
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    const int width = 1920, height = 1080;
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

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
    viewer->setSceneData(rootGroup);
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, (currentPath / "Shader/ScreenQuad.vert.glsl").string());

    osg::ref_ptr<osg::Program> colorGradingLutProgram = new osg::Program;
    colorGradingLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, (currentPath / "Shader/ColorGrading.comp.glsl").string()));
    osg::ref_ptr<osg::Texture3D> colorGradingLutTexture = new osg::Texture3D;
    colorGradingLutTexture->setTextureSize(32, 32, 32);
    colorGradingLutTexture->setInternalFormat(GL_RGBA16F_ARB);
    osg::ref_ptr<osg::BindImageTexture> colorGradingLutImage = new osg::BindImageTexture(0, colorGradingLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F_ARB, 0, GL_TRUE);
    osg::ref_ptr<osg::DispatchCompute> colorGradingLutDispatch = new osg::DispatchCompute(4, 4, 4);
    colorGradingLutDispatch->getOrCreateStateSet()->setAttributeAndModes(colorGradingLutProgram);
    colorGradingLutDispatch->getOrCreateStateSet()->setAttributeAndModes(colorGradingLutImage);
    colorGradingLutDispatch->setCullingActive(false);
    rootGroup->addChild(colorGradingLutDispatch);

    osg::ref_ptr<xxx::Pipeline::Pass> input1Pass = pipeline->addInputPass("Input1", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    input1Pass->attach(BufferType::COLOR_BUFFER, GL_RGBA8);
    input1Pass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24, osg::Texture::NEAREST, osg::Texture::NEAREST);

    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, (currentPath / "Shader/Display.frag.glsl").string()));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(colorGradingLutTexture, "uColorTexture", 0);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->setRealizeOperation(new EnableGLDebugOperation);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}

#endif // 0
