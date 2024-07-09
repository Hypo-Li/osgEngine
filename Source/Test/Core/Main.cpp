#include <Core/Render/Pipeline.h>
#include <Core/Base/Entity.h>
#include <Core/Base/Context.h>
#include <Core/Component/MeshRenderer.h>
#include <Core/Asset/AssetManager.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/BindImageTexture>
#include <osg/DispatchCompute>
#include <osgDB/WriteFile>
#include "GLTFLoader.h"
#include "TestEventHandler.h"

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

static const char* gSource = R"(
void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    vec4 tColor0 = decodeColor(texture(uTexture0, mi.texcoord0));
    vec4 tColor1 = texture(uTexture1, mi.texcoord0);
    vec3 local0 = mix(0.5, 1.0, tColor1.r) * mix(uColor3.rgb, uColor2.rgb, vec3(tColor1.b));
    mo.baseColor = tColor0.rgb * local0;

    vec4 tColor2 = texture(uTexture2, mi.texcoord0 * 0.05);
    float local1 = mix(-20.0, 20.0, tColor2.r);
    float local2 = clamp((local1 + (-mi.fragPosVS.z) - 10.0) / 10.0, 0.0, 1.0);
    float local3 = mix(tColor0.a, 0.5, local2);
    mo.roughness = mix(0.5, 0.2, local3);

    mo.normal = decodeNormal(texture(uTexture3, mi.texcoord0).rgb);
}
)";

int main()
{
    xxx::Context::get();

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
    xxx::Context::get().setSceneRoot(rootGroup);
    //viewer->setRealizeOperation(new EnableGLDebugOperation);
    //viewer->setRealizeOperation(new xxx::ImGuiInitOperation);
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(60.0, double(width) / double(height), 0.1, 400.0);

    /*osg::Image* image = osgDB::readImageFile(TEMP_DIR "T_Rock_Marble_Polished_D.PNG");
    osg::Texture2D* texture = new osg::Texture2D(image);
    texture->setInternalFormat(GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    texture->setMaxAnisotropy(16.0f);
    xxx::TextureAsset* textureAsset = new xxx::TextureAsset;
    textureAsset->setTexture(texture);
    xxx::AssetManager::storeAsset("Texture/T_Rock_Marble_Polished_D.xast", textureAsset);*/

    /*xxx::MaterialTemplateAsset* materialTemplateAsset = new xxx::MaterialTemplateAsset;
    materialTemplateAsset->appendParameter("Texture0", xxx::AssetManager::loadAsset<xxx::TextureAsset>("Texture/T_Rock_Marble_Polished_D"));
    materialTemplateAsset->appendParameter("Texture1", xxx::AssetManager::loadAsset<xxx::TextureAsset>("Texture/T_Ceramic_Tile_M"));
    materialTemplateAsset->appendParameter("Texture2", xxx::AssetManager::loadAsset<xxx::TextureAsset>("Texture/T_Perlin_Noise_M"));
    materialTemplateAsset->appendParameter("Texture3", xxx::AssetManager::loadAsset<xxx::TextureAsset>("Texture/T_Ceramic_Tile_N"));
    materialTemplateAsset->appendParameter("Color2", osg::Vec3(0.41, 0.41, 0.41));
    materialTemplateAsset->appendParameter("Color3", osg::Vec3(0.825, 0.81, 0.788));
    materialTemplateAsset->setSource(gSource);
    materialTemplateAsset->apply();
    xxx::AssetManager::storeAsset("Material/TestMaterialTemplate", materialTemplateAsset);*/

    /*xxx::MaterialInstanceAsset* materialInstanceAsset = new xxx::MaterialInstanceAsset;
    materialInstanceAsset->setMaterialTemplate(materialTemplateAsset);
    materialInstanceAsset->setParameter("BaseColorTexture", textureAsset2);
    materialInstanceAsset->setParameter("Metallic", osg::Vec3(1.0, 1.0, 1.0));
    materialInstanceAsset->setParameter("Roughness", 0.8f);
    xxx::AssetManager::storeAsset("Material/TestMaterialInstance.xast", materialInstanceAsset);*/

    /*std::vector<osg::ref_ptr<xxx::MeshAsset>> meshes = xxx::GLTFLoader::load(TEMP_DIR "Test.glb");
    meshes[0]->setPreviewMaterial(0, xxx::AssetManager::loadAsset<xxx::MaterialAsset>("Material/TestMaterialTemplate"));
    xxx::AssetManager::storeAsset("Mesh/Test", meshes[0]);*/

    //xxx::Entity* entity = new xxx::Entity("TestEntity");
    //xxx::MeshRenderer* meshRenderer = new xxx::MeshRenderer;
    ////meshRenderer->setMesh(meshes[0]);
    //meshRenderer->setMesh(xxx::AssetManager::loadAsset<xxx::MeshAsset>("Mesh/Test"));
    //entity->appendComponent(meshRenderer);
    //rootGroup->addChild(entity);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGB10_A2);
    gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);
    gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);

    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* finalProgram = new osg::Program;
    finalProgram->addShader(screenQuadShader);
    finalProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", finalProgram);
    finalPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER2), "uColorTexture", 0);

    //xxx::ImGuiHandler* imguiHandler = new xxx::ImGuiHandler(viewer, finalPass->getCamera(), dynamic_cast<osg::Texture2D*>(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER2)), pipeline);
    //viewer->addEventHandler(imguiHandler);
    //viewer->addEventHandler(new xxx::TestEventHandler(gbufferPass->getCamera(), imguiHandler));
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
