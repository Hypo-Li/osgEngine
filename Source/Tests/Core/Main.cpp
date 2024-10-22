#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Core/Asset.h"
#include "Engine/Core/Prefab.h"
#include "Engine/Core/AssetManager.h"
#include "Engine/Render/Pipeline.h"
#include "Engine/Component/MeshRenderer.h"
#include "Engine/Component/Light.h"
#include "Engine/Core/Engine.h"
#include "Engine/Render/Texture2D.h"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgGA/TrackballManipulator>

#include <iostream>

using namespace xxx;
using namespace xxx::refl;

static const char testShaderSource[] = R"(
void calcMaterial(in MaterialInputs mi, inout MaterialOutputs mo)
{
    vec4 texColor = texture(uBaseColorTexture, mi.texcoord0);
    mo.emissive = uEmissive;
    mo.baseColor = texColor.rgb * uBaseColorFactor;
    mo.opaque = texColor.a;
    mo.metallic = uMetallic;
    mo.roughness = uRoughness;
}
)";

void createTestAssets()
{
    AssetManager& am = AssetManager::get();

    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TEMP_DIR "awesomeface.png");
    osg::ref_ptr<Texture2D> texture = new Texture2D(image, TextureImportOptions());
    am.createAsset(texture, "Engine/Texture/AwesomeFace")->save();

    osg::ref_ptr<Shader> shader = new Shader;
    shader->addParameter("BaseColorTexture", texture.get());
    shader->addParameter("BaseColorFactor", osg::Vec3f(1, 1, 1));
    shader->addParameter("Emissive", osg::Vec3f(0, 0, 0));
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Roughness", 0.5f);
    shader->setSource(testShaderSource);
    am.createAsset(shader, "Engine/Shader/TestShader")->save();

    osg::ref_ptr<Material> material = new Material;
    material->setShader(shader);
    am.createAsset(material, "Engine/Material/TestMaterial")->save();

    osg::ref_ptr<Mesh> mesh = new Mesh(TEMP_DIR "cube.obj");
    mesh->setDefaultMaterial(0, material);
    am.createAsset(mesh, "Engine/Mesh/Cube")->save();

    osg::ref_ptr<Entity> entity = new Entity("TestEntity");
    MeshRenderer* meshRenderer = entity->addComponent<MeshRenderer>();
    meshRenderer->setMesh(mesh);
    am.createAsset(entity, "Engine/Entity/TestEntity")->save();
}

int main()
{
    AssetManager& am = AssetManager::get();

    EngineSetupConfig engineSetupConfig;
    engineSetupConfig.width = 1024;
    engineSetupConfig.height = 768;
    engineSetupConfig.glContextVersion = "4.6";
    engineSetupConfig.fullScreen = false;
    engineSetupConfig.runMode = RunMode::Edit;
    Engine* engine = new Engine(engineSetupConfig);
    osgViewer::CompositeViewer viewer;
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addView(engine->getView());
    viewer.realize();

    Context::get().getGraphicsContext()->makeCurrent();

    //createTestAssets();

    //TextureCubemap* textureCubemap = new TextureCubemap(TEMP_DIR "zwartkops_straight_sunset_1k.hdr");
    //am.createAsset(textureCubemap, "Engine/Texture/TestCubemap")->save();

    /*osg::Image* image = osgDB::readImageFile(TEMP_DIR "white.png");
    osg::ref_ptr<Texture2D> whiteTexture2D = new Texture2D(image, TextureImportOptions());
    am.createAsset(whiteTexture2D, "Engine/Texture/White")->save();*/

    /*Asset* materialAsset = am.getAsset("Engine/Material/TestMaterial");
    materialAsset->load();
    Material* material = materialAsset->getRootObject<Material>();

    osg::ref_ptr<Mesh> mesh = new Mesh(TEMP_DIR "sphere.obj");
    mesh->setDefaultMaterial(0, material);
    am.createAsset(mesh, "Engine/Mesh/Sphere")->save();*/

    Asset* entityAsset = am.getAsset("Engine/Entity/TestEntity");
    entityAsset->load();
    Entity* entity = entityAsset->getRootObject<Entity>();
    DirectionalLight* directionalLight = entity->addComponent<DirectionalLight>();
    osg::Vec3f lightDirection(1, 1, -1);
    lightDirection.normalize();
    directionalLight->setDirection(lightDirection);
    entityAsset->save();

    Context::get().getGraphicsContext()->releaseContext();

    return 0;
}
