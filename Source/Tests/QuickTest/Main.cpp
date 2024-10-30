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
#include <osg/LineWidth>

#include <osgViewer/ViewerEventHandlers>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/SegmentPlacer>
#include <osgParticle/BoxPlacer>
#include <osgParticle/PrecipitationEffect>

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
    vec4 clipSpace = osg_ProjectionMatrix * mat4(mat3(osg_ViewMatrix)) * vec4(osg_Vertex, 1.0);
    clipSpace *= 1.0 / clipSpace.w;
    clipSpace.z = max(clipSpace.z, -0.99);
    gl_Position = clipSpace;
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

const char* mesh_vs = R"(
#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;
out vec3 v2f_FragPos;
out vec3 v2f_Normal;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform mat3 osg_NormalMatrix;
void main()
{
    v2f_Normal = osg_NormalMatrix * osg_Normal;
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    v2f_FragPos = viewSpace.xyz;
    gl_Position = osg_ProjectionMatrix * viewSpace;
}
)";

const char* mesh_fs = R"(
#version 430 core
in vec3 v2f_FragPos;
in vec3 v2f_Normal;
out vec4 fragData;
void main()
{
    vec3 normal = normalize(v2f_Normal);
    vec3 viewDir = normalize(-v2f_FragPos);
    float NoV = max(dot(normal, viewDir), 0.0);
    fragData = vec4(vec3(1, 0, 0) * NoV, 1);
}
)";

osg::Node* createSnowParticleNode()
{
    osgParticle::ParticleSystem* ps = new osgParticle::ParticleSystem;
    ps->setCullingActive(false);
    ps->setDefaultAttributesUsingShaders(TEMP_DIR "alpha.png", true, false);
    ps->setUseVertexArray(false);
    ps->getDefaultParticleTemplate().setShape(osgParticle::Particle::QUAD);
    ps->getDefaultParticleTemplate().setSizeRange(osgParticle::rangef(0.002f, 0.002f));
    ps->getDefaultParticleTemplate().setLifeTime(10);
    osg::Program* particleProgram = new osg::Program;
    particleProgram->addShader(new osg::Shader(osg::Shader::VERTEX, particle_vs));
    particleProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, particle_fs));
    ps->getOrCreateStateSet()->setAttribute(particleProgram, osg::StateAttribute::ON);
    ps->getOrCreateStateSet()->setAttribute(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);

    osgParticle::ModularEmitter* emitter = new osgParticle::ModularEmitter;
    emitter->setParticleSystem(ps);
    emitter->setCullingActive(false);
    osgParticle::BoxPlacer* placer = new osgParticle::BoxPlacer;
    placer->setXRange(-0.5f, 0.5f);
    placer->setYRange(-0.5f, 0.5f);
    placer->setZRange(1.0f, 1.1f);
    emitter->setPlacer(placer);

    osgParticle::RandomRateCounter* rrc = dynamic_cast<osgParticle::RandomRateCounter*>(emitter->getCounter());
    rrc->setRateRange(100, 150);

    osgParticle::RadialShooter* rs = dynamic_cast<osgParticle::RadialShooter*>(emitter->getShooter());
    rs->setThetaRange(osg::PIf * 0.9, osg::PIf * 1.1);
    rs->setInitialSpeedRange(osgParticle::rangef(0.2f, 0.3f));

    osgParticle::ParticleSystemUpdater* psu = new osgParticle::ParticleSystemUpdater;
    psu->addParticleSystem(ps);
    psu->setCullingActive(false);

    osg::Group* group = new osg::Group;
    group->addChild(ps);
    group->addChild(emitter);
    group->addChild(psu);
    return group;
}

osg::Node* createRainParticleNode()
{
    osgParticle::ParticleSystem* ps = new osgParticle::ParticleSystem;
    ps->setCullingActive(false);
    ps->setDefaultAttributesUsingShaders("", true, false);
    ps->setUseVertexArray(false);
    osgParticle::Particle& defaultParticle = ps->getDefaultParticleTemplate();
    defaultParticle.setShape(osgParticle::Particle::LINE);
    defaultParticle.setSizeRange(osgParticle::rangef(0.03f, 0.03f));
    defaultParticle.setAlphaRange(osgParticle::rangef(0.2, 0.25));
    defaultParticle.setLifeTime(2);
    osg::Program* particleProgram = new osg::Program;
    particleProgram->addShader(new osg::Shader(osg::Shader::VERTEX, particle_vs));
    particleProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, particle_fs));
    ps->getOrCreateStateSet()->setAttribute(particleProgram, osg::StateAttribute::ON);
    ps->getOrCreateStateSet()->setAttribute(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
    ps->getOrCreateStateSet()->setAttribute(new osg::LineWidth(2.0f), osg::StateAttribute::ON);
    ps->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
    

    osgParticle::ModularEmitter* emitter = new osgParticle::ModularEmitter;
    emitter->setParticleSystem(ps);
    emitter->setCullingActive(false);
    osgParticle::BoxPlacer* placer = new osgParticle::BoxPlacer;
    placer->setXRange(-0.5f, 0.5f);
    placer->setYRange(-0.5f, 0.5f);
    placer->setZRange(1.0f, 1.1f);
    emitter->setPlacer(placer);

    osgParticle::RandomRateCounter* rrc = dynamic_cast<osgParticle::RandomRateCounter*>(emitter->getCounter());
    rrc->setRateRange(400, 400);

    osgParticle::RadialShooter* rs = dynamic_cast<osgParticle::RadialShooter*>(emitter->getShooter());
    rs->setThetaRange(osg::PIf, osg::PIf);
    rs->setPhiRange(0.0, 0.0);
    rs->setInitialSpeedRange(osgParticle::rangef(2.0, 2.0f));

    osgParticle::ParticleSystemUpdater* psu = new osgParticle::ParticleSystemUpdater;
    psu->addParticleSystem(ps);
    psu->setCullingActive(false);

    osg::Group* group = new osg::Group;
    group->addChild(ps);
    group->addChild(emitter);
    group->addChild(psu);
    return group;
}

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

    osg::Node* mesh = osgDB::readNodeFile(TEMP_DIR "suzanne.obj");
    osg::Program* meshProgram = new osg::Program;
    meshProgram->addShader(new osg::Shader(osg::Shader::VERTEX, mesh_vs));
    meshProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, mesh_fs));
    mesh->getOrCreateStateSet()->setAttribute(meshProgram, osg::StateAttribute::ON);

    osg::Group* root = new osg::Group;
    root->addChild(mesh);
    root->addChild(createRainParticleNode());
    root->addChild(createSnowParticleNode());
    view1->setSceneData(root);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->realize();

    while (!viewer->done())
        viewer->frame();
    return 0;
}

#endif // 0
