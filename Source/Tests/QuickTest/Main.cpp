#if 1
#include <Engine/Core/Engine.h>
#include <Engine/Render/Pipeline.h>
#include <Engine/Core/Asset.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Component/MeshRenderer.h>
#include <Engine/Component/Light.h>

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
#include <osgParticle/ParticleSystem>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystemUpdater>

#include <filesystem>

using namespace xxx;

const char* particle_vs = R"(
#version 430 core
in vec4 osg_Vertex;
in vec4 osg_Color;
out vec4 v2f_Color;
uniform mat4 osg_ModelViewProjectionMatrix;
void main()
{
    v2f_Color = osg_Color;
    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}
)";

const char* particle_fs = R"(
#version 430 core
in vec4 v2f_Color;
out vec4 fragData;
void main()
{
    fragData = v2f_Color;
}
)";

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

    osg::ref_ptr<osgViewer::CompositeViewer> viewer = new osgViewer::CompositeViewer;

    osg::ref_ptr<osgViewer::View> view1 = new osgViewer::View;
    viewer->addView(view1);
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

    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));

    xxx::Pipeline::Pass* displayPass = pipeline1->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(inputPass1->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    osgParticle::ParticleSystem* ps = new osgParticle::ParticleSystem;
    ps->setDefaultAttributesUsingShaders("", true, false);
    ps->getDefaultParticleTemplate().setShape(osgParticle::Particle::QUAD);
    osg::Program* particleProgram = new osg::Program;
    particleProgram->addShader(new osg::Shader(osg::Shader::VERTEX, particle_vs));
    particleProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, particle_fs));
    ps->getOrCreateStateSet()->setAttribute(particleProgram, osg::StateAttribute::ON);

    osgParticle::ModularEmitter* emitter = new osgParticle::ModularEmitter;
    emitter->setParticleSystem(ps);

    osgParticle::RandomRateCounter* rrc = static_cast<osgParticle::RandomRateCounter*>(emitter->getCounter());
    rrc->setRateRange(20, 30);

    osgParticle::ParticleSystemUpdater* psu = new osgParticle::ParticleSystemUpdater;
    psu->addParticleSystem(ps);

    osg::Group* root = new osg::Group;
    root->addChild(emitter);
    root->addChild(ps);
    root->addChild(psu);
    view1->setSceneData(root);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->realize();

    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}

#endif // 0
