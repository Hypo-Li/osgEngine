#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Core/Asset.h"
#include "Engine/Core/Prefab.h"
#include "Engine/Core/AssetManager.h"
#include "Engine/Render/Pipeline.h"
#include "Engine/Component/MeshRenderer.h"

#include "DebugCallback.h"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

#include <iostream>

using namespace xxx;
using namespace xxx::refl;

int main()
{
    AssetManager& am = AssetManager::get();

    
    /*Texture2D* texture2d = new Texture2D(TEMP_DIR "T_Perlin_Noise_M.PNG");
    Asset* textureAsset = AssetManager::get().createAsset(texture2d, "Engine/Texture/T_Perlin_Noise_M.xast");
    textureAsset->save();*/
    

    ///*
    Mesh* mesh = new Mesh(TEMP_DIR "test.obj");

    Asset* textureAsset = am.getAsset("Engine/Texture/AwesomeFace.xast");
    textureAsset->load();
    Texture* texture = textureAsset->getRootObject<Texture>();

    Shader* shader = new Shader;
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
    mo.emissive = (mo.normal * 0.5 + 0.5) * uEmissiveFactor;
    mo.opaque = texColor.a;
    mo.baseColor = texColor.rgb * uBaseColor;
    mo.metallic = uMetallic;
    mo.roughness = uRoughness;
    mat3 tbn = mat3(mi.tangentWS, mi.bitangentWS, mi.normalWS);
    mo.normal = tbn * vec3(0, 0, 1);
    mo.occlusion = 1.0;
}
)"
    );

    Material* material = new Material;
    material->setShader(shader);
    material->setAlphaMode(AlphaMode::Mask);
    material->setDoubleSided(true);
    material->setParameter("BaseColor", osg::Vec3f(1.0, 0.5, 0.0));
    material->enableParameter("BaseColor", true);

    shader->addParameter("EmissiveFactor", osg::Vec3(1, 0.5, 0));
    material->syncWithShader();

    mesh->setDefaultMaterial(0, material);

    Entity* entity = new Entity("TestEntity");
    MeshRenderer* meshRenderer = entity->addComponent<MeshRenderer>();
    meshRenderer->setMesh(mesh);

    Asset* entityAsset = am.createAsset(entity, "Engine/TestEntity.xast");
    entityAsset->save();
    //*/

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

    Asset* asset = am.getAsset("Engine/TestEntity.xast");
    asset->load();
    
    rootGroup->addChild(dynamic_cast<Entity*>(asset->getRootObject())->getOsgNode());

    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline::Pass> gbufferPass = pipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    gbufferPass->attach(BufferType::COLOR_BUFFER1, GL_RGBA16F);
    gbufferPass->attach(BufferType::COLOR_BUFFER2, GL_RGBA16F);
    gbufferPass->attach(BufferType::COLOR_BUFFER3, GL_RGBA16F);
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
