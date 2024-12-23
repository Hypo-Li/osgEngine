#include <Engine/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/ClipControl>
#include <osg/Depth>
#include <osg/MatrixTransform>

static const char* vs = R"(
#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;

out vec3 v2f_Normal;

uniform mat4 osg_ModelViewProjectionMatrix;

void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
    v2f_Normal = normalize(osg_Normal);
}
)";

static const char* fs = R"(
#version 430 core
in vec3 v2f_Normal;
out vec4 fragData;
void main()
{
    fragData = vec4(normalize(v2f_Normal) * 0.5 + 0.5, 1.0);
}
)";

static const char* fs_red = R"(
#version 430 core
in vec3 v2f_Normal;
out vec4 fragData;
void main()
{
    fragData = vec4(vec3(1.0, 0.5, 0.5) * (normalize(v2f_Normal) * 0.5 + 0.5), 1.0);
}
)";

static const char* fs_green = R"(
#version 430 core
in vec3 v2f_Normal;
out vec4 fragData;
void main()
{
    fragData = vec4(vec3(0.5, 1.0, 0.5) * (normalize(v2f_Normal) * 0.5 + 0.5), 1.0);
}
)";

#define ENABLE_REVERSE_Z 1

osg::Node* getScene()
{
    osg::Program* redProgram = new osg::Program;
    redProgram->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    redProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs_red));

    osg::Program* greenProgram = new osg::Program;
    greenProgram->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    greenProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs_green));

    osg::Node* cubeMesh = osgDB::readNodeFile(TEMP_DIR R"(cube.obj)");
    cubeMesh->getOrCreateStateSet()->setAttributeAndModes(redProgram);
    osg::MatrixTransform* cubeMT = new osg::MatrixTransform;
    cubeMT->addChild(cubeMesh);
    cubeMT->setMatrix(osg::Matrixd::scale(500.0, 500.0, 500.0) * osg::Matrixd::translate(0, 0, 0));

    osg::Node* cubeMesh1 = osgDB::readNodeFile(TEMP_DIR R"(cube.obj)");
    cubeMesh1->getOrCreateStateSet()->setAttributeAndModes(greenProgram);
    osg::MatrixTransform* cubeMT1 = new osg::MatrixTransform;
    cubeMT1->addChild(cubeMesh1);
    cubeMT1->setMatrix(osg::Matrixd::scale(500.0, 500.0, 500.0) * osg::Matrixd::translate(0, 0, 0.005));

    /*osg::Node* sphereMesh = osgDB::readNodeFile(TEMP_DIR R"(sphere.obj)");
    sphereMesh->getOrCreateStateSet()->setAttributeAndModes(program);
    osg::MatrixTransform* sphereMT = new osg::MatrixTransform;
    sphereMT->addChild(sphereMesh);
    sphereMT->setMatrix(osg::Matrixd::translate(osg::Vec3d(2, 0, 0)));*/

    osg::Group* sceneGroup = new osg::Group;
    sceneGroup->addChild(cubeMT);
    sceneGroup->addChild(cubeMT1);

    return sceneGroup;
}

int main()
{
    const int width = 1280, height = 720;
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

    double zNear = 1;
    double zFar = 1000.0;
#if ENABLE_REVERSE_Z
    double f = 1.0f / std::tan(osg::DegreesToRadians(90.0) * 0.5);
    camera->setProjectionMatrix(osg::Matrixd(
        f / (width / double(height)), 0.0, 0.0, 0.0,
        0.0, f, 0.0, 0.0,
        0.0, 0.0, 0.0, -1.0,
        0.0, 0.0, zNear, 0.0
    ));
#else
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), zNear, zFar);
#endif

    rootGroup->addChild(getScene());

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32F, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    gbufferPass->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
#if ENABLE_REVERSE_Z
    gbufferPass->getCamera()->setClearDepth(0.0f);
    gbufferPass->getCamera()->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::GREATER));
    gbufferPass->getCamera()->getOrCreateStateSet()->setAttributeAndModes(new osg::ClipControl(osg::ClipControl::LOWER_LEFT, osg::ClipControl::ZERO_TO_ONE));
#endif

    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->realize();

    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
