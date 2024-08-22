#include <Engine/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>

static osg::Vec3 positions[] = {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
};

static const char* vs = R"(
#version 430 core
in vec4 osg_Vertex;
uniform mat4 osg_ModelViewProjectionMatrix;
void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}
)";

static const char* fs = R"(
#version 430 core
#pragma import_defines(INPUT_PASS)
out vec4 fragData;
void main()
{
#if (INPUT_PASS == 1)
    fragData = vec4(1, 0, 0, 1);
#elif (INPUT_PASS == 2)
    fragData = vec4(0, 1, 0, 1);
#else
    fragData = vec4(0, 0, 1, 1);
#endif
}
)";

osg::Geometry* createGeometry()
{
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(new osg::Vec3Array(std::begin(positions), std::end(positions)));
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setAttribute(program, osg::StateAttribute::ON);
    return geometry;
}

osg::Geode* createGeode()
{
    osg::Geode* geode = new osg::Geode;
    osg::Geometry* geometry = createGeometry();
    geode->addDrawable(geometry);
    return geode;
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
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);

    rootGroup->addChild(createGeode());

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;

    osg::ref_ptr<xxx::Pipeline::Pass> input1Pass = pipeline->addInputPass("Input1", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    input1Pass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    input1Pass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    input1Pass->getCamera()->getOrCreateStateSet()->setDefine("INPUT_PASS", "1");

    osg::ref_ptr<xxx::Pipeline::Pass> input2Pass = pipeline->addInputPass("Input2", 0x00000002, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    input2Pass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    input2Pass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    input2Pass->getCamera()->getOrCreateStateSet()->setDefine("INPUT_PASS", "2");

    osg::ref_ptr<xxx::Pipeline::Pass> earthPass = pipeline->addInputPass("Earth", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000002, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> shadowCastPass = pipeline->addInputPass("ShadowCast", 0x00000004, GL_DEPTH_BUFFER_BIT, true, osg::Vec2(2048, 2048));
    osg::ref_ptr<xxx::Pipeline::Pass> shadowMaskPass = pipeline->addWorkPass("ShadowMask", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> ssaoPass = pipeline->addWorkPass("SSAO", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> lightingPass = pipeline->addWorkPass("Lighting", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> atmospherePass = pipeline->addWorkPass("Atmosphere", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> transparentPass = pipeline->addInputPass("Transparency", 0x00000008, 0);
    osg::ref_ptr<xxx::Pipeline::Pass> taaPass = pipeline->addWorkPass("TAA", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> bloomPass = pipeline->addWorkPass("Bloom", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> colorGradingPass = pipeline->addWorkPass("ColorGrading", new osg::Program, GL_COLOR_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> forwardOpaquePass = pipeline->addInputPass("ForwardOpaque", 0x00000010, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    osg::ref_ptr<xxx::Pipeline::Pass> forwardTransparentPass = pipeline->addInputPass("ForwardTransparent", 0x00000020, 0);
    osg::ref_ptr<xxx::Pipeline::Pass> uiPass = pipeline->addInputPass("UI", 0x00000040, 0);

    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(input1Pass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
