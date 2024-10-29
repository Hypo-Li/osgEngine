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
#include <osg/io_utils>

#include <osgViewer/ViewerEventHandlers>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/SegmentPlacer>
#include <osgParticle/BoxPlacer>

#include <filesystem>

using namespace xxx;

const char* particle_vs = R"(
#version 430 core
in vec3 osg_Vertex;
in vec4 osg_Color;
in vec4 osg_MultiTexCoord0;
out vec4 v2f_Color;
out vec4 v2f_Texcoord0;
uniform mat4 osg_ProjectionMatrix;
uniform mat4 osg_ViewMatrix;
void main()
{
    v2f_Color = osg_Color;
    v2f_Texcoord0 = osg_MultiTexCoord0;
    vec3 cameraPos = osg_ViewMatrixInverse[3].xyz;
    gl_Position = osg_ProjectionMatrix * osg_ViewMatrix * vec4(cameraPos + osg_Vertex, 1.0);
    gl_Position *= 1.0 / gl_Position.w;
    gl_Position.z = 1.0;
}
)";

const char* particle_fs = R"(
#version 430 core
in vec4 v2f_Color;
in vec4 v2f_Texcoord0;
out vec4 fragData;
uniform sampler2D baseTexture;
void main()
{
    fragData = v2f_Color * texture(baseTexture, v2f_Texcoord0.xy);
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
    inputPass1->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    inputPass1->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));

    xxx::Pipeline::Pass* displayPass = pipeline1->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(inputPass1->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    osgParticle::ParticleSystem* ps = new osgParticle::ParticleSystem;
    ps->setCullingActive(false);
    ps->setDefaultAttributesUsingShaders(TEMP_DIR "snowflake.png", true, false);
    ps->setUseVertexArray(false);
    ps->getDefaultParticleTemplate().setShape(osgParticle::Particle::QUAD);
    ps->getDefaultParticleTemplate().setSizeRange(osgParticle::rangef(0.005f, 0.005f));
    ps->getDefaultParticleTemplate().setLifeTime(10);
    osg::Program* particleProgram = new osg::Program;
    particleProgram->addShader(new osg::Shader(osg::Shader::VERTEX, particle_vs));
    particleProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, particle_fs));
    ps->getOrCreateStateSet()->setAttribute(particleProgram, osg::StateAttribute::ON);
    ps->getOrCreateStateSet()->setAttribute(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
    ps->getOrCreateStateSet()->setAttribute(new osg::Depth(osg::Depth::ALWAYS, 0.0f, 1.0f, false));

    osgParticle::ModularEmitter* emitter = new osgParticle::ModularEmitter;
    emitter->setParticleSystem(ps);
    emitter->setCullingActive(false);
    /*osgParticle::SegmentPlacer* placer = new osgParticle::SegmentPlacer;
    placer->setSegment(osg::Vec3(20, 0, 10), osg::Vec3(-20, 0, 10));*/
    osgParticle::BoxPlacer* placer = new osgParticle::BoxPlacer;
    placer->setXRange(-1.0f, 1.0f);
    placer->setYRange(-1.0f, 1.0f);
    placer->setZRange(1.0f, 1.1f);
    emitter->setPlacer(placer);

    osgParticle::RandomRateCounter* rrc = dynamic_cast<osgParticle::RandomRateCounter*>(emitter->getCounter());
    rrc->setRateRange(100, 200);

    osgParticle::RadialShooter* rs = dynamic_cast<osgParticle::RadialShooter*>(emitter->getShooter());
    rs->setThetaRange(osg::PIf * 0.9, osg::PIf * 1.1);
    rs->setInitialSpeedRange(osgParticle::rangef(0.2f, 0.3f));

    osgParticle::ParticleSystemUpdater* psu = new osgParticle::ParticleSystemUpdater;
    psu->addParticleSystem(ps);
    psu->setCullingActive(false);

    osg::Group* root = new osg::Group;
    root->addChild(ps);
    root->addChild(emitter);
    root->addChild(psu);
    view1->setSceneData(root);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->realize();

    while (!viewer->done())
        viewer->frame();
    return 0;
}

#endif // 0
