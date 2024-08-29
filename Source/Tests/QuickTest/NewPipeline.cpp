#if 1

#include <osgViewer/Viewer>
#include <osg/BufferObject>
#include <osg/BufferIndexBinding>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/Multisample>
#include <Engine/Render/Pipeline.h>


void debugCallback(GLenum source, GLenum type, GLuint, GLenum severity,
    GLsizei, const GLchar* message, const void*)
{
    std::string srcStr = "UNDEFINED";
    switch (source)

    {
    case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "WINDOW_SYSTEM"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "SHADER_COMPILER"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "THIRD_PARTY"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "APPLICATION"; break;
    case GL_DEBUG_SOURCE_OTHER:           srcStr = "OTHER"; break;
    }

    std::string typeStr = "UNDEFINED";

    osg::NotifySeverity osgSeverity = osg::DEBUG_INFO;
    switch (type)
    {

    case GL_DEBUG_TYPE_ERROR:
        //	__debugbreak();
        typeStr = "ERROR";
        osgSeverity = osg::FATAL;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "DEPRECATED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "UNDEFINED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "PORTABILITY"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "PERFORMANCE"; break;
    case GL_DEBUG_TYPE_OTHER:               typeStr = "OTHER"; break;
    }


    osg::notify(osgSeverity) << "OpenGL " << typeStr << " [" << srcStr << "]: " << message << std::endl;

}

void enableGLDebugExtension(int context_id)
{
    //create the extensions
    PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl = nullptr;
    PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = nullptr;
    if (osg::isGLExtensionSupported(context_id, "GL_KHR_debug"))
    {
        osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallback");
        osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControl");

    }
    else if (osg::isGLExtensionSupported(context_id, "GL_ARB_debug_output"))
    {
        osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackARB");
        osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlARB");
    }
    else if (osg::isGLExtensionSupported(context_id, "GL_AMD_debug_output"))
    {
        osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackAMD");
        osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlAMD");
    }

    if (glDebugMessageCallback != nullptr && glDebugMessageControl != nullptr)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, GL_TRUE);
        glDebugMessageCallback(debugCallback, nullptr);
    }
}

class EnableGLDebugOperation : public osg::GraphicsOperation
{
public:
    EnableGLDebugOperation()
        : osg::GraphicsOperation("EnableGLDebugOperation", false) {
    }
    virtual void operator ()(osg::GraphicsContext* gc) {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        int context_id = gc->getState()->getContextID();
        enableGLDebugExtension(context_id);
    }
    OpenThreads::Mutex _mutex;
};

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf inverseProjectionMatrix;
    osg::Vec4f gbufferNearFar;
    osg::Vec4f forwardOpaqueNearFar;
    osg::Vec4f opaqueTotalNearFar;
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
        gViewData.viewMatrix = renderInfo.getCurrentCamera()->getViewMatrix();
        gViewData.inverseViewMatrix = renderInfo.getCurrentCamera()->getInverseViewMatrix();
        gViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gViewData.inverseProjectionMatrix = osg::Matrixf::inverse(gViewData.projectionMatrix);

        float A = gViewData.projectionMatrix(2, 2), B = gViewData.projectionMatrix(3, 2);
        float zNear = B / (A - 1.0), zFar = B / (A + 1.0);
        gViewData.gbufferNearFar = osg::Vec4f(zNear, zFar, A, B);
        gViewData.opaqueTotalNearFar = osg::Vec4f(zNear, zFar, A, B);

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
        gViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gViewData.inverseProjectionMatrix = osg::Matrixf::inverse(gViewData.projectionMatrix);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

class ForwardOpaquePassPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        gViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gViewData.inverseProjectionMatrix = osg::Matrixf::inverse(gViewData.projectionMatrix);

        float A = gViewData.projectionMatrix(2, 2), B = gViewData.projectionMatrix(3, 2);
        float zNear = B / (A - 1.0f), zFar = B / (A + 1.0f);
        gViewData.forwardOpaqueNearFar = osg::Vec4f(zNear, zFar, A, B);
        zNear = std::min(zNear, gViewData.opaqueTotalNearFar.x()), zFar = std::max(zFar, gViewData.opaqueTotalNearFar.y());
        A = (zNear + zFar) / (zNear - zFar), B = 2.0f * zNear * zFar / (zNear - zFar);
        gViewData.opaqueTotalNearFar = osg::Vec4f(zNear, zFar, A, B);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

class ForwardTransparentPassPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        gViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gViewData.inverseProjectionMatrix = osg::Matrixf::inverse(gViewData.projectionMatrix);

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
    vec4 uForwardOpaqueNearFar;
    vec4 uOpaqueTotalNearFar;
};

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f_fragPos = viewSpace.xyz;
    v2f_normal = osg_NormalMatrix * osg_Normal;
    v2f_color = osg_Color;
    v2f_gndcz = -uOpaqueTotalNearFar.z - uOpaqueTotalNearFar.w / viewSpace.z;
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
    fragData = vec4(v2f_color.rgb, 0.5);
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

    osg::Node* forwardOpaqueNode = osgDB::readNodeFile(TEMP_DIR "fwd_opaque.obj");
    forwardOpaqueNode->getOrCreateStateSet()->setAttribute(colorProgram, osg::StateAttribute::ON);
    forwardOpaqueNode->setNodeMask(FORWARD_OPAQUE_MASK);
    rootGroup->addChild(forwardOpaqueNode);

    osg::Program* transColorProgram = new osg::Program;
    transColorProgram->addShader(new osg::Shader(osg::Shader::VERTEX, trans_vs));
    transColorProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, trans_fs));

    osg::Node* transNode = osgDB::readNodeFile(TEMP_DIR "trans.obj");
    transNode->getOrCreateStateSet()->setAttribute(transColorProgram, osg::StateAttribute::ON);
    transNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    transNode->setNodeMask(TRANS_MASK);
    rootGroup->addChild(transNode);

    osg::Node* forwardTransNode = osgDB::readNodeFile(TEMP_DIR "fwd_trans.obj");
    forwardTransNode->getOrCreateStateSet()->setAttribute(transColorProgram, osg::StateAttribute::ON);
    forwardTransNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    forwardTransNode->setNodeMask(FORWARD_TRANS_MASK);
    rootGroup->addChild(forwardTransNode);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", GBUFFER_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    gbufferPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    gbufferPass->getCamera()->setPreDrawCallback(new GBufferPassPreDrawCallback);

    osg::ref_ptr<xxx::Pipeline::Pass> transparentPass = pipeline->addInputPass("Transparent", TRANS_MASK, 0);
    transparentPass->attach(BufferType::COLOR_BUFFER0, gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
    transparentPass->attach(BufferType::DEPTH_BUFFER, gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER));
    transparentPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    //transparentPass->setMode(GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->getCamera()->setPreDrawCallback(new TransparentPassPreDrawCallback);

    osg::ref_ptr<xxx::Pipeline::Pass> forwardOpaquePass = pipeline->addInputPass("ForwardOpaque", FORWARD_OPAQUE_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    forwardOpaquePass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F, true);
    forwardOpaquePass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, true);
    forwardOpaquePass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardOpaquePass->getCamera()->setPreDrawCallback(new ForwardOpaquePassPreDrawCallback);

    osg::Program* combineColorAndDepthMsProgram = new osg::Program;
    combineColorAndDepthMsProgram->addShader(screenQuadShader);
    combineColorAndDepthMsProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineColorAndDepthMs.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> combineForwardOpaquePass = pipeline->addWorkPass("CombineForwardOpaque", combineColorAndDepthMsProgram, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    combineForwardOpaquePass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F, true);
    combineForwardOpaquePass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, true, osg::Texture::NEAREST, osg::Texture::NEAREST);
    combineForwardOpaquePass->setAttribute(new osg::Depth(osg::Depth::ALWAYS), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    combineForwardOpaquePass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    combineForwardOpaquePass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    combineForwardOpaquePass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 1);
    combineForwardOpaquePass->applyTexture(forwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorMsTexture", 2);
    combineForwardOpaquePass->applyTexture(forwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthMsTexture", 3);

    osg::ref_ptr<xxx::Pipeline::Pass> forwardTransPass = pipeline->addInputPass("ForwardTrans", FORWARD_TRANS_MASK, 0);
    forwardTransPass->attach(BufferType::COLOR_BUFFER0, combineForwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0));
    forwardTransPass->attach(BufferType::DEPTH_BUFFER, combineForwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER));
    forwardTransPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardTransPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    //forwardTransPass->setMode(GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardTransPass->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardTransPass->getCamera()->setPreDrawCallback(new TransparentPassPreDrawCallback);

    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColorMs.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(combineForwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorMsTexture", 0);
    displayPass->applyTexture(combineForwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthMsTexture", 1);
    displayPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

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
