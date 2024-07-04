#include <Core/Render/Pipeline.h>
#include <Core/Base/Entity.h>
#include <Core/Component/MeshRenderer.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osgDB/WriteFile>
#include <Core/Asset/AssetManager.h>

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

class TestVisitor : public osg::NodeVisitor
{
public:
    TestVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    ~TestVisitor() = default;

    void apply(osg::MatrixTransform& node)
    {
        xxx::Entity* entity = dynamic_cast<xxx::Entity*>(&node);
        if (entity)
        {
            std::cout << "get entity" << std::endl;
        }
        traverse(node);
    }

    void apply(osg::Group& node)
    {
        xxx::MeshRenderer* meshRenderer = dynamic_cast<xxx::MeshRenderer*>(&node);
        if (meshRenderer)
        {
            std::cout << "get mesh renderer" << std::endl;
        }
        traverse(node);
    }
};

static const char* gSource = R"(
void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    vec4 tColor = texture(uBaseColorTexture, mi.texcoord0);
    mo.baseColor = tColor.rgb;
    mo.metallic = uMetallic;
    mo.roughness = uRoughness;
    mo.normal = uNormal;
    mo.emissive = uEmissive;
    mo.occlusion = 1.0f;
    mo.opaque = tColor.a;
}
)";

static const char* gSource2 = R"(
void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    mo.baseColor = uBaseColor;
    mo.metallic = uMetallic;
    mo.roughness = uRoughness.x;
    mo.normal = uNormal;
    mo.emissive = uEmissive;
    mo.occlusion = 1.0f;
    mo.opaque = 1.0f;
}
)";

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
    viewer->setRealizeOperation(new EnableGLDebugOperation);
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);

    osg::Image* image = osgDB::readImageFile(TEMP_DIR "container.jpg");
    osg::Texture2D* texture = new osg::Texture2D(image);
    texture->setInternalFormat(GL_RGBA8);
    texture->setMaxAnisotropy(16.0f);
    xxx::TextureAsset* textureAsset = new xxx::TextureAsset;
    textureAsset->setTexture(texture);
    //xxx::AssetManager::storeAsset(TEMP_DIR "Texture.xast", textureAsset);

    osg::Image* image2 = osgDB::readImageFile(TEMP_DIR "awesomeface.png");
    osg::Texture2D* texture2 = new osg::Texture2D(image2);
    texture2->setInternalFormat(GL_RGBA8);
    texture2->setMaxAnisotropy(16.0f);
    xxx::TextureAsset* textureAsset2 = new xxx::TextureAsset;
    textureAsset2->setTexture(texture2);

    //xxx::TextureAsset* textureAsset = xxx::AssetManager::loadAsset<TextureAsset>(TEMP_DIR "Texture.xast");
    xxx::MaterialTemplateAsset* materialTemplateAsset = new xxx::MaterialTemplateAsset;
    materialTemplateAsset->setAlphaMode(xxx::MaterialTemplateAsset::AlphaMode::Alpha_Mask);
    materialTemplateAsset->setDoubleSided(true);
    materialTemplateAsset->appendParameter("BaseColorTexture", textureAsset);
    materialTemplateAsset->appendParameter("Metallic", 0.0f);
    materialTemplateAsset->setParameter("Metallic", osg::Vec3(1.0, 1.0, 1.0));
    materialTemplateAsset->appendParameter("Roughness", 0.5f);
    materialTemplateAsset->setParameter("Roughness", 0.8f);
    materialTemplateAsset->appendParameter("Emissive", osg::Vec3(0.0, 0.0, 0.0));
    materialTemplateAsset->appendParameter("Normal", osg::Vec3(0.5, 0.5, 1.0));
    materialTemplateAsset->setSource(gSource);
    materialTemplateAsset->apply();
    //xxx::AssetManager::storeAsset(TEMP_DIR "Material.xast", materialAsset);

    xxx::MaterialInstanceAsset* materialInstanceAsset = new xxx::MaterialInstanceAsset;
    materialInstanceAsset->setMaterialTemplate(materialTemplateAsset);
    materialInstanceAsset->setParameter("BaseColorTexture", textureAsset2);
    materialInstanceAsset->setParameter("Metallic", osg::Vec3(1.0, 1.0, 1.0));
    materialInstanceAsset->setParameter("Roughness", 0.8f);

    materialTemplateAsset->removeParameter("BaseColorTexture");
    materialTemplateAsset->appendParameter("BaseColor", osg::Vec3(0.8, 0.8, 0.8));
    materialTemplateAsset->removeParameter("Roughness");
    materialTemplateAsset->appendParameter("Roughness", osg::Vec3(0.8, 0.8, 0.8));
    materialTemplateAsset->setSource(gSource2);
    materialTemplateAsset->apply();

    materialInstanceAsset->syncMaterialTemplate();

    //xxx::MaterialAsset* materialAsset = xxx::AssetManager::loadAsset<MaterialAsset>(TEMP_DIR "Material.xast");
    osg::ref_ptr<osg::StateSet> materialStateSet = new osg::StateSet(*materialInstanceAsset->getStateSet());
    osg::ref_ptr<osg::Program> realProgram = new osg::Program;
    realProgram->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Mesh/Mesh.vert.glsl"));
    realProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Mesh/Mesh.frag.glsl"));
    realProgram->addShader(materialInstanceAsset->getShader());
    materialStateSet->setAttribute(realProgram, osg::StateAttribute::ON);

    osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(TEMP_DIR "cube.obj");
    meshNode->setStateSet(materialStateSet);
    rootGroup->addChild(meshNode);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGBA16F);
    gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);
    gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);

    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* finalProgram = new osg::Program;
    finalProgram->addShader(screenQuadShader);
    finalProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", finalProgram);
    finalPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER2), "uColorTexture", 0);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
