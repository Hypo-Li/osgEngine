#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Core/Asset.h"
#include "Engine/Core/Prefab.h"
#include "Engine/Core/AssetManager.h"
#include "Engine/Render/Pipeline.h"

#include "DebugCallback.h"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

#include <iostream>

using namespace xxx;
using namespace xxx::refl;

class TestComponent : public Component
{
    REFLECT_CLASS(TestComponent)
public:
    TestComponent() :
        mOsgImplGroup(new osg::Group)
    {
        mOsgComponentGroup->addChild(mOsgImplGroup);
    }

    TestComponent(const TestComponent& other) :
        mOsgImplGroup(new osg::Group(*other.mOsgImplGroup))
    {
        mOsgComponentGroup->addChild(mOsgImplGroup);
    }

    virtual Type getType() const override
    {
        return Type::MeshRenderer;
    }

    void setShader(Shader* shader)
    {
        mShader = shader;
    }

protected:
    osg::ref_ptr<Shader> mShader;

    osg::ref_ptr<osg::Group> mOsgImplGroup;
};

namespace xxx::refl
{
    template <> Type* Reflection::createType<TestComponent>()
    {
        Class* clazz = new ClassInstance<TestComponent, Component>("TestComponent");
        clazz->addProperty("TestShader", &TestComponent::mShader);
        getClassMap().emplace("TestComponent", clazz);
        return clazz;
    }
}

struct TestStruct
{
    int a;
    float b;
    char c;
};

const char* vs = R"(
#version 430 core
in vec4 osg_Vertex;
uniform mat4 osg_ModelViewProjectionMatrix;
void main()
{
    gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;
}
)";

const char* fs = R"(
#version 430 core
out vec4 fragData;
void main()
{
    fragData = vec4(1, 0, 0, 1);
}
)";

osg::Vec3f positions[] = {
    osg::Vec3f(0, 0, 0),
    osg::Vec3f(1, 1, 1),
    osg::Vec3f(1, 2, 3)
};

uint32_t indices[] = { 0, 1, 2 };

osg::Geometry* generateGeometry()
{
    static osg::Vec3Array* va = new osg::Vec3Array(3, positions);
    static osg::DrawElementsUInt* deui = new osg::DrawElementsUInt(GL_TRIANGLES, 3, indices);
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(va);
    geometry->addPrimitiveSet(deui);
    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    osg::Program* program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vs));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fs));
    stateSet->setAttributeAndModes(program);
    return geometry;
}

int main()
{
    /*AssetManager::get();
    Shader* shader = new Shader;
    shader->addParameter("BaseColor", osg::Vec3f(0.8, 0.8, 0.8));
    shader->addParameter("Roughness", 0.5f);
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Specular", 0.5f);
    Class* clazz = shader->getClass();
    Method* setParameterFloatMethod = clazz->getMethod("setParameter<float>");
    setParameterFloatMethod->invoke(shader, std::string("Roughness"), 0.01f);
    {
        Asset* shaderAsset = AssetManager::get().createAsset(shader, ASSET_DIR "Shader.xast");
        shaderAsset->save();
    }

    Entity* entity = new Entity;
    TestComponent* testComponent = new TestComponent;
    testComponent->setShader(shader);
    entity->addComponent(testComponent);
    entity->addChild(new Entity);
    {
        Asset* entityAsset = AssetManager::get().createAsset(entity, ASSET_DIR "Entity.xast");
        entityAsset->save();
    }

    {
        Asset* entityAsset = AssetManager::get().getAsset(ASSET_DIR "Entity.xast");
        entityAsset->load();
        Object* object = entityAsset->getRootObject();
        std::string guid = object->getGuid().toString();
        std::cout << guid << std::endl;
    }

    Texture2D* texture2d = new Texture2D(TEMP_DIR "awesomeface.png");
    Asset* textureAsset = AssetManager::get().createAsset(texture2d, ASSET_DIR "Texture/Awesomeface.xast");
    textureAsset->save();*/

    /*AssetManager& am = AssetManager::get();

    Asset* shaderAsset = am.getAsset(ASSET_DIR "Shader.xast");
    shaderAsset->load();
    Shader* shader = shaderAsset->getRootObject<Shader>();

    Asset* textureAsset = am.getAsset(ASSET_DIR "Texture/Awesomeface.xast");
    textureAsset->load();
    Texture* texture = textureAsset->getRootObject<Texture>();

    shader->addParameter("BaseColorTexture", texture);
    shaderAsset->save();*/

    Mesh* mesh = new Mesh(TEMP_DIR "test.fbx");

    AssetManager& am = AssetManager::get();
    Asset* textureAsset = am.getAsset(ASSET_DIR "Texture/Awesomeface.xast");
    textureAsset->load();
    Texture* texture = textureAsset->getRootObject<Texture>();

    Shader* shader = new Shader;
    shader->setAlphaMode(AlphaMode::Mask);
    shader->setDoubleSided(true);
    shader->addParameter("BaseColor", osg::Vec3f(0.8, 0.8, 0.8));
    shader->addParameter("Roughness", 0.5f);
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Specular", 0.5f);
    shader->addParameter("BaseColorTexture", texture);
    shader->setSource(
R"(
void calcMaterial(in MaterialInputs mi, out MaterialOutputs mo)
{
    vec4 texColor = texture(uBaseColorTexture, mi.texcoord0.xy);
    mo.opaque = texColor.a;
#if (SHADING_MODEL >= STANDARD)
    mo.baseColor = texColor.rgb;
    mo.metallic = uMetallic;
    mo.roughness = uRoughness;
    mat3 tbn = mat3(mi.tangentWS, mi.bitangentWS, mi.normalWS);
    mo.normal = tbn * vec3(0, 0, 1);
    mo.occlusion = 1.0;
#endif
    mo.emissive = mo.normal * 0.5 + 0.5;
}
)"
    );

    Material* material = new Material;
    material->setShader(shader);
    material->setParameter("BaseColor", osg::Vec3f(1.0, 0.5, 0.0));

    /*Asset* asset = AssetManager::get().getAsset(ASSET_DIR "Mesh.xast");
    asset->load();
    Mesh* mesh = asset->getRootObject<Mesh>();*/

    osg::Geode* geode = new osg::Geode;
    
    std::vector<osg::Geometry*> geometries = mesh->generateGeometries();
    for (osg::Geometry* geometry : geometries)
    {
        geode->addDrawable(geometry);
        geometry->setStateSet(material->getOsgStateSet());
    }

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

    rootGroup->addChild(geode);

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGBA8);
    gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA8);
    gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA8);
    gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
    osg::Program* finalProgram = new osg::Program;
    finalProgram->addShader(screenQuadShader);
    finalProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addDisplayPass("Display", finalProgram);
    finalPass->applyTexture(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setRealizeOperation(new EnableGLDebugOperation);
    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }

    return 0;
}
