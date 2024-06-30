#include <Core/Render/Pipeline.h>
#include <Core/Base/Entity.h>
#include <Core/Component/MeshRenderer.h>
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>

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

class MaterialUpdateVisitor : public osg::NodeVisitor
{
    osg::ref_ptr<xxx::MaterialAsset> _materialAsset;
public:
    MaterialUpdateVisitor(xxx::MaterialAsset* materialAsset) : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _materialAsset(materialAsset) {}
    ~MaterialUpdateVisitor() = default;

    void apply(osg::Group& node)
    {
        xxx::MeshRenderer* meshRenderer = dynamic_cast<xxx::MeshRenderer*>(&node);
        if (meshRenderer)
        {
            if (meshRenderer->getSubmeshCount())
        }
        traverse(node);
    }
};

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
