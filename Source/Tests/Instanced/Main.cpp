#include <Engine/Render/Pipeline.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/TextureBuffer>

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

static osg::Vec3 positions[] = {
    osg::Vec3(-1, -1, -1),
    osg::Vec3( 1, -1, -1),
    osg::Vec3( 1,  1, -1),
    osg::Vec3(-1,  1, -1),
    osg::Vec3(-1, -1,  1),
    osg::Vec3( 1, -1,  1),
    osg::Vec3( 1,  1,  1),
    osg::Vec3(-1,  1,  1)
};

static uint16_t indices[] = {
    2, 1, 0,
    2, 0, 3,
    4, 5, 6,
    6, 7, 4,
    0, 1, 5,
    5, 4, 0,
    1, 2, 6,
    6, 5, 1,
    2, 3, 7,
    7, 6, 2,
    3, 0, 4,
    4, 7, 3
};

static const char* vs = R"(
#version 430 core
layout(location = 0) in vec3 iPosition;

out vec3 v2f_Normal;

uniform samplerBuffer uInstancedData;
uniform mat4 osg_ModelViewProjectionMatrix;

void main()
{
    vec4 m0 = texelFetch(uInstancedData, gl_InstanceID * 4);
    vec4 m1 = texelFetch(uInstancedData, gl_InstanceID * 4 + 1);
    vec4 m2 = texelFetch(uInstancedData, gl_InstanceID * 4 + 2);
    vec4 m3 = texelFetch(uInstancedData, gl_InstanceID * 4 + 3);
    mat4 instancedModelMatrix = mat4(m0, m1, m2, m3);
    gl_Position = osg_ModelViewProjectionMatrix * instancedModelMatrix * vec4(iPosition, 1.0);
    v2f_Normal = normalize(iPosition);
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

osg::ref_ptr<osg::Geode> createInstancedGeode()
{
    size_t instanceCount = 1000;
    osg::ref_ptr<osg::Image> instancedDataImage = new osg::Image;
    instancedDataImage->allocateImage(instanceCount * 4, 1, 1, GL_RGBA, GL_FLOAT);
    osg::Matrixf* instancedData = reinterpret_cast<osg::Matrixf*>(instancedDataImage->data());
    for (size_t i = 0; i < instanceCount; ++i)
    {
        instancedData[i] = osg::Matrixf::identity();
        size_t r = i / 100;
        size_t s = (i - r * 100) / 10;
        size_t t = i - r * 100 - s * 10;
        instancedData[i].setTrans(osg::Vec3(r * 5, s * 5, t * 5));
    }

    osg::ref_ptr<osg::TextureBuffer> tbo = new osg::TextureBuffer;
    tbo->setImage(instancedDataImage);
    tbo->setInternalFormat(GL_RGBA32F);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> positionArray = new osg::Vec3Array(std::begin(positions), std::end(positions));
    osg::ref_ptr<osg::DrawElementsUShort> indexArray = new osg::DrawElementsUShort(GL_TRIANGLES, std::begin(indices), std::end(indices));
    indexArray->setNumInstances(instanceCount);
    geom->setVertexArray(positionArray);
    geom->addPrimitiveSet(indexArray);
    osg::ref_ptr<osg::StateSet> ss = geom->getOrCreateStateSet();

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
    ss->setAttributeAndModes(program);
    ss->setTextureAttributeAndModes(0, tbo);
    ss->addUniform(new osg::Uniform("uInstancedData", 0));

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom);

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

    rootGroup->addChild(createInstancedGeode());

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    gbufferPass->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* displayProgram = new osg::Program;
    displayProgram->addShader(screenQuadShader);
    displayProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", displayProgram);
    displayPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setRealizeOperation(new EnableGLDebugOperation);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
