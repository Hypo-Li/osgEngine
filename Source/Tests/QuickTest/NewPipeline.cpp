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
#include <osgEarth/MapNode>
#include <osgEarth/GDAL>
#include <osgEarth/TMS>
#include <osgEarth/GeoTransform>
#include <osgEarth/EarthManipulator>
#include <osgEarth/AutoClipPlaneHandler>
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
    osg::Vec4f earthNearFar;
    osg::Vec4f gbufferNearFar;
    osg::Vec4f forwardOpaqueNearFar;
    osg::Vec4f totalNearFar;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

#define EARTH_MASK          0x00000001
#define GBUFFER_MASK        0x00000002
#define TRANS_MASK          0x00000004
#define FORWARD_OPAQUE_MASK 0x00000008
#define FORWARD_TRANS_MASK  0x00000010

/**
*   [ ?  0  0  0 ]
*   [ 0  ?  0  0 ]
*   [ 0  0  A  B ]
*   [ 0  0 -1  0 ]
* N = B / (A - 1.0);
* F = B / (A + 1.0);
* A = (N + F) / (N - F);
* B = 2NF / (N - F);
*/

class EarthPassPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        osg::Matrixd& projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        float A = projectionMatrix(2, 2), B = projectionMatrix(3, 2);
        float zNear = B / (A - 1.0), zFar = B / (A + 1.0);
        gViewData.earthNearFar = osg::Vec4f(zNear, zFar, A, B);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

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
        gViewData.totalNearFar = osg::Vec4f(zNear, zFar, A, B);

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
        zNear = std::min(zNear, gViewData.totalNearFar.x()), zFar = std::max(zFar, gViewData.totalNearFar.y());
        A = (zNear + zFar) / (zNear - zFar), B = 2.0f * zNear * zFar / (zNear - zFar);
        gViewData.totalNearFar = osg::Vec4f(zNear, zFar, A, B);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

class ForwardTransparentPassPreDrawCallback : public osg::Camera::DrawCallback
{
    osg::ref_ptr<osg::FrameBufferObject> mReadFBO;
    osg::ref_ptr<osg::FrameBufferObject> mDrawFBO;
public:
    ForwardTransparentPassPreDrawCallback(osg::FrameBufferObject* readFBO, osg::FrameBufferObject* drawFBO) : mReadFBO(readFBO), mDrawFBO(drawFBO)
    {

    }

    virtual void operator () (osg::RenderInfo& renderInfo) const
    {
        using BufferType = osg::FrameBufferObject::BufferComponent;
        const osg::Texture* tex = mReadFBO->getAttachment(BufferType::COLOR_BUFFER0).getTexture();
        static int prevWidth = tex->getTextureWidth(), prevHeight = tex->getTextureHeight();
        int currWidth = tex->getTextureWidth(), currHeight = tex->getTextureHeight();
        if (prevWidth != currWidth || prevHeight != currHeight)
        {
            prevWidth = currWidth, prevHeight = currHeight;

            osg::ref_ptr<osg::Texture> readColorTexture = const_cast<osg::Texture*>(mReadFBO->getAttachment(BufferType::COLOR_BUFFER0).getTexture());
            osg::ref_ptr<osg::Texture> readDepthTexture = const_cast<osg::Texture*>(mReadFBO->getAttachment(BufferType::DEPTH_BUFFER).getTexture());
            mReadFBO->setAttachment(BufferType::COLOR_BUFFER0, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(readColorTexture.get())));
            mReadFBO->setAttachment(BufferType::DEPTH_BUFFER, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(readDepthTexture.get())));

            osg::ref_ptr<osg::Texture> drawColorTexture = const_cast<osg::Texture*>(mDrawFBO->getAttachment(BufferType::COLOR_BUFFER0).getTexture());
            osg::ref_ptr<osg::Texture> drawDepthTexture = const_cast<osg::Texture*>(mDrawFBO->getAttachment(BufferType::DEPTH_BUFFER).getTexture());
            mDrawFBO->setAttachment(BufferType::COLOR_BUFFER0, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(drawColorTexture.get())));
            mDrawFBO->setAttachment(BufferType::DEPTH_BUFFER, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(drawDepthTexture.get())));
        }
        gViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gViewData.inverseProjectionMatrix = osg::Matrixf::inverse(gViewData.projectionMatrix);

        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();

        mReadFBO->apply(*renderInfo.getState(), osg::FrameBufferObject::READ_FRAMEBUFFER);
        mDrawFBO->apply(*renderInfo.getState(), osg::FrameBufferObject::DRAW_FRAMEBUFFER);
        int w = tex->getTextureWidth(), h = tex->getTextureHeight();
        osg::GLExtensions* ext = renderInfo.getState()->get<osg::GLExtensions>();
        ext->glBlitFramebuffer(
            0, 0, static_cast<GLint>(w), static_cast<GLint>(h),
            0, 0, static_cast<GLint>(w), static_cast<GLint>(h),
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );

    }
};

class NearFarClampCallback : public osg::Camera::ClampProjectionMatrixCallback
{
public:
    virtual bool clampProjectionMatrixImplementation(osg::Matrixf& projection, double& znear, double& zfar) const
    {
        if (znear < zfar)
        {
            double near = std::max(znear, 0.1);
            double far = std::min(zfar, near + 5000.0);
            projection(2, 2) = (near + far) / (near - far);
            projection(3, 2) = 2.0f * near * far / (near - far);

            // double near = 0.1;
            // double far = 5000.0;
            // projection(2, 2) = (near + far) / (near - far);
            // projection(3, 2) = 2.0f * near * far / (near - far);

            return true;
        }
        return false;
    }
    virtual bool clampProjectionMatrixImplementation(osg::Matrixd& projection, double& znear, double& zfar) const
    {
        if (znear < zfar)
        {
            double near = std::max(znear, 0.1);
            double far = std::min(zfar, near + 5000.0);
            projection(2, 2) = (near + far) / (near - far);
            projection(3, 2) = 2.0f * near * far / (near - far);

            // double near = 0.1;
            // double far = 5000.0;
            // projection(2, 2) = (near + far) / (near - far);
            // projection(3, 2) = 2.0f * near * far / (near - far);

            return true;
        }
        return false;
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
    vec4 uEarthNearFar;
    vec4 uGBufferNearFar;
    vec4 uForwardOpaqueNearFar;
    vec4 uTotalNearFar;
};

void main()
{
    vec4 viewSpace = osg_ModelViewMatrix * osg_Vertex;
    gl_Position = osg_ProjectionMatrix * viewSpace;
    v2f_fragPos = viewSpace.xyz;
    v2f_normal = osg_NormalMatrix * osg_Normal;
    v2f_color = osg_Color;
    v2f_gndcz = -uTotalNearFar.z - uTotalNearFar.w / viewSpace.z;
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

int main(int argc, char** argv)
{
    osgEarth::initialize();
    osg::ArgumentParser arguments(&argc, argv);
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
    camera->setProjectionMatrixAsPerspective(60.0, double(width) / double(height), 0.1, 400.0);
    osg::ref_ptr<osg::Group> rootGroup = new osg::Group;
    viewer->setSceneData(rootGroup);

    osgEarth::Map* map = new osgEarth::Map;
    osgEarth::TMSImageLayer* tmsImagery = new osgEarth::TMSImageLayer;
    tmsImagery->setURL(R"(C:\Users\admin\Downloads\wenhualou\tms.xml)");
    map->addLayer(tmsImagery);
    osgEarth::MapNode* mapNode = new osgEarth::MapNode(map);
    mapNode->getTerrainOptions().setMinTileRangeFactor(3.0);
    mapNode->setNodeMask(EARTH_MASK);
    rootGroup->addChild(mapNode);

    osg::Program* colorProgram = new osg::Program;
    colorProgram->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    colorProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));

    osgEarth::GeoTransform* geoTransform = new osgEarth::GeoTransform;
    geoTransform->setPosition(osgEarth::GeoPoint(mapNode->getMap()->getSRS(), 116.33887703, 40.01817574, 10));
    rootGroup->addChild(geoTransform);
    osg::MatrixTransform* transform = new osg::MatrixTransform;
    transform->setMatrix(osg::Matrixd::scale(osg::Vec3d(10, 10, 10)));
    geoTransform->addChild(transform);

    osg::Node* gbufferNode = osgDB::readNodeFile(TEMP_DIR "gbuffer2.obj");
    gbufferNode->getOrCreateStateSet()->setAttribute(colorProgram, osg::StateAttribute::ON);
    gbufferNode->setNodeMask(GBUFFER_MASK);
    transform->addChild(gbufferNode);

    osg::Program* transProgram = new osg::Program;
    transProgram->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/Transparent.vert.glsl"));
    transProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/Transparent.frag.glsl"));

    osg::Node* transNode = osgDB::readNodeFile(TEMP_DIR "trans.obj");
    transNode->getOrCreateStateSet()->setAttribute(transProgram, osg::StateAttribute::ON);
    transNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    transNode->setNodeMask(TRANS_MASK);
    transform->addChild(transNode);

    osg::Node* forwardOpaqueNode = osgDB::readNodeFile(TEMP_DIR "fwd_opaque.obj");
    forwardOpaqueNode->getOrCreateStateSet()->setAttribute(colorProgram, osg::StateAttribute::ON);
    forwardOpaqueNode->setNodeMask(FORWARD_OPAQUE_MASK);
    transform->addChild(forwardOpaqueNode);

    osg::Node* forwardTransNode = osgDB::readNodeFile(TEMP_DIR "fwd_trans.obj");
    forwardTransNode->getOrCreateStateSet()->setAttribute(transProgram, osg::StateAttribute::ON);
    forwardTransNode->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    forwardTransNode->setNodeMask(FORWARD_TRANS_MASK);
    transform->addChild(forwardTransNode);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<xxx::Pipeline::Pass> earthPass = pipeline->addInputPass("Earth", EARTH_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    earthPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    earthPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    earthPass->getCamera()->setPreDrawCallback(new EarthPassPreDrawCallback);
    earthPass->getCamera()->setCullCallback(new osgEarth::AutoClipPlaneCullCallback(mapNode));

    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", GBUFFER_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    gbufferPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    gbufferPass->getCamera()->setPreDrawCallback(new GBufferPassPreDrawCallback);
    gbufferPass->getCamera()->setClampProjectionMatrixCallback(new NearFarClampCallback);

    // Shadow Cast
    // Shadow Mask
    // SSAO
    // Lighting
    
    // CombineEarthAndGBuffer
    osg::Program* combineEarthAndGBufferProgram = new osg::Program;
    combineEarthAndGBufferProgram->addShader(screenQuadShader);
    combineEarthAndGBufferProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineEarthAndGBuffer2.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> combineEarthAndGBufferPass = pipeline->addWorkPass("CombineEarthAndGBufferColor", combineEarthAndGBufferProgram, 0);
    combineEarthAndGBufferPass->attach(BufferType::COLOR_BUFFER0, earthPass->getBufferTexture(BufferType::COLOR_BUFFER0));
    combineEarthAndGBufferPass->attach(BufferType::DEPTH_BUFFER, earthPass->getBufferTexture(BufferType::DEPTH_BUFFER));
    combineEarthAndGBufferPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    combineEarthAndGBufferPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uGBufferColorTexture", 0);
    combineEarthAndGBufferPass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uGBufferDepthTexture", 1);
    combineEarthAndGBufferPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // Atmosphere
    // Volumetric Cloud

    osg::ref_ptr<xxx::Pipeline::Pass> transparentPass = pipeline->addInputPass("Transparent", TRANS_MASK, 0);
    transparentPass->attach(BufferType::COLOR_BUFFER0, combineEarthAndGBufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
    transparentPass->attach(BufferType::DEPTH_BUFFER, gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER));
    transparentPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    //transparentPass->setMode(GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    transparentPass->getCamera()->setPreDrawCallback(new TransparentPassPreDrawCallback);
    transparentPass->applyTexture(earthPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uEarthDepthTexture", 15);
    transparentPass->getCamera()->setClampProjectionMatrixCallback(new NearFarClampCallback);

    // TAA
    // Bloom
    // ColorGrading

    osg::ref_ptr<xxx::Pipeline::Pass> forwardOpaquePass = pipeline->addInputPass("ForwardOpaque", FORWARD_OPAQUE_MASK, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    forwardOpaquePass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F, true);
    forwardOpaquePass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, true);
    forwardOpaquePass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardOpaquePass->getCamera()->setPreDrawCallback(new ForwardOpaquePassPreDrawCallback);
    forwardOpaquePass->getCamera()->setClampProjectionMatrixCallback(new NearFarClampCallback);

    osg::Program* combineColorAndDepthMsProgram = new osg::Program;
    combineColorAndDepthMsProgram->addShader(screenQuadShader);
    combineColorAndDepthMsProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineColorAndDepthMs2.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> combineForwardOpaquePass = pipeline->addWorkPass("CombineForwardOpaque", combineColorAndDepthMsProgram, GL_DEPTH_BUFFER_BIT);
    combineForwardOpaquePass->attach(BufferType::COLOR_BUFFER0, combineEarthAndGBufferPass->getBufferTexture(BufferType::COLOR_BUFFER0));
    combineForwardOpaquePass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    combineForwardOpaquePass->setAttribute(new osg::Depth(osg::Depth::ALWAYS), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    combineForwardOpaquePass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    combineForwardOpaquePass->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    combineForwardOpaquePass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 0);
    combineForwardOpaquePass->applyTexture(forwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorMsTexture", 1);
    combineForwardOpaquePass->applyTexture(forwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthMsTexture", 2);
    combineForwardOpaquePass->applyTexture(earthPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uEarthDepthTexture", 3);

    // 在合并ForwardOpaque时直接使用multisample texture附加到fbo会导致时间开销为原来的4倍(4xMSAA), 为了避免这种情况, 这里使用一次拷贝
    // 可以用glBlitFramebuffer替代
    /*osg::Program* copyColorAndDepthProgram = new osg::Program;
    copyColorAndDepthProgram->addShader(screenQuadShader);
    copyColorAndDepthProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColorAndDepth.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> copyColorAndDepthToMsPass = pipeline->addWorkPass("CopyColorAndDepthToMs", copyColorAndDepthProgram, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    copyColorAndDepthToMsPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    copyColorAndDepthToMsPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F, true);
    copyColorAndDepthToMsPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT32, true, osg::Texture::NEAREST, osg::Texture::NEAREST);
    copyColorAndDepthToMsPass->applyTexture(combineForwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    copyColorAndDepthToMsPass->applyTexture(combineForwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 1);*/

    osg::ref_ptr<osg::FrameBufferObject> readFbo = new osg::FrameBufferObject;
    readFbo->setAttachment(BufferType::COLOR_BUFFER0, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(combineForwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0))));
    readFbo->setAttachment(BufferType::DEPTH_BUFFER, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(combineForwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER))));

    osg::ref_ptr<xxx::Pipeline::Pass> forwardTransPass = pipeline->addInputPass("ForwardTrans", FORWARD_TRANS_MASK, 0);
    forwardTransPass->attach(BufferType::COLOR_BUFFER0, forwardOpaquePass->getBufferTexture(BufferType::COLOR_BUFFER0));
    forwardTransPass->attach(BufferType::DEPTH_BUFFER, forwardOpaquePass->getBufferTexture(BufferType::DEPTH_BUFFER));

    osg::ref_ptr<osg::FrameBufferObject> drawFbo = new osg::FrameBufferObject;
    drawFbo->setAttachment(BufferType::COLOR_BUFFER0, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(forwardTransPass->getBufferTexture(BufferType::COLOR_BUFFER0))));
    drawFbo->setAttachment(BufferType::DEPTH_BUFFER, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2DMultisample*>(forwardTransPass->getBufferTexture(BufferType::DEPTH_BUFFER))));

    forwardTransPass->setAttribute(gViewDataUBB, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardTransPass->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    //forwardTransPass->setMode(GL_CULL_FACE, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardTransPass->setMode(GL_BLEND, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    forwardTransPass->getCamera()->setPreDrawCallback(new ForwardTransparentPassPreDrawCallback(readFbo, drawFbo));
    forwardTransPass->applyTexture(earthPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uEarthDepthTexture", 15);
    forwardTransPass->getCamera()->setClampProjectionMatrixCallback(new NearFarClampCallback);

    // UI

    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColorMs.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(forwardTransPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorMsTexture", 0);
    displayPass->applyTexture(forwardTransPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthMsTexture", 1);
    displayPass->setAttribute(new osg::Depth(osg::Depth::ALWAYS), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setCameraManipulator(new osgEarth::EarthManipulator);
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
