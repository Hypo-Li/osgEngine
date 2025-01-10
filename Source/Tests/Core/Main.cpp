#include "Engine/Core/OsgReflection.h"
#include "Engine/Core/Entity.h"
#include "Engine/Core/Scene.h"
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
#include <random>

using namespace xxx;
using namespace xxx::refl;

static const char testShaderSource[] = R"(
void calcMaterial(in MaterialInputs mi, inout MaterialOutputs mo)
{
    vec4 texColor = texture(uBaseColorTexture, mi.texcoord0.xy);
    mo.emissive = uEmissive;
    mo.baseColor = texColor.rgb * uBaseColorFactor;
    mo.opacity = texColor.a;
    mo.metallic = uMetallic;
    mo.roughness = uRoughness;
}
)";

void createTestAssets()
{
    AssetManager& am = AssetManager::get();

    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(TEMP_DIR "awesomeface.png");
    osg::ref_ptr<Texture2D> texture = new Texture2D(image, TextureImportOptions());
    am.createAsset("Engine/Texture/AwesomeFace", texture)->save();

    osg::ref_ptr<Shader> shader = new Shader;
    shader->addParameter("BaseColorTexture", texture.get());
    shader->addParameter("BaseColorFactor", osg::Vec3f(1, 1, 1));
    shader->addParameter("Emissive", osg::Vec3f(0, 0, 0));
    shader->addParameter("Metallic", 0.0f);
    shader->addParameter("Roughness", 0.5f);
    shader->setSource(testShaderSource);
    am.createAsset("Engine/Shader/TestShader", shader)->save();

    osg::ref_ptr<Material> material = new Material;
    material->setShader(shader);
    am.createAsset("Engine/Material/TestMaterial", material)->save();

    osg::ref_ptr<Mesh> mesh = new Mesh(TEMP_DIR "cube.obj");
    mesh->setMaterial(0, material);
    am.createAsset("Engine/Mesh/Cube", mesh)->save();

    osg::ref_ptr<Entity> entity = new Entity("TestEntity");
    MeshRenderer* meshRenderer = entity->addComponent<MeshRenderer>();
    meshRenderer->setMesh(mesh);
    am.createAsset("Engine/Entity/TestEntity", entity)->save();
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

    //return new Texture2D(brdfLutTexture);
    return nullptr;
}

void create1x1Texture(osg::Vec4f color, const std::string& name)
{
    osg::Image* image = new osg::Image;
    image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    uint8_t* rgba = (uint8_t*)(image->data());
    rgba[0] = color.r() * 255;
    rgba[1] = color.g() * 255;
    rgba[2] = color.b() * 255;
    rgba[3] = color.a() * 255;

    TextureImportOptions options;
    osg::ref_ptr<Texture2D> tex2d = new Texture2D(image, options);
    AssetManager::get().createAsset("Engine/Texture/" + name, tex2d)->save();
}

class SerializationTest : public xxx::Object
{
    REFLECT_CLASS(SerializationTest)

public:
    // Fundamental
    bool mBool;
    char mChar;
    wchar_t mWChar;
    int8_t mInt8;
    int16_t mInt16;
    int32_t mInt32;
    int64_t mInt64;
    uint8_t mUint8;
    uint16_t mUint16;
    uint32_t mUint32;
    uint64_t mUint64;
    float mFloat;
    double mDouble;

    // Enumeration
    enum class Color
    {
        Red,
        Green,
        Blue,
        Black,
        White,
    };
    Color mColorEnum;

    // Special
    std::array<int, 4> mArray;
    std::list<int> mList;
    std::map<int, int> mMap;
    std::pair<int, float> mPair;
    std::set<int> mSet;
    std::string mString;
    std::unordered_map<int, int> mUnorderedMap;
    std::unordered_set<int> mUnorderedSet;
    std::variant<int, float> mVariant;
    std::vector<int> mVector;
};

namespace xxx::refl
{
    template <>
    Type* Reflection::createType<SerializationTest::Color>()
    {
        Enumeration* enumeration = new TEnumeration<SerializationTest::Color>("Color", {
            {"Red", SerializationTest::Color::Red},
            {"Green", SerializationTest::Color::Green},
            {"Blue", SerializationTest::Color::Blue},
            {"Black", SerializationTest::Color::Black},
            {"White", SerializationTest::Color::White},
        });
        return enumeration;
    }

    template <>
    Type* Reflection::createType<SerializationTest>()
    {
        Class* clazz = new TClass<SerializationTest>("SerializationTest");
        clazz->addProperty("mBool", &SerializationTest::mBool);
        clazz->addProperty("mChar", &SerializationTest::mChar);
        clazz->addProperty("mWChar", &SerializationTest::mWChar);
        clazz->addProperty("mInt8", &SerializationTest::mInt8);
        clazz->addProperty("mInt16", &SerializationTest::mInt16);
        clazz->addProperty("mInt32", &SerializationTest::mInt32);
        clazz->addProperty("mInt64", &SerializationTest::mInt64);
        clazz->addProperty("mUint8", &SerializationTest::mUint8);
        clazz->addProperty("mUint16", &SerializationTest::mUint16);
        clazz->addProperty("mUint32", &SerializationTest::mUint32);
        clazz->addProperty("mUint64", &SerializationTest::mUint64);
        clazz->addProperty("mFloat", &SerializationTest::mFloat);
        clazz->addProperty("mDouble", &SerializationTest::mDouble);

        clazz->addProperty("mColorEnum", &SerializationTest::mColorEnum);

        clazz->addProperty("mArray", &SerializationTest::mArray);
        clazz->addProperty("mList", &SerializationTest::mList);
        clazz->addProperty("mMap", &SerializationTest::mMap);
        clazz->addProperty("mPair", &SerializationTest::mPair);
        clazz->addProperty("mSet", &SerializationTest::mSet);
        clazz->addProperty("mString", &SerializationTest::mString);
        clazz->addProperty("mUnorderedMap", &SerializationTest::mUnorderedMap);
        clazz->addProperty("mUnorderedSet", &SerializationTest::mUnorderedSet);
        clazz->addProperty("mVariant", &SerializationTest::mVariant);
        clazz->addProperty("mVector", &SerializationTest::mVector);
        return clazz;
    }
}

void createSerializationTest()
{
    SerializationTest* st = new SerializationTest;
    st->mBool = true;
    st->mChar = 'F';
    st->mWChar = 'S';
    st->mInt8 = 61;
    st->mInt16 = 85;
    st->mInt32 = 92;
    st->mInt64 = 177;
    st->mUint8 = 12;
    st->mUint16 = 55;
    st->mUint32 = 49;
    st->mUint64 = 248;
    st->mFloat = 240.57;
    st->mDouble = 131.775;
    st->mColorEnum = SerializationTest::Color::Black;
    st->mArray = { 2, 4, 6, 8 };
    st->mList = { 1, 3, 5, 7, 9 };
    st->mMap = { {0, 1}, {2, 4}, {3, 6} };
    st->mPair = { 5, 5.0f };
    st->mSet = { 1, 3, 5, 7, 11, 13 };
    st->mString = "I am String";
    st->mUnorderedMap = { {0, 2}, {2, 5}, {3, 7} };
    st->mUnorderedSet = { 1, 3, 5, 7, 9, 11, 13, 15 };
    st->mVariant = 2.0f;
    st->mVector = { 9, 8, 7, 6, 5, 4 };

    AssetManager& am = AssetManager::get();
    am.createAsset("Engine/Test/SerializationTest", st)->save();
}

void createScene()
{
    AssetManager& am = AssetManager::get();
    osg::ref_ptr <Scene> scene = new Scene;

    osg::ref_ptr<Entity> entity = new Entity("TestEntity");
    MeshRenderer* meshRenderer = entity->addComponent<MeshRenderer>();
    Mesh* mesh = am.getAsset("Engine/Mesh/Cube")->getRootObjectSafety<Mesh>();
    meshRenderer->setMesh(mesh);
    scene->addEntity(entity);

    osg::ref_ptr<Entity> dirLight = new Entity("DirectionalLight");
    DirectionalLight* directionalLight = dirLight->addComponent<DirectionalLight>();
    directionalLight->setDirection(osg::Vec3f(0.5, 0.5, -0.5));
    scene->addEntity(dirLight);

    osg::ref_ptr<Entity> envLight = new Entity("EnvironmentLight");
    ImageBasedLight* imageBasedLight = envLight->addComponent<ImageBasedLight>();
    envLight->setTranslation(osg::Vec3f(0, 0, 1));
    scene->addEntity(envLight, dirLight);

    am.createAsset("Engine/Scene/TestScene", scene)->save();
}

void createTestCubemap()
{
    AssetManager& am = AssetManager::get();
    TextureImportOptions options;
    options.format = Texture::Format::RGBA16F;
    osg::Image* hdrImage = osgDB::readImageFile(TEMP_DIR "zwartkops_straight_sunset_1k.hdr");
    TextureCubemap* textureCubemap = new xxx::TextureCubemap(hdrImage, options);
    am.createAsset("Engine/Texture/TestCubemap", textureCubemap)->save();
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
    osgViewer::Viewer* viewer = new osgViewer::Viewer;
    Engine* engine = new Engine(viewer, engineSetupConfig);
    //viewer.setRealizeOperation(new editor::EditorRealizeOperation);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->realize();

    Context::get().getGraphicsContext()->makeCurrent();

    //createScene();
    //create1x1Texture(osg::Vec4(0.5, 0.5, 1, 1), "Normal");

    Texture2D* whiteTex = dynamic_cast<Texture2D*>(am.getAsset("Engine/Texture/White")->getRootObjectSafety());
    Texture2D* normalTex = dynamic_cast<Texture2D*>(am.getAsset("Engine/Texture/Normal")->getRootObjectSafety());

    osg::ref_ptr<Shader> shader = new Shader;
    shader->addParameter("BaseColorTexture", whiteTex);
    shader->addParameter("BaseColorFactor", osg::Vec4f(0.8, 0.8, 0.8, 1.0));
    shader->addParameter("MetallicRoughnessTexture", whiteTex);
    shader->addParameter("MetallicFactor", 0.0f);
    shader->addParameter("RoughnessFactor", 0.5f);
    shader->addParameter("EmissiveTexture", whiteTex);
    shader->addParameter("EmissiveFactor", osg::Vec3f(0, 0, 0));
    shader->addParameter("OcclusionTexture", whiteTex);
    shader->addParameter("NormalTexture", normalTex);
    static const char* gltfShaderSource =
R"(void calcMaterial(in MaterialInputs mi, inout MaterialOutputs mo)
{
    vec4 baseColor = texture(uBaseColorTexture, mi.texcoord0.xy);
    baseColor *= uBaseColorFactor;
    mo.baseColor = baseColor.rgb;
    mo.opacity = baseColor.a;

    vec2 metallicRoughness = texture(uMetallicRoughnessTexture, mi.texcoord0.xy).rg;
    metallicRoughness *= vec2(uMetallicFactor, uRoughnessFactor);
    mo.metallic = metallicRoughness.x;
    mo.roughness = metallicRoughness.y;

    vec3 emissive = texture(uEmissiveTexture, mi.texcoord0.xy).rgb;
    emissive *= uEmissiveFactor;
    mo.emissive = emissive;

    mo.occlusion = texture(uOcclusionTexture, mi.texcoord0.xy).r;

    mo.normal = texture(uNormalTexture, mi.texcoord0.xy).rgb * 2.0 - 1.0;
}
)";
    shader->setSource(gltfShaderSource);
    am.createAsset("Engine/Shader/GLTF", shader)->save();

    /*osg::ref_ptr<Material> material = new Material;
    material->setShader(shader);
    am.createAsset("Engine/Material/TestMaterial2", material)->save();*/


    Context::get().getGraphicsContext()->releaseContext();

    return 0;
}
