#include <Core/Render/Pipeline.h>
#include "ControllerManipulator.h"
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/DispatchCompute>
#include <osg/Texture3D>
#include <osg/BindImageTexture>
#include <osg/BufferIndexBinding>
#include <osg/BlendFunc>

struct ViewData
{
    osg::Matrixf viewMatrix;
    osg::Matrixf inverseViewMatrix;
    osg::Matrixf projectionMatrix;
    osg::Matrixf inverseProjectionMatrix;
}gViewData;
osg::ref_ptr<osg::UniformBufferBinding> gViewDataUBB;

struct AtmosphereParameters
{
    float solarAltitude = 30.0f;
    float solarAzimuth = 0.0f;
    osg::Vec3 sunColor = osg::Vec3(1.0, 1.0, 1.0);
    float sunIntensity = 6.0f;
    float groundRadius = 6360.0f;
    float atmosphereRadius = 6420.0f;
    osg::Vec3 groundAlbedo = osg::Vec3(0.4, 0.4, 0.4);
    osg::Vec3 rayleighScatteringCoeff = osg::Vec3(0.175287, 0.409607, 1.0);
    float rayleighScatteringScale = 0.0331f;
    float rayleighDensityH = 8.0f;
    float mieScatteringBase = 0.003996f;
    float mieAbsorptionBase = 0.000444f;
    float mieDensityH = 1.2f;
    osg::Vec3 ozoneAbsorptionCoeff = osg::Vec3(0.345561, 1.0, 0.045189);
    float ozoneAbsorptionScale = 0.001881f;
    float ozoneCenterHeight = 25.0f;
    float ozoneThickness = 15.0f;
}gAtmosphereParameters;

struct AtmosphereShaderParameters
{
    osg::Vec3 rayleighScatteringBase;
    float mieScatteringBase;
    osg::Vec3 ozoneAbsorptionBase;
    float mieAbsorptionBase;
    float rayleighDensityH;
    float mieDensityH;
    float ozoneCenterHeight;
    float ozoneThickness;
    osg::Vec3 groundAlbedo;
    float groundRadius;
    osg::Vec3 sunDirection;
    float atmosphereRadius;
    osg::Vec3 sunIntensity;
}gAtmosphereShaderParameters;
osg::ref_ptr<osg::UniformBufferBinding> gAtmosphereShaderParametersUBB;

void calcAtmosphereShaderParameters(const AtmosphereParameters& param, AtmosphereShaderParameters& shaderParam)
{
    shaderParam.rayleighScatteringBase = param.rayleighScatteringCoeff * param.rayleighScatteringScale;
    shaderParam.mieScatteringBase = param.mieScatteringBase;
    shaderParam.ozoneAbsorptionBase = param.ozoneAbsorptionCoeff * param.ozoneAbsorptionScale;
    shaderParam.mieAbsorptionBase = param.mieAbsorptionBase;
    shaderParam.rayleighDensityH = param.rayleighDensityH;
    shaderParam.mieDensityH = param.mieDensityH;
    shaderParam.ozoneCenterHeight = param.ozoneCenterHeight;
    shaderParam.ozoneThickness = param.ozoneThickness;
    shaderParam.groundAlbedo = param.groundAlbedo;
    shaderParam.groundRadius = param.groundRadius;
    float altitude = osg::DegreesToRadians(param.solarAltitude);
    float azimuth = osg::DegreesToRadians(param.solarAzimuth);
    shaderParam.sunDirection = osg::Vec3(
        std::cos(altitude) * std::sin(azimuth),
        std::cos(altitude) * std::cos(azimuth),
        std::sin(altitude)
    );
    shaderParam.atmosphereRadius = param.atmosphereRadius;
    shaderParam.sunIntensity = param.sunColor * param.sunIntensity;
}

class InputPassCameraPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        const osg::Matrixd& viewMatrix = renderInfo.getState()->getInitialViewMatrix();
        const osg::Matrixd& inverseViewMatrix = renderInfo.getState()->getInitialInverseViewMatrix();
        const osg::Matrixd& projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        osg::Matrixd inverseProjectionMatrix = osg::Matrixd::inverse(projectionMatrix);
        gViewData.viewMatrix = viewMatrix;
        gViewData.inverseViewMatrix = inverseViewMatrix;
        gViewData.projectionMatrix = projectionMatrix;
        gViewData.inverseProjectionMatrix = inverseProjectionMatrix;
        osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gViewDataUBB->getBufferData());
        buffer->assign((float*)&gViewData, (float*)(&gViewData + 1));
        buffer->dirty();
    }
};

class DrawTimesCullCallback : public osg::DrawableCullCallback
{
    uint32_t _times;
public:
    DrawTimesCullCallback(uint32_t times) : _times(times) {}

    virtual bool cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const
    {
        if (_times > 0)
        {
            const_cast<uint32_t&>(_times)--;
            return false;
        }
        return true;
    }
};

int main()
{
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

    calcAtmosphereShaderParameters(gAtmosphereParameters, gAtmosphereShaderParameters);
    osg::ref_ptr<osg::FloatArray> atmosphereShaderParametersBuffer = new osg::FloatArray((float*)&gAtmosphereShaderParameters, (float*)(&gAtmosphereShaderParameters + 1));
    osg::ref_ptr<osg::UniformBufferObject> atmosphereShaderParametersUBO = new osg::UniformBufferObject;
    atmosphereShaderParametersBuffer->setBufferObject(atmosphereShaderParametersUBO);
    gAtmosphereShaderParametersUBB = new osg::UniformBufferBinding(1, atmosphereShaderParametersBuffer, 0, sizeof(AtmosphereShaderParameters));

    const int width = 1024, height = 1024;
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
    camera->setProjectionMatrixAsPerspective(60.0, double(width) / double(height), 0.1, 1000.0);

    osg::ref_ptr<osg::Texture3D> noise1Texture = new osg::Texture3D;
    noise1Texture->setTextureSize(128, 128, 128);
    noise1Texture->setInternalFormat(GL_RGBA8);
    noise1Texture->setSourceFormat(GL_RGBA);
    noise1Texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    noise1Texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    noise1Texture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
    noise1Texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noise1Texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> noise1Image = new osg::BindImageTexture(0, noise1Texture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> noise1Program = new osg::Program;
    noise1Program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/Noise1.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> noise1Dispatch = new osg::DispatchCompute(32, 32, 2);
    noise1Dispatch->setCullingActive(false);
    noise1Dispatch->getOrCreateStateSet()->setAttribute(noise1Program, osg::StateAttribute::ON);
    noise1Dispatch->getOrCreateStateSet()->setAttribute(noise1Image, osg::StateAttribute::ON);
    noise1Dispatch->setCullCallback(new DrawTimesCullCallback(1));
    rootGroup->addChild(noise1Dispatch);

    osg::ref_ptr<osg::Texture3D> noise2Texture = new osg::Texture3D;
    noise2Texture->setTextureSize(32, 32, 32);
    noise2Texture->setInternalFormat(GL_RGBA8);
    noise2Texture->setSourceFormat(GL_RGBA);
    noise2Texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    noise2Texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    noise2Texture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
    noise2Texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noise2Texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> noise2Image = new osg::BindImageTexture(0, noise2Texture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> noise2Program = new osg::Program;
    noise2Program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/Noise2.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> noise2Dispatch = new osg::DispatchCompute(8, 8, 1);
    noise2Dispatch->setCullingActive(false);
    noise2Dispatch->getOrCreateStateSet()->setAttribute(noise2Program, osg::StateAttribute::ON);
    noise2Dispatch->getOrCreateStateSet()->setAttribute(noise2Image, osg::StateAttribute::ON);
    noise2Dispatch->setCullCallback(new DrawTimesCullCallback(1));
    rootGroup->addChild(noise2Dispatch);

    /*osg::ref_ptr<osg::Texture2D> noise3Texture = new osg::Texture2D;
    noise3Texture->setTextureSize(128, 128);
    noise3Texture->setInternalFormat(GL_RGBA8);
    noise3Texture->setSourceFormat(GL_RGBA);
    noise3Texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    noise3Texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    noise3Texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    noise3Texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> noise3Image = new osg::BindImageTexture(0, noise3Texture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> noise3Program = new osg::Program;
    noise3Program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricClouds/Noise3.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> noise3Dispatch = new osg::DispatchCompute();
    noise3Dispatch->getOrCreateStateSet()->setAttribute(noise3Program, osg::StateAttribute::ON);
    noise3Dispatch->getOrCreateStateSet()->setAttribute(noise3Image, osg::StateAttribute::ON);
    rootGroup->addChild(noise3Dispatch);*/

    osg::ref_ptr<osg::Texture2D> transmittanceLutTexture = new osg::Texture2D;
    transmittanceLutTexture->setTextureSize(256, 64);
    transmittanceLutTexture->setInternalFormat(GL_R11F_G11F_B10F);
    transmittanceLutTexture->setSourceFormat(GL_RGB);
    transmittanceLutTexture->setSourceType(GL_HALF_FLOAT);
    transmittanceLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    transmittanceLutTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    transmittanceLutTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    transmittanceLutTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    osg::ref_ptr<osg::BindImageTexture> transmittanceLutImage = new osg::BindImageTexture(0, transmittanceLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_R11F_G11F_B10F);
    osg::ref_ptr<osg::Program> transmittanceLutProgram = new osg::Program;
    transmittanceLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Atmosphere/TransmittanceLut.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> transmittanceLutDispatch = new osg::DispatchCompute(8, 2, 1);
    transmittanceLutDispatch->setCullingActive(false);
    transmittanceLutDispatch->getOrCreateStateSet()->setAttribute(transmittanceLutProgram, osg::StateAttribute::ON);
    transmittanceLutDispatch->getOrCreateStateSet()->setAttribute(transmittanceLutImage, osg::StateAttribute::ON);
    transmittanceLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereShaderParametersUBB, osg::StateAttribute::ON);
    rootGroup->addChild(transmittanceLutDispatch);

    osg::ref_ptr<osg::Texture2D> multiScatteringLutTexture = new osg::Texture2D;
    multiScatteringLutTexture->setTextureSize(32, 32);
    multiScatteringLutTexture->setInternalFormat(GL_R11F_G11F_B10F);
    multiScatteringLutTexture->setSourceFormat(GL_RGB);
    multiScatteringLutTexture->setSourceType(GL_HALF_FLOAT);
    multiScatteringLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    multiScatteringLutTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    multiScatteringLutTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    multiScatteringLutTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    osg::ref_ptr<osg::BindImageTexture> multiScatteringLutImage = new osg::BindImageTexture(0, multiScatteringLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_R11F_G11F_B10F);
    osg::ref_ptr<osg::Program> multiScatteringLutProgram = new osg::Program;
    multiScatteringLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Atmosphere/MultiScatteringLut.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> multiScatteringLutDispatch = new osg::DispatchCompute(1, 1, 1);
    multiScatteringLutDispatch->setCullingActive(false);
    multiScatteringLutDispatch->getOrCreateStateSet()->setAttribute(multiScatteringLutProgram, osg::StateAttribute::ON);
    multiScatteringLutDispatch->getOrCreateStateSet()->setAttribute(multiScatteringLutImage, osg::StateAttribute::ON);
    multiScatteringLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereShaderParametersUBB, osg::StateAttribute::ON);
    multiScatteringLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
    multiScatteringLutDispatch->getOrCreateStateSet()->setTextureAttribute(0, transmittanceLutTexture, osg::StateAttribute::ON);
    rootGroup->addChild(multiScatteringLutDispatch);

    osg::ref_ptr<osg::Texture2D> skyViewLutTexture = new osg::Texture2D;
    skyViewLutTexture->setTextureSize(192, 108);
    skyViewLutTexture->setInternalFormat(GL_R11F_G11F_B10F);
    skyViewLutTexture->setSourceFormat(GL_RGB);
    skyViewLutTexture->setSourceType(GL_HALF_FLOAT);
    skyViewLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    skyViewLutTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    skyViewLutTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::MIRROR);
    skyViewLutTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    osg::ref_ptr<osg::BindImageTexture> skyViewLutImage = new osg::BindImageTexture(0, skyViewLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_R11F_G11F_B10F);
    osg::ref_ptr<osg::Program> skyViewLutProgram = new osg::Program;
    skyViewLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Atmosphere/SkyViewLut.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> skyViewLutDispatch = new osg::DispatchCompute(6, 4, 1);
    skyViewLutDispatch->setCullingActive(false);
    skyViewLutDispatch->getOrCreateStateSet()->setAttribute(skyViewLutProgram, osg::StateAttribute::ON);
    skyViewLutDispatch->getOrCreateStateSet()->setAttribute(skyViewLutImage, osg::StateAttribute::ON);
    skyViewLutDispatch->getOrCreateStateSet()->setAttribute(gViewDataUBB, osg::StateAttribute::ON);
    skyViewLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereShaderParametersUBB, osg::StateAttribute::ON);
    skyViewLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
    skyViewLutDispatch->getOrCreateStateSet()->setTextureAttribute(0, transmittanceLutTexture, osg::StateAttribute::ON);
    skyViewLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uMultiScatteringLutTexture", 1));
    skyViewLutDispatch->getOrCreateStateSet()->setTextureAttribute(1, multiScatteringLutTexture, osg::StateAttribute::ON);
    rootGroup->addChild(skyViewLutDispatch);

    osg::ref_ptr<osg::Texture3D> aerialPerspectiveLutTexture = new osg::Texture3D;
    aerialPerspectiveLutTexture->setTextureSize(32, 32, 16);
    aerialPerspectiveLutTexture->setInternalFormat(GL_RGBA16F);
    aerialPerspectiveLutTexture->setSourceFormat(GL_RGBA);
    aerialPerspectiveLutTexture->setSourceType(GL_HALF_FLOAT);
    aerialPerspectiveLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    aerialPerspectiveLutTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    aerialPerspectiveLutTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    aerialPerspectiveLutTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    aerialPerspectiveLutTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    osg::ref_ptr<osg::BindImageTexture> aerialPerspectiveLutImage = new osg::BindImageTexture(0, aerialPerspectiveLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F);
    osg::ref_ptr<osg::Program> aerialPerspectiveLutProgram = new osg::Program;
    aerialPerspectiveLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Atmosphere/AerialPerspectiveLut.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> aerialPerspectiveLutDispatch = new osg::DispatchCompute(2, 2, 4);
    aerialPerspectiveLutDispatch->setCullingActive(false);
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setAttribute(aerialPerspectiveLutProgram, osg::StateAttribute::ON);
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setAttribute(aerialPerspectiveLutImage, osg::StateAttribute::ON);
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setAttribute(gViewDataUBB, osg::StateAttribute::ON);
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereShaderParametersUBB, osg::StateAttribute::ON);
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setTextureAttribute(0, transmittanceLutTexture, osg::StateAttribute::ON);
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uMultiScatteringLutTexture", 1));
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setTextureAttribute(1, multiScatteringLutTexture, osg::StateAttribute::ON);
    rootGroup->addChild(aerialPerspectiveLutDispatch);

    osg::Program* inputProgram = new osg::Program;
    inputProgram->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/TestInput.vert.glsl"));
    inputProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/TestInput.frag.glsl"));
    osg::ref_ptr<osg::Node> meshNode = osgDB::readNodeFile(TEMP_DIR "suzanne.obj");
    meshNode->getOrCreateStateSet()->setAttribute(inputProgram, osg::StateAttribute::ON);
    rootGroup->addChild(meshNode);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    osg::ref_ptr<xxx::Pipeline::Pass> inputPass = pipeline->addInputPass("Input", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //inputPass->getCamera()->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    inputPass->getCamera()->setClearColor(osg::Vec4(0.09, 0.33, 0.81, 1.0));
    inputPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA8);
    inputPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, osg::Texture::NEAREST, osg::Texture::NEAREST);
    inputPass->getCamera()->setPreDrawCallback(new InputPassCameraPreDrawCallback);

    osg::ref_ptr<osg::Shader> screenQuadVertexShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    /*osg::ref_ptr<osg::Program> noiseVisualizeProgram = new osg::Program;
    noiseVisualizeProgram->addShader(screenQuadVertexShader);
    noiseVisualizeProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "VolumetricCloud/RayMarching.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> workPass = pipeline->addWorkPass("Work", noiseVisualizeProgram);
    workPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    workPass->applyTexture(noise1Texture, "uNoise1Texture", 0);
    workPass->applyTexture(noise2Texture, "uNoise2Texture", 1);
    workPass->applyTexture(inputPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 2);
    workPass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 3);
    workPass->setAttribute(gViewDataUBB);*/

    osg::ref_ptr<osg::Program> atmosphereProgram = new osg::Program;
    atmosphereProgram->addShader(screenQuadVertexShader);
    atmosphereProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Atmosphere/RayMarching.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> atmospherePass = pipeline->addWorkPass("Atmosphere", atmosphereProgram);
    atmospherePass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    atmospherePass->applyTexture(transmittanceLutTexture, "uTransmittanceLutTexture", 0);
    atmospherePass->applyTexture(multiScatteringLutTexture, "uMultiScatteringLutTexture", 1);
    atmospherePass->applyTexture(skyViewLutTexture, "uSkyViewLutTexture", 2);
    atmospherePass->applyTexture(aerialPerspectiveLutTexture, "uAerialPerspectiveLutTexture", 3);
    atmospherePass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uSceneDepthTexture", 4);
    atmospherePass->setAttribute(gViewDataUBB);
    atmospherePass->setMode(GL_BLEND);
    atmospherePass->setAttribute(new osg::BlendFunc(GL_ONE, GL_SRC_ALPHA));

    osg::ref_ptr<osg::Program> volumetricCloudProgram = new osg::Program;
    volumetricCloudProgram->addShader(screenQuadVertexShader);
    volumetricCloudProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "VolumetricCloud/Draw.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> volumetricCloudPass = pipeline->addWorkPass("VolumetricCloud", volumetricCloudProgram);
    volumetricCloudPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    volumetricCloudPass->applyTexture(noise1Texture, "uNoise1Texture", 0);
    volumetricCloudPass->applyTexture(noise2Texture, "uNoise2Texture", 1);
    volumetricCloudPass->applyTexture(transmittanceLutTexture, "uTransmittanceLutTexture", 2);
    volumetricCloudPass->applyTexture(multiScatteringLutTexture, "uMultiScatteringLutTexture", 3);
    volumetricCloudPass->applyTexture(skyViewLutTexture, "uSkyViewLutTexture", 4);
    volumetricCloudPass->applyTexture(aerialPerspectiveLutTexture, "uAerialPerspectiveLutTexture", 5);
    volumetricCloudPass->setAttribute(gViewDataUBB);
    volumetricCloudPass->setAttribute(gAtmosphereShaderParametersUBB);

    osg::ref_ptr<osg::Program> colorGradingProgram = new osg::Program;
    colorGradingProgram->addShader(screenQuadVertexShader);
    colorGradingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineAtmosphereAndCloud.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> finalPass = pipeline->addFinalPass("Final", colorGradingProgram);
    finalPass->applyTexture(atmospherePass->getBufferTexture(BufferType::COLOR_BUFFER0), "uAtmosphereColorTexture", 0);
    finalPass->applyTexture(volumetricCloudPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCloudColorTexture", 1);

    viewer->setCameraManipulator(new ControllerManipulator(20.0));

    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
