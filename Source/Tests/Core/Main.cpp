#include "Engine/Core/OsgReflection.h"
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
#include <Editor/Editor.h>

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

TextureCubemap* generateSpecularCubemap(TextureCubemap* imageCubemap)
{
    TextureCubemap* specularCubemap = new TextureCubemap(new osg::TextureCubeMap);
    specularCubemap->setSize(512);
    specularCubemap->setFormat(Texture::Format::RGBA16F);
    specularCubemap->setPixelFormat(Texture::PixelFormat::RGBA);
    specularCubemap->setPixelType(Texture::PixelType::Half);
    specularCubemap->setMinFilter(Texture::FilterMode::Linear_Mipmap_Linear);
    specularCubemap->setMipmapGeneration(false);
    specularCubemap->setMipmapCount(4);
    specularCubemap->apply();

    osg::State* state = Context::get().getGraphicsContext()->getState();
    osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
    osg::ref_ptr<osg::StateSet> computeStateSet = new osg::StateSet;
    computeStateSet->addUniform(new osg::Uniform("uCubemapTexture", 0));
    computeStateSet->setTextureAttribute(0, imageCubemap->getOsgTexture());
    computeStateSet->setMode(GL_TEXTURE_CUBE_MAP_SEAMLESS, osg::StateAttribute::ON);
    int numGroups[5][3] = {
        {16, 16, 6},
        {8, 8, 6},
        {32, 32, 6},
        {16, 16, 6},
        {8, 8, 6}
    };
    for (int i = 0; i < 5; ++i)
    {
        osg::ref_ptr<osg::BindImageTexture> prefilterImage = new osg::BindImageTexture(0, specularCubemap->getOsgTexture(), osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F, i, true);
        osg::ref_ptr<osg::Program> prefilterProgram = new osg::Program;
        prefilterProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "IBL/Prefilter" + std::to_string(i) + ".comp.glsl"));
        computeStateSet->setAttribute(prefilterProgram, osg::StateAttribute::ON);
        computeStateSet->setAttribute(prefilterImage, osg::StateAttribute::ON);
        state->apply(computeStateSet);
        extensions->glDispatchCompute(numGroups[i][0], numGroups[i][1], numGroups[i][2]);
        extensions->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    return specularCubemap;
}

void generateDiffuseSHCoeff(TextureCubemap* imageCubemap)
{
    osg::ref_ptr<osg::Vec4Array> shCoeffBuffer = new osg::Vec4Array(16 * 16 * 9);
    osg::ref_ptr<osg::ShaderStorageBufferObject> shCoeffSSBO = new osg::ShaderStorageBufferObject;
    shCoeffBuffer->setBufferObject(shCoeffSSBO);
    osg::ref_ptr<osg::ShaderStorageBufferBinding> shCoeffSSBB = new osg::ShaderStorageBufferBinding(0, shCoeffBuffer, 0, shCoeffBuffer->getTotalDataSize());

    osg::ref_ptr<osg::StateSet> computeStateSet = new osg::StateSet;
    osg::ref_ptr<osg::Program> diffuseSHCoeffProgram = new osg::Program;
    diffuseSHCoeffProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "SphericalHarmonics/DiffuseSHCoeff.comp.glsl"));
    computeStateSet->setAttribute(diffuseSHCoeffProgram, osg::StateAttribute::ON);
    computeStateSet->setAttribute(shCoeffSSBB, osg::StateAttribute::ON);
    computeStateSet->addUniform(new osg::Uniform("uCubemapTexture", 0));
    computeStateSet->setTextureAttribute(0, imageCubemap->getOsgTexture());
    computeStateSet->setMode(GL_TEXTURE_CUBE_MAP_SEAMLESS, osg::StateAttribute::ON);

    osg::State* state = Context::get().getGraphicsContext()->getState();
    osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
    state->apply(computeStateSet);
    extensions->glDispatchCompute(16, 16, 1);
    extensions->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    shCoeffSSBO->getGLBufferObject(state->getContextID())->bindBuffer();
    extensions->glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, shCoeffBuffer->getTotalDataSize(), &shCoeffBuffer->at(0));

    const osg::Vec4f* shCoeffTemp = &shCoeffBuffer->at(0);
    std::array<osg::Vec4f, 9> diffuseSHCoeff;
    for (int i = 0; i < 16 * 16; ++i)
    {
        diffuseSHCoeff[0] += shCoeffTemp[i * 9 + 0];
        diffuseSHCoeff[1] += shCoeffTemp[i * 9 + 1];
        diffuseSHCoeff[2] += shCoeffTemp[i * 9 + 2];
        diffuseSHCoeff[3] += shCoeffTemp[i * 9 + 3];
        diffuseSHCoeff[4] += shCoeffTemp[i * 9 + 4];
        diffuseSHCoeff[5] += shCoeffTemp[i * 9 + 5];
        diffuseSHCoeff[6] += shCoeffTemp[i * 9 + 6];
        diffuseSHCoeff[7] += shCoeffTemp[i * 9 + 7];
        diffuseSHCoeff[8] += shCoeffTemp[i * 9 + 8];
    }
    return;
}

Texture2D* generateBRDFLut()
{
    osg::ref_ptr<osg::Texture2D> brdfLutTexture = new osg::Texture2D;
    brdfLutTexture->setTextureSize(128, 128);
    brdfLutTexture->setInternalFormat(GL_RG16F);
    brdfLutTexture->setSourceFormat(GL_RG);
    brdfLutTexture->setSourceType(GL_HALF_FLOAT);
    brdfLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    brdfLutTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    brdfLutTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    brdfLutTexture->setUseHardwareMipMapGeneration(false);

    osg::ref_ptr<osg::Program> brdfLutProgram = new osg::Program;
    brdfLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "IBL/BRDFLut.comp.glsl"));
    osg::ref_ptr<osg::BindImageTexture> brdfLutImage = new osg::BindImageTexture(0, brdfLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RG16F);
    osg::ref_ptr<osg::StateSet> computeStateSet = new osg::StateSet;
    computeStateSet->setAttribute(brdfLutProgram, osg::StateAttribute::ON);
    computeStateSet->setAttribute(brdfLutImage, osg::StateAttribute::ON);

    osg::State* state = Context::get().getGraphicsContext()->getState();
    osg::GLExtensions* extensions = state->get<osg::GLExtensions>();
    state->apply(computeStateSet);
    extensions->glDispatchCompute(4, 4, 1);
    extensions->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return new Texture2D(brdfLutTexture);
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
    //viewer.setRealizeOperation(new editor::EditorRealizeOperation);
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.addView(engine->getView());
    viewer.realize();

    Context::get().getGraphicsContext()->makeCurrent();

    //createTestAssets();

    /*osg::ref_ptr<osg::Image> hdrImage = osgDB::readImageFile(TEMP_DIR "zwartkops_straight_sunset_1k.hdr");
    TextureImportOptions options;
    options.format = Texture::Format::RGBA16F;
    options.minFilter = Texture::FilterMode::Linear;
    TextureCubemap* textureCubemap = new TextureCubemap(hdrImage, options);
    am.createAsset(textureCubemap, "Engine/Texture/TestCubemap")->save();*/

    /*osg::Image* image = osgDB::readImageFile(TEMP_DIR "white.png");
    osg::ref_ptr<Texture2D> whiteTexture2D = new Texture2D(image, TextureImportOptions());
    am.createAsset(whiteTexture2D, "Engine/Texture/White")->save();*/

    /*Asset* materialAsset = am.getAsset("Engine/Material/TestMaterial");
    materialAsset->load();
    Material* material = materialAsset->getRootObject<Material>();

    osg::ref_ptr<Mesh> mesh = new Mesh(TEMP_DIR "sphere.obj");
    mesh->setDefaultMaterial(0, material);
    am.createAsset(mesh, "Engine/Mesh/Sphere")->save();*/

    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TEMP_DIR "awesomeface.png");
    osg::ref_ptr<Texture2D> texture = new Texture2D(image, TextureImportOptions());
    texture->setDataCompression(true);
    am.createAsset(texture, "Engine/Texture/AwesomeFace")->save();

    /*osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TEMP_DIR "zwartkops_straight_sunset_1k.hdr");
    TextureImportOptions options;
    options.format = Texture::Format::RGBA16F;
    options.minFilter = Texture::FilterMode::Linear_Mipmap_Linear;
    options.mipmapGeneration = true;
    osg::ref_ptr<TextureCubemap> texture = new TextureCubemap(image, options);
    texture->setDataCompression(true);
    am.createAsset(texture, "Engine/Texture/TestCubemap")->save();*/

    // Asset* asset = am.getAsset("Engine/Texture/Test");
    // asset->load();
    // Texture2D* texture2D = asset->getRootObject<Texture2D>();

    //Asset* cubemapAsset = am.getAsset("Engine/Texture/TestCubemap");
    //cubemapAsset->load();
    //TextureCubemap* cubemap = cubemapAsset->getRootObject<TextureCubemap>();
    ////TextureCubemap* specularCubemap = generateSpecularCubemap(cubemap);
    ////am.createAsset(specularCubemap, "Engine/Texture/SpecularCubemap")->save();
    //generateDiffuseSHCoeff(cubemap);

    /*Material* testMaterial = am.getAsset("Engine/Material/TestMaterial")->getRootObject<Material>();
    Mesh* mesh = new Mesh(TEMP_DIR "Suzanne.obj");
    mesh->setDefaultMaterial(0, testMaterial);
    am.createAsset(mesh, "Engine/Mesh/Suzanne")->save();*/

    /*osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TEMP_DIR "white.png");
    TextureImportOptions options;
    options.format = Texture::Format::RGBA16F;
    options.minFilter = Texture::FilterMode::Linear_Mipmap_Linear;
    options.mipmapGeneration = true;
    osg::ref_ptr<TextureCubemap> texture = new TextureCubemap(image, options, 1);
    am.createAsset(texture, "Engine/Texture/WhiteCubemap")->save();*/

    /*osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TEMP_DIR "T_Rock_Marble_Polished_D.PNG");
    TextureImportOptions options;
    options.format = Texture::Format::RGB8;
    Texture2D* texture = new Texture2D(image, options);
    texture->setDataCompression(true);
    am.createAsset(texture, "Engine/Texture/T_Rock_Marble_Polished_D")->save();*/

    Context::get().getGraphicsContext()->releaseContext();

    return 0;
}
