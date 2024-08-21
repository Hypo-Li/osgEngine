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
out vec4 fragData;
uniform vec4 uColor;
void main()
{
    fragData = uColor;
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
    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    return geometry;
}

osg::Geode* createGeode1()
{
    osg::Geode* geode = new osg::Geode;
    osg::Geometry* geometry = createGeometry();
    geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::DEFAULT_BIN);
    geode->addDrawable(geometry);
    geode->getOrCreateStateSet()->addUniform(new osg::Uniform("uColor", osg::Vec4(1, 0, 0, 1)));
    geode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    return geode;
}

osg::Geode* createGeode2()
{
    osg::Geode* geode = new osg::Geode;
    osg::Geometry* geometry = createGeometry();
    geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::DEFAULT_BIN);
    geode->addDrawable(geometry);
    geode->getOrCreateStateSet()->addUniform(new osg::Uniform("uColor", osg::Vec4(0, 1, 0, 1)));
    geode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
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

    osg::MatrixTransform* transform1 = new osg::MatrixTransform;
    transform1->setMatrix(osg::Matrixd::translate(osg::Vec3d(0, 0, 1)));
    transform1->addChild(createGeode1());
    rootGroup->addChild(transform1);

    osg::MatrixTransform* transform2 = new osg::MatrixTransform;
    transform2->setMatrix(osg::Matrixd::translate(osg::Vec3d(0, 0, -1)));
    transform2->addChild(createGeode2());
    rootGroup->addChild(transform2);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    //gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* finalProgram = new osg::Program;
    finalProgram->addShader(screenQuadShader);
    finalProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", finalProgram);
    finalPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
