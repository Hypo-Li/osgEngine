#include <osgViewer/Viewer>
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osg/Depth>
#include <osg/CullFace>
#include <Engine/Render/Pipeline.h>

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf inverseProjectionMatrix;
    osg::Vec4f gbufferNearFar;
    osg::Vec4f transparentNearFar;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

#define EARTH_MASK          0x00000001
#define GBUFFER_MASK        0x00000002
#define TRANS_MASK          0x00000004
#define FORWARD_OPAQUE_MASK 0x00000008
#define FORWARD_TRANS_MASK  0x00000010

class GBufferPassPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        osg::Matrixd& viewMatrix = renderInfo.getCurrentCamera()->getViewMatrix();
        osg::Matrixd& inverseViewMatrix = renderInfo.getCurrentCamera()->getInverseViewMatrix();
        osg::Matrixd& projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        osg::Matrixd inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);

        gViewData.viewMatrix = viewMatrix;
        gViewData.inverseViewMatrix = inverseViewMatrix;
        gViewData.projectionMatrix = projectionMatrix;
        gViewData.inverseProjectionMatrix = inverseProjectionMatrix;

        double col3z = projectionMatrix(2, 2), col4z = projectionMatrix(3, 2);
        double zNear = col4z / (col3z - 1.0), zFar = col4z / (col3z + 1.0);
        gViewData.gbufferNearFar = osg::Vec4f(zNear, zFar, col3z, col4z);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

class TransparentPassPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        osg::Matrixd& projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        osg::Matrixd inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);

        gViewData.projectionMatrix = projectionMatrix;
        gViewData.inverseProjectionMatrix = inverseProjectionMatrix;

        double col3z = projectionMatrix(2, 2), col4z = projectionMatrix(3, 2);
        double zNear = col4z / (col3z - 1.0), zFar = col4z / (col3z + 1.0);
        gViewData.transparentNearFar = osg::Vec4f(zNear, zFar, col3z, col4z);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

static const char* vs = R"(
#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
out vec3 v2f_fragPos;
out vec3 v2f_normal;
out vec4 v2f_color;
uniform mat3 osg_NormalMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f_fragPos = viewSpace.xyz;
    v2f_normal = osg_NormalMatrix * osg_Normal;
    v2f_color = osg_Color;
}
)";

static const char* trans_vs = R"(
#version 430 core
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec4 osg_Color;
out vec3 v2f_fragPos;
out vec3 v2f_normal;
out vec4 v2f_color;
out float v2f_gndcz;
uniform mat3 osg_NormalMatrix;
uniform mat4 osg_ModelViewMatrix;
uniform mat4 osg_ProjectionMatrix;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    vec4 uGbufferNearFar;
    vec4 uTransparentNearFar;
};

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f_fragPos = viewSpace.xyz;
    v2f_normal = osg_NormalMatrix * osg_Normal;
    v2f_color = osg_Color;
    mat4 gbufferProjection = osg_ProjectionMatrix;
    gbufferProjection[2][2] = uGbufferNearFar.z;
    gbufferProjection[3][2] = uGbufferNearFar.w;
    vec4 gbufferClipSpace = gbufferProjection * viewSpace;
    v2f_gndcz = gbufferClipSpace.z / gbufferClipSpace.w;
}
)";

static const char* fs = R"(
#version 430 core
in vec3 v2f_fragPos;
in vec3 v2f_normal;
in vec4 v2f_color;
out vec4 fragData;
void main()
{
    vec3 normal = normalize(v2f_normal);
    vec3 viewDir = normalize(-v2f_fragPos);
    float ndv = max(dot(normal, viewDir), 0.0);
    fragData = vec4(v2f_color.rgb * ndv, 1.0);
}
)";

static const char* trans_fs = R"(
#version 430 core
in vec3 v2f_fragPos;
in vec3 v2f_normal;
in vec4 v2f_color;
in float v2f_gndcz;
out vec4 fragData;
void main()
{
    vec3 normal = normalize(v2f_normal);
    vec3 viewDir = normalize(-v2f_fragPos);
    float ndv = max(dot(normal, viewDir), 0.0);
    float gdepth = clamp(v2f_gndcz * 0.5 + 0.5, 0.0000001, 0.9999999);
    gl_FragDepth = gdepth;
    fragData = vec4(v2f_color.rgb * ndv, 0.5);
}
)";

int main()
{
    // init ViewData UBO
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

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
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);
    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
    viewer->setSceneData(rootGroup);

    osg::Program* colorProgram = new osg::Program;
    colorProgram->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    colorProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));

    osg::Node* gbufferNode = osgDB::readNodeFile(TEMP_DIR "gbuffer.obj");
    gbufferNode->getOrCreateStateSet()->setAttribute(colorProgram, osg::StateAttribute::ON);
    gbufferNode->setNodeMask(GBUFFER_MASK);
    rootGroup->addChild(gbufferNode);

    osg::Program* transColorProgram = new osg::Program;
    transColorProgram->addShader(new osg::Shader(osg::Shader::VERTEX, trans_vs));
    transColorProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, trans_fs));

    osg::Node* transNode = osgDB::readNodeFile(TEMP_DIR "trans.obj");
    transNode->getOrCreateStateSet()->setAttribute(transColorProgram, osg::StateAttribute::ON);
    transNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    transNode->setNodeMask(TRANS_MASK);
    rootGroup->addChild(transNode);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", GBUFFER_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32F);
    gbufferPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    gbufferPass->getCamera()->setPreDrawCallback(new GBufferPassPreDrawCallback);

    osg::ref_ptr<xxx::Pipeline::Pass> transparentPass = pipeline->addInputPass("Transparent", TRANS_MASK, 0);
    transparentPass->attach(BufferType::COLOR_BUFFER0, gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
    transparentPass->attach(BufferType::DEPTH_BUFFER, gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER));
    transparentPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setMode(GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->getCamera()->setPreDrawCallback(new TransparentPassPreDrawCallback);

    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
