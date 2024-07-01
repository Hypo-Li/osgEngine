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

class ComputeDrawCallback : public osg::Drawable::DrawCallback
{
    osg::ref_ptr<osg::Texture> _texture;
public:
    ComputeDrawCallback(osg::Texture* texture) : _texture(texture) {}

    virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
    {
        drawable->drawImplementation(renderInfo);
        renderInfo.getState()->get<osg::GLExtensions>()->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        _texture->apply(*renderInfo.getState());
        renderInfo.getState()->get<osg::GLExtensions>()->glGenerateMipmap(_texture->getTextureTarget());
        std::cout << _texture->getNumImages() << std::endl;
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->readImageFromCurrentTexture(renderInfo.getContextID(), true, _texture->getSourceType());
        _texture->setImage(0, image);
        xxx::TextureAsset* textureAsset = new xxx::TextureAsset;
        textureAsset->setTexture(_texture);
        xxx::AssetManager::storeAsset(TEMP_DIR "Texture.xast", textureAsset);
        return;
    }
};

static const char* gSource = R"(
void getMaterialParameters(in MaterialInputParameters inParam, out MaterialOutputParameters outParam)
{
    outParam.baseColor = uBaseColor;
    outParam.metallic = uMetallic;
    outParam.roughness = uRoughness;
    outParam.normal = uNormal;
    outParam.emissive = uEmissive;
    outParam.occlusion = 1.0f;
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
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(90.0, double(width) / double(height), 0.1, 400.0);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    osg::ref_ptr<xxx::Entity> entity = new xxx::Entity("test");
    entity->appendComponent(new xxx::MeshRenderer);

    TestVisitor tv;
    entity->accept(tv);

    /*osg::ref_ptr<osg::Program> computeProgram = new osg::Program;
    computeProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Test/ComputeImage.comp.glsl"));
    osg::ref_ptr<osg::Texture2D> computeTexture = new osg::Texture2D;
    computeTexture->setInternalFormat(GL_RGBA16F);
    computeTexture->setSourceFormat(GL_RGBA);
    computeTexture->setSourceType(GL_HALF_FLOAT);
    computeTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    computeTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    computeTexture->setTextureWidth(32);
    computeTexture->setTextureHeight(32);
    computeTexture->setNumMipmapLevels(5);
    osg::ref_ptr<osg::BindImageTexture> computeImage = new osg::BindImageTexture(0, computeTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F_ARB);
    osg::ref_ptr<osg::DispatchCompute> computeDispatch = new osg::DispatchCompute(1, 1, 1);
    computeDispatch->getOrCreateStateSet()->setAttributeAndModes(computeProgram);
    computeDispatch->getOrCreateStateSet()->setAttributeAndModes(computeImage);
    computeDispatch->setDrawCallback(new ComputeDrawCallback(computeTexture));
    rootGroup->addChild(computeDispatch);*/

    xxx::MaterialAsset* materialAsset = new xxx::MaterialAsset;
    materialAsset->appendParameter("BaseColor", osg::Vec3(0.8, 0.8, 0.8));
    materialAsset->appendParameter("Metallic", 0.0f);
    materialAsset->appendParameter("Roughness", 0.5f);
    materialAsset->appendParameter("Emissive", osg::Vec3(0.0, 0.0, 0.0));
    materialAsset->appendParameter("Normal", osg::Vec3(0.5, 0.5, 1.0));
    materialAsset->setSource(gSource);
    xxx::AssetManager::storeAsset(TEMP_DIR "Material.xast", materialAsset);

    //xxx::MaterialAsset* materialAsset = dynamic_cast<xxx::MaterialAsset*>(xxx::AssetManager::loadAsset(TEMP_DIR "Material.xast"));

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);

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
