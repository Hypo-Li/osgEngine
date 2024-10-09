#include <Engine/Render/Pipeline.h>
#include "ControllerManipulator.h"
#include "ImGuiHandler.h"
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osg/DispatchCompute>
#include <osg/Texture3D>
#include <osg/BindImageTexture>
#include <osg/BufferIndexBinding>
#include <osg/BlendFunc>
#include <osgViewer/ViewerEventHandlers>

#include <osgEarth/MapNode>
#include <osgEarth/GDAL>
#include <osgEarth/TMS>
#include <osgEarth/ElevationLayer>
#include <osgEarth/GeoTransform>
#include <osgEarth/EarthManipulator>
#include <osgEarth/AutoClipPlaneHandler>

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
    float groundRadius = 6371.0f;
    float atmosphereRadius = 6471.0f;
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

struct AtmosphereParametersBuffer
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

}gAtmosphereParametersBuffer;
osg::ref_ptr<osg::UniformBufferBinding> gAtmosphereParametersUBB;

void calcAtmosphereParametersBuffer(const AtmosphereParameters& parameter, AtmosphereParametersBuffer& buffer)
{
    buffer.rayleighScatteringBase = parameter.rayleighScatteringCoeff * parameter.rayleighScatteringScale;
    buffer.mieScatteringBase = parameter.mieScatteringBase;
    buffer.ozoneAbsorptionBase = parameter.ozoneAbsorptionCoeff * parameter.ozoneAbsorptionScale;
    buffer.mieAbsorptionBase = parameter.mieAbsorptionBase;
    buffer.rayleighDensityH = parameter.rayleighDensityH;
    buffer.mieDensityH = parameter.mieDensityH;
    buffer.ozoneCenterHeight = parameter.ozoneCenterHeight;
    buffer.ozoneThickness = parameter.ozoneThickness;
    buffer.groundAlbedo = parameter.groundAlbedo;
    buffer.groundRadius = parameter.groundRadius;
    float altitude = osg::DegreesToRadians(parameter.solarAltitude);
    float azimuth = osg::DegreesToRadians(parameter.solarAzimuth);
    buffer.sunDirection = osg::Vec3(
        std::cos(altitude) * std::sin(azimuth),
        std::cos(altitude) * std::cos(azimuth),
        std::sin(altitude)
    );
    buffer.atmosphereRadius = parameter.atmosphereRadius;
    buffer.sunIntensity = parameter.sunColor * parameter.sunIntensity;
}

struct VolumetricCloudParameters
{
    osg::Vec3 albedo = osg::Vec3(1, 1, 1);
    osg::Vec3 extinction = osg::Vec3(0.005, 0.005, 0.005);
    float windSpeed = 0.0f;
    float cloudLayerBottomHeight = 5.0f;
    float cloudLayerThickness = 10.0f;
    float phaseG0 = 0.5;
    float phaseG1 = -0.5;
    float phaseBlend = 0.2;
    float msScatteringFactor = 0.2;
    float msExtinctionFactor = 0.5;
    float msPhaseFactor = 0.2;
    float cloudMapScaleFactor = 0.006;
    float basicNoiseScaleFactor = 0.08;
    float detailNoiseScaleFactor = 0.3;
}gVolumetricCloudParameters;

struct VolumetricCloudParametersBuffer
{
    osg::Vec3 albedo;
    float cloudLayerBottomRadius;

    osg::Vec3 extinction;
    float cloudLayerThickness;

    float windSpeed;
    float phaseG0;
    float phaseG1;
    float phaseBlend;

    float msScatteringFactor;
    float msExtinctionFactor;
    float msPhaseFactor;
    float cloudMapScaleFactor;

    float basicNoiseScaleFactor;
    float detailNoiseScaleFactor;
}gVolumetricCloudParametersBuffer;
osg::ref_ptr<osg::UniformBufferBinding> gVolumetricCloudParametersUBB;

void calcVolumetricCloudParametersBuffer(const VolumetricCloudParameters& parameter, VolumetricCloudParametersBuffer& buffer)
{
    buffer.albedo = parameter.albedo;
    buffer.cloudLayerBottomRadius = parameter.cloudLayerBottomHeight + 6371.0;
    buffer.extinction = parameter.extinction;
    buffer.cloudLayerThickness = parameter.cloudLayerThickness;
    buffer.windSpeed = parameter.windSpeed;
    buffer.phaseG0 = parameter.phaseG0;
    buffer.phaseG1 = parameter.phaseG1;
    buffer.phaseBlend = parameter.phaseBlend;
    buffer.msScatteringFactor = parameter.msScatteringFactor;
    buffer.msExtinctionFactor = parameter.msExtinctionFactor;
    buffer.msPhaseFactor = parameter.msPhaseFactor;
    buffer.cloudMapScaleFactor = parameter.cloudMapScaleFactor;
    buffer.basicNoiseScaleFactor = parameter.basicNoiseScaleFactor;
    buffer.detailNoiseScaleFactor = parameter.detailNoiseScaleFactor;
}

namespace xxx
{
    void ImGuiHandler::draw()
    {
        bool atmosphereNeedUpdate = false;
        ImGui::Begin("Test");
        if (ImGui::SliderFloat("Solar Altitude", &gAtmosphereParameters.solarAltitude, -90.0f, 90.0f))
            atmosphereNeedUpdate = true;
        if (ImGui::SliderFloat("Solar Azimuth", &gAtmosphereParameters.solarAzimuth, 0.0f, 360.0f))
            atmosphereNeedUpdate = true;
        if (ImGui::ColorEdit3("Sun Color", &gAtmosphereParameters.sunColor.x()))
            atmosphereNeedUpdate = true;
        if (ImGui::SliderFloat("Sun Intensity", &gAtmosphereParameters.sunIntensity, 0.0f, 20.0f))
            atmosphereNeedUpdate = true;
        if (ImGui::SliderFloat("Mie Scattering Base", &gAtmosphereParameters.mieScatteringBase, 0.0f, 5.0f))
            atmosphereNeedUpdate = true;
        if (ImGui::SliderFloat("Mie Absorption Base", &gAtmosphereParameters.mieAbsorptionBase, 0.0f, 5.0f))
            atmosphereNeedUpdate = true;

        bool volumetricCloudNeedUpdate = false;
        if (ImGui::ColorEdit3("Albedo", &gVolumetricCloudParameters.albedo.x()))
            volumetricCloudNeedUpdate = true;
        if (ImGui::DragFloat("Extinction", &gVolumetricCloudParameters.extinction.x(), 0.0005, 0.0f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic))
        {
            gVolumetricCloudParameters.extinction.y() = gVolumetricCloudParameters.extinction.x();
            gVolumetricCloudParameters.extinction.z() = gVolumetricCloudParameters.extinction.x();
            volumetricCloudNeedUpdate = true;
        }
        if (ImGui::SliderFloat("Cloud Layer Bottom Height", &gVolumetricCloudParameters.cloudLayerBottomHeight, 0.0f, 10.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("Cloud Layer Thickness", &gVolumetricCloudParameters.cloudLayerThickness, 0.0f, 30.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("Wind Speed", &gVolumetricCloudParameters.windSpeed, 0.0f, 10.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("PhaseG0", &gVolumetricCloudParameters.phaseG0, -1.0f, 1.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("PhaseG1", &gVolumetricCloudParameters.phaseG1, -1.0f, 1.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("PhaseBlend", &gVolumetricCloudParameters.phaseBlend, 0.0f, 1.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("MS Scattering Factor", &gVolumetricCloudParameters.msScatteringFactor, 0.0f, 1.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("MS Extinction Factor", &gVolumetricCloudParameters.msExtinctionFactor, 0.0f, 1.0f))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("MS Phase Factor", &gVolumetricCloudParameters.msPhaseFactor, 0.0f, 1.0f))
            volumetricCloudNeedUpdate = true;

        if (ImGui::SliderFloat("CloudMap Scale Factor", &gVolumetricCloudParameters.cloudMapScaleFactor, 0.0f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("BasicNoise Scale Factor", &gVolumetricCloudParameters.basicNoiseScaleFactor, 0.0f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic))
            volumetricCloudNeedUpdate = true;
        if (ImGui::SliderFloat("DetailNoise Scale Factor", &gVolumetricCloudParameters.detailNoiseScaleFactor, 0.0f, 1.0f, "%.6f", ImGuiSliderFlags_Logarithmic))
            volumetricCloudNeedUpdate = true;

        if (atmosphereNeedUpdate)
        {
            calcAtmosphereParametersBuffer(gAtmosphereParameters, gAtmosphereParametersBuffer);
            osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gAtmosphereParametersUBB->getBufferData());
            buffer->assign((float*)&gAtmosphereParametersBuffer, (float*)(&gAtmosphereParametersBuffer + 1));
            buffer->dirty();
        }
        if (volumetricCloudNeedUpdate)
        {
            calcVolumetricCloudParametersBuffer(gVolumetricCloudParameters, gVolumetricCloudParametersBuffer);
            osg::FloatArray* buffer = static_cast<osg::FloatArray*>(gVolumetricCloudParametersUBB->getBufferData());
            buffer->assign((float*)&gVolumetricCloudParametersBuffer, (float*)(&gVolumetricCloudParametersBuffer + 1));
            buffer->dirty();
        }

        ImGui::End();
    }
}

struct DoubleViewData
{
    osg::Matrixd viewMatrix;
    osg::Matrixd inverseViewMatrix;
    osg::Matrixd projectionMatrix;
    osg::Matrixd inverseProjectionMatrix;
}gDoubleViewData;

class InputPassCameraPreDrawCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator() (osg::RenderInfo& renderInfo) const
    {
        gDoubleViewData.viewMatrix = renderInfo.getState()->getInitialViewMatrix();
        gDoubleViewData.inverseViewMatrix = renderInfo.getState()->getInitialInverseViewMatrix();
        gDoubleViewData.projectionMatrix = renderInfo.getCurrentCamera()->getProjectionMatrix();
        gDoubleViewData.inverseProjectionMatrix = osg::Matrixd::inverse(gDoubleViewData.projectionMatrix);
        gViewData.viewMatrix = gDoubleViewData.viewMatrix;
        gViewData.inverseViewMatrix = gDoubleViewData.inverseViewMatrix;
        gViewData.projectionMatrix = gDoubleViewData.projectionMatrix;
        gViewData.inverseProjectionMatrix = gDoubleViewData.inverseProjectionMatrix;
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

class ReconstructionPassCallback : public osg::Camera::DrawCallback
{
    osg::Matrixd _prevFrameInverseViewMatrix;
    osg::Matrixd _prevFrameViewProjectionMatrix;
    osg::ref_ptr<osg::Uniform> _reprojectionMatrixUniform;
public:
    ReconstructionPassCallback(osg::Camera* camera)
    {
        _reprojectionMatrixUniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "uReprojectionMatrix");
        camera->getOrCreateStateSet()->addUniform(_reprojectionMatrixUniform);
    }

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        osg::Matrixf reprojectionMatrix = gDoubleViewData.inverseProjectionMatrix * gDoubleViewData.inverseViewMatrix * _prevFrameViewProjectionMatrix;
        _reprojectionMatrixUniform->set(reprojectionMatrix);
        const_cast<osg::Matrixd&>(_prevFrameViewProjectionMatrix) = gDoubleViewData.viewMatrix * gDoubleViewData.projectionMatrix;
        const_cast<osg::Matrixd&>(_prevFrameInverseViewMatrix) = gDoubleViewData.inverseViewMatrix;
    }
};

#define FAST_RENDER 1
#define CUSTOM_CLOUD_MAP 0

int main()
{
    osgEarth::initialize();
    osg::ref_ptr<osg::FloatArray> viewDataBuffer = new osg::FloatArray((float*)&gViewData, (float*)(&gViewData + 1));
    osg::ref_ptr<osg::UniformBufferObject> viewDataUBO = new osg::UniformBufferObject;
    viewDataBuffer->setBufferObject(viewDataUBO);
    gViewDataUBB = new osg::UniformBufferBinding(0, viewDataBuffer, 0, sizeof(ViewData));

    calcAtmosphereParametersBuffer(gAtmosphereParameters, gAtmosphereParametersBuffer);
    osg::ref_ptr<osg::FloatArray> atmosphereParametersArray = new osg::FloatArray((float*)&gAtmosphereParametersBuffer, (float*)(&gAtmosphereParametersBuffer + 1));
    osg::ref_ptr<osg::UniformBufferObject> atmosphereParametersUBO = new osg::UniformBufferObject;
    atmosphereParametersArray->setBufferObject(atmosphereParametersUBO);
    gAtmosphereParametersUBB = new osg::UniformBufferBinding(1, atmosphereParametersArray, 0, sizeof(AtmosphereParametersBuffer));

    calcVolumetricCloudParametersBuffer(gVolumetricCloudParameters, gVolumetricCloudParametersBuffer);
    osg::ref_ptr<osg::FloatArray> volumetricCloudParametersArray = new osg::FloatArray((float*)&gVolumetricCloudParametersBuffer, (float*)(&gVolumetricCloudParametersBuffer + 1));
    osg::ref_ptr<osg::UniformBufferObject> volumetricCloudParametersUBO = new osg::UniformBufferObject;
    volumetricCloudParametersArray->setBufferObject(volumetricCloudParametersUBO);
    gVolumetricCloudParametersUBB = new osg::UniformBufferBinding(2, volumetricCloudParametersArray, 0, sizeof(VolumetricCloudParametersBuffer));

    const int width = 1920, height = 1080;
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
    viewer->setRealizeOperation(new xxx::ImGuiInitOperation);
    viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    camera->setGraphicsContext(gc);
    camera->setViewport(0, 0, width, height);
    camera->setProjectionMatrixAsPerspective(60.0, double(width) / double(height), 0.1, 1000.0);

#if CUSTOM_CLOUD_MAP
    osg::ref_ptr<osg::Texture2D> cloudMapTexture = new osg::Texture2D(osgDB::readImageFile(TEMP_DIR "CloudMap.png"));
    cloudMapTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    cloudMapTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    cloudMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    cloudMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
#else
    osg::ref_ptr<osg::Texture2D> cloudMapTexture = new osg::Texture2D;
    cloudMapTexture->setTextureSize(512, 512);
    cloudMapTexture->setInternalFormat(GL_RGBA16F);
    cloudMapTexture->setSourceFormat(GL_RGBA);
    cloudMapTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    cloudMapTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    cloudMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    cloudMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> cloudMapImage = new osg::BindImageTexture(0, cloudMapTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F);
    osg::ref_ptr<osg::Program> cloudMapProgram = new osg::Program;
    cloudMapProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/CloudMap.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> cloudMapDispatch = new osg::DispatchCompute(16, 16, 1);
    cloudMapDispatch->setCullingActive(false);
    cloudMapDispatch->getOrCreateStateSet()->setAttribute(cloudMapProgram, osg::StateAttribute::ON);
    cloudMapDispatch->getOrCreateStateSet()->setAttribute(cloudMapImage, osg::StateAttribute::ON);
    cloudMapDispatch->setCullCallback(new DrawTimesCullCallback(1));
    rootGroup->addChild(cloudMapDispatch);
#endif

    osg::ref_ptr<osg::Program> cloudMapSDFProgram = new osg::Program;
    cloudMapSDFProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/CloudMapSDF.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> cloudMapSDFDispatch = new osg::DispatchCompute(16, 16, 1);
    cloudMapSDFDispatch->setCullingActive(false);
    cloudMapSDFDispatch->getOrCreateStateSet()->setAttribute(cloudMapSDFProgram, osg::StateAttribute::ON);
    cloudMapSDFDispatch->getOrCreateStateSet()->setAttribute(cloudMapImage, osg::StateAttribute::ON);
    cloudMapSDFDispatch->setCullCallback(new DrawTimesCullCallback(1));
    rootGroup->addChild(cloudMapSDFDispatch);

    osg::ref_ptr<osg::Texture3D> basicNoiseTexture = new osg::Texture3D;
    basicNoiseTexture->setTextureSize(128, 128, 128);
    basicNoiseTexture->setInternalFormat(GL_RGBA8);
    basicNoiseTexture->setSourceFormat(GL_RGBA);
    basicNoiseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    basicNoiseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    basicNoiseTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
    basicNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    basicNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> basicNoiseImage = new osg::BindImageTexture(0, basicNoiseTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> basicNoiseProgram = new osg::Program;
    basicNoiseProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/BasicNoise.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> basicNoiseDispatch = new osg::DispatchCompute(32, 32, 2);
    basicNoiseDispatch->setCullingActive(false);
    basicNoiseDispatch->getOrCreateStateSet()->setAttribute(basicNoiseProgram, osg::StateAttribute::ON);
    basicNoiseDispatch->getOrCreateStateSet()->setAttribute(basicNoiseImage, osg::StateAttribute::ON);
    basicNoiseDispatch->setCullCallback(new DrawTimesCullCallback(1));
    rootGroup->addChild(basicNoiseDispatch);

    osg::ref_ptr<osg::Texture3D> detailNoiseTexture = new osg::Texture3D;
    detailNoiseTexture->setTextureSize(32, 32, 32);
    detailNoiseTexture->setInternalFormat(GL_RGBA8);
    detailNoiseTexture->setSourceFormat(GL_RGBA);
    detailNoiseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    detailNoiseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    detailNoiseTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);
    detailNoiseTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    detailNoiseTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    osg::ref_ptr<osg::BindImageTexture> detailNoiseImage = new osg::BindImageTexture(0, detailNoiseTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA8);
    osg::ref_ptr<osg::Program> detailNoiseProgram = new osg::Program;
    detailNoiseProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "VolumetricCloud/DetailNoise.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> detailNoiseDispatch = new osg::DispatchCompute(8, 8, 1);
    detailNoiseDispatch->setCullingActive(false);
    detailNoiseDispatch->getOrCreateStateSet()->setAttribute(detailNoiseProgram, osg::StateAttribute::ON);
    detailNoiseDispatch->getOrCreateStateSet()->setAttribute(detailNoiseImage, osg::StateAttribute::ON);
    detailNoiseDispatch->setCullCallback(new DrawTimesCullCallback(1));
    rootGroup->addChild(detailNoiseDispatch);

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
    transmittanceLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereParametersUBB, osg::StateAttribute::ON);
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
    multiScatteringLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereParametersUBB, osg::StateAttribute::ON);
    multiScatteringLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
    multiScatteringLutDispatch->getOrCreateStateSet()->setTextureAttribute(0, transmittanceLutTexture, osg::StateAttribute::ON);
    rootGroup->addChild(multiScatteringLutDispatch);

    osg::ref_ptr<osg::Texture2D> distantSkyLightLutTexture = new osg::Texture2D;
    distantSkyLightLutTexture->setTextureSize(1, 1);
    distantSkyLightLutTexture->setInternalFormat(GL_R11F_G11F_B10F);
    distantSkyLightLutTexture->setSourceFormat(GL_RGB);
    distantSkyLightLutTexture->setSourceType(GL_HALF_FLOAT);
    distantSkyLightLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    distantSkyLightLutTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    osg::ref_ptr<osg::BindImageTexture> distantSkyLightLutImage = new osg::BindImageTexture(0, distantSkyLightLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_R11F_G11F_B10F);
    osg::ref_ptr<osg::Program> distantSkyLightLutProgram = new osg::Program;
    distantSkyLightLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "Atmosphere/DistantSkyLightLut.comp.glsl"));
    osg::ref_ptr<osg::DispatchCompute> distantSkyLightLutDispatch = new osg::DispatchCompute(1, 1, 1);
    distantSkyLightLutDispatch->setCullingActive(false);
    distantSkyLightLutDispatch->getOrCreateStateSet()->setAttribute(distantSkyLightLutProgram, osg::StateAttribute::ON);
    distantSkyLightLutDispatch->getOrCreateStateSet()->setAttribute(distantSkyLightLutImage, osg::StateAttribute::ON);
    distantSkyLightLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereParametersUBB, osg::StateAttribute::ON);
    distantSkyLightLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
    distantSkyLightLutDispatch->getOrCreateStateSet()->setTextureAttribute(0, transmittanceLutTexture, osg::StateAttribute::ON);
    distantSkyLightLutDispatch->getOrCreateStateSet()->addUniform(new osg::Uniform("uMultiScatteringLutTexture", 1));
    distantSkyLightLutDispatch->getOrCreateStateSet()->setTextureAttribute(1, multiScatteringLutTexture, osg::StateAttribute::ON);
    rootGroup->addChild(distantSkyLightLutDispatch);

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
    skyViewLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereParametersUBB, osg::StateAttribute::ON);
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
    aerialPerspectiveLutDispatch->getOrCreateStateSet()->setAttribute(gAtmosphereParametersUBB, osg::StateAttribute::ON);
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

    osgEarth::Map* map = new osgEarth::Map;
    osgEarth::TMSImageLayer* tmsImageLayer = new osgEarth::TMSImageLayer;
    tmsImageLayer->setURL(R"(C:\Users\admin\Downloads\wenhualou\tms.xml)");
    map->addLayer(tmsImageLayer);

    //osgEarth::GDALElevationLayer* elevationLayer = new osgEarth::GDALElevationLayer;
    //elevationLayer->setURL(R"(C:\Users\Public\Nwt\cache\recv\lhc\30m.tif)");
    //map->addLayer(elevationLayer);

    osgEarth::ProfileOptions po;
    po.srsString() = "+proj=latlong +a=6371000 +b=6371000 +towgs84=0,0,0,0,0,0,0";
    map->setProfile(osgEarth::Profile::create(po));
    osgEarth::MapNode* mapNode = new osgEarth::MapNode(map);
    mapNode->getTerrainOptions().setMinTileRangeFactor(3.0);
    rootGroup->addChild(mapNode);

    mapNode->getOrCreateStateSet()->setAttributeAndModes(gAtmosphereParametersUBB);
    osgEarth::VirtualProgram* vp = osgEarth::VirtualProgram::getOrCreate(mapNode->getOrCreateStateSet());
    static const char* fragSource = R"(
		#version 430
		vec4 vp_Vertex;
		layout(location = 0) out vec4 gColor;
        layout (std140, binding = 1) uniform AtmosphereParameters
        {
            vec3 uRayleighScatteringBase;
            float uMieScatteringBase;
            vec3 uOzoneAbsorptionBase;
            float uMieAbsorptionBase;
            float uRayleighDensityH;
            float uMieDensityH;
            float uOzoneHeight;
            float uOzoneThickness;
            vec3 uGroundAlbedo;
            float uGroundRadius;
            vec3 uSunDirection;
            float uAtmosphereRadius;
            vec3 uSunIntensity;
        };
		uniform mat4 osg_ViewMatrixInverse;
		void oe_custom_fragment(inout vec4 color)
		{
			vec3 normal = normalize((osg_ViewMatrixInverse * vp_Vertex).xyz);
			float ndl = clamp(max(dot(normal, uSunDirection), 0.0) + 0.1, 0.0, 1.0);
			gColor = vec4(pow(color.rgb, vec3(2.2)) * ndl, color.a);
		}
	)";
    vp->setFunction("oe_custom_fragment", fragSource, osgEarth::ShaderComp::LOCATION_FRAGMENT_OUTPUT);

    using BufferType = xxx::Pipeline::Pass::BufferType;
    osg::ref_ptr<xxx::Pipeline> pipeline = new xxx::Pipeline(viewer, gc);
    osg::ref_ptr<xxx::Pipeline::Pass> inputPass = pipeline->addInputPass("Input", 0xFFFFFFFF, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    inputPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    inputPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT, false, osg::Texture::NEAREST, osg::Texture::NEAREST);
    inputPass->getCamera()->setPreDrawCallback(new InputPassCameraPreDrawCallback);
    inputPass->getCamera()->setCullCallback(new osgEarth::AutoClipPlaneCullCallback(mapNode));

    osg::ref_ptr<osg::Shader> screenQuadVertexShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");

    osg::ref_ptr<osg::Program> atmosphereProgram = new osg::Program;
    atmosphereProgram->addShader(screenQuadVertexShader);
    atmosphereProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Atmosphere/RayMarching.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> atmospherePass = pipeline->addWorkPass("Atmosphere", atmosphereProgram, 0);
    atmospherePass->attach(BufferType::COLOR_BUFFER0, inputPass->getBufferTexture(BufferType::COLOR_BUFFER0));
    atmospherePass->applyTexture(transmittanceLutTexture, "uTransmittanceLutTexture", 0);
    atmospherePass->applyTexture(multiScatteringLutTexture, "uMultiScatteringLutTexture", 1);
    atmospherePass->applyTexture(skyViewLutTexture, "uSkyViewLutTexture", 2);
    atmospherePass->applyTexture(aerialPerspectiveLutTexture, "uAerialPerspectiveLutTexture", 3);
    atmospherePass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uSceneDepthTexture", 4);
    atmospherePass->setAttribute(gViewDataUBB);
    atmospherePass->setMode(GL_BLEND);
    atmospherePass->setAttribute(new osg::BlendFunc(GL_ONE, GL_SRC_ALPHA));

    osg::ref_ptr<osg::Image> blueNoiseImage = osgDB::readImageFile(TEMP_DIR "BlueNoise.png");
    osg::ref_ptr<osg::Texture2D> blueNoiseTexture = new osg::Texture2D(blueNoiseImage);
    blueNoiseTexture->setInternalFormat(GL_R8);
    blueNoiseTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    blueNoiseTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

#if FAST_RENDER
    osg::Vec2f cloudRenderScale = osg::Vec2(0.25, 0.25);
#else
    osg::Vec2f cloudRenderScale = osg::Vec2(1.0, 1.0);
#endif
    osg::ref_ptr<osg::Program> volumetricCloudProgram = new osg::Program;
    volumetricCloudProgram->addShader(screenQuadVertexShader);
    volumetricCloudProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "VolumetricCloud/RayMarching3.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> volumetricCloudPass = pipeline->addWorkPass("VolumetricCloud", volumetricCloudProgram, GL_COLOR_BUFFER_BIT, false, cloudRenderScale);
    volumetricCloudPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA32F);
    volumetricCloudPass->attach(BufferType::COLOR_BUFFER1, GL_R16F);
    volumetricCloudPass->applyTexture(inputPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uDepthTexture", 0);
    volumetricCloudPass->applyTexture(basicNoiseTexture, "uBasicNoiseTexture", 1);
    volumetricCloudPass->applyTexture(detailNoiseTexture, "uDetailNoiseTexture", 2);
    volumetricCloudPass->applyTexture(cloudMapTexture, "uCloudMapTexture", 3);
    volumetricCloudPass->applyTexture(transmittanceLutTexture, "uTransmittanceLutTexture", 4);
    volumetricCloudPass->applyTexture(distantSkyLightLutTexture, "uDistantSkyLightLutTexture", 5);
    volumetricCloudPass->applyTexture(aerialPerspectiveLutTexture, "uAerialPerspectiveLutTexture", 6);
    volumetricCloudPass->applyTexture(blueNoiseTexture, "uBlueNoiseTexture", 7);
    volumetricCloudPass->setAttribute(gViewDataUBB);
    volumetricCloudPass->setAttribute(gAtmosphereParametersUBB);
    volumetricCloudPass->setAttribute(gVolumetricCloudParametersUBB);
#if FAST_RENDER
    volumetricCloudPass->getCamera()->getOrCreateStateSet()->setDefine("FAST_RENDER", "1");
#else
    volumetricCloudPass->getCamera()->getOrCreateStateSet()->setDefine("FAST_RENDER", "0");
#endif

    osg::Viewport* viewport = pipeline->getView()->getCamera()->getViewport();
    osg::ref_ptr<osg::Texture2D> historyCloudColorTexture = new osg::Texture2D;
    historyCloudColorTexture->setTextureSize(viewport->x(), viewport->y());
    historyCloudColorTexture->setInternalFormat(GL_RGBA16F);

    osg::ref_ptr<osg::Texture2D> historyCloudDistanceTexture = new osg::Texture2D;
    historyCloudDistanceTexture->setTextureSize(viewport->x(), viewport->y());
    historyCloudDistanceTexture->setInternalFormat(GL_R16F);

#if FAST_RENDER
    osg::ref_ptr<osg::Program> cloudReconstructionProgram = new osg::Program;
    cloudReconstructionProgram->addShader(screenQuadVertexShader);
    cloudReconstructionProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "VolumetricCloud/Reconstruction.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> cloudReconstructionPass = pipeline->addWorkPass("CloudReconstruction", cloudReconstructionProgram, GL_COLOR_BUFFER_BIT);
    cloudReconstructionPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    cloudReconstructionPass->attach(BufferType::COLOR_BUFFER1, GL_R16F);

    cloudReconstructionPass->applyTexture(volumetricCloudPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCurrentCloudColorTexture", 0);
    cloudReconstructionPass->applyTexture(volumetricCloudPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uCurrentCloudDistanceTexture", 1);
    cloudReconstructionPass->applyTexture(historyCloudColorTexture, "uHistoryCloudColorTexture", 2);
    cloudReconstructionPass->applyTexture(historyCloudDistanceTexture, "uHistoryCloudDistanceTexture", 3);
    cloudReconstructionPass->setAttribute(gViewDataUBB);

    cloudReconstructionPass->getCamera()->setPreDrawCallback(new ReconstructionPassCallback(cloudReconstructionPass->getCamera()));

    osg::ref_ptr<osg::Program> copyColorProgram = new osg::Program;
    copyColorProgram->addShader(screenQuadVertexShader);
    copyColorProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CopyColor2.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> copyCloudPass = pipeline->addWorkPass("CopyCloud", copyColorProgram, GL_COLOR_BUFFER_BIT);
    copyCloudPass->attach(BufferType::COLOR_BUFFER0, historyCloudColorTexture);
    copyCloudPass->attach(BufferType::COLOR_BUFFER1, historyCloudDistanceTexture);
    copyCloudPass->applyTexture(cloudReconstructionPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
    copyCloudPass->applyTexture(cloudReconstructionPass->getBufferTexture(BufferType::COLOR_BUFFER1), "uColor1Texture", 1);

    osg::ref_ptr<osg::Texture2D> historyColorTexture = new osg::Texture2D;
    historyColorTexture->setTextureSize(viewport->x(), viewport->y());
    historyColorTexture->setInternalFormat(GL_RGBA16F);

    osg::ref_ptr<osg::Program> taaProgram = new osg::Program;
    taaProgram->addShader(screenQuadVertexShader);
    taaProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/TAA.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> taaPass = pipeline->addWorkPass("TAA", taaProgram, 0);
    taaPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
    taaPass->applyTexture(cloudReconstructionPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCurrentColorTexture", 0);
    taaPass->applyTexture(historyColorTexture, "uHistoryColorTexture", 1);

    osg::ref_ptr<xxx::Pipeline::Pass> copyColorPass = pipeline->addWorkPass("CopyColor", copyColorProgram, GL_COLOR_BUFFER_BIT);
    copyColorPass->attach(BufferType::COLOR_BUFFER0, historyColorTexture);
    copyColorPass->applyTexture(taaPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uColorTexture", 0);
#endif

    osg::ref_ptr<osg::Program> colorGradingProgram = new osg::Program;
    colorGradingProgram->addShader(screenQuadVertexShader);
    colorGradingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Common/CombineAtmosphereAndCloud.frag.glsl"));
    osg::ref_ptr<xxx::Pipeline::Pass> displayPass = pipeline->addDisplayPass("Display", colorGradingProgram);
    displayPass->applyTexture(atmospherePass->getBufferTexture(BufferType::COLOR_BUFFER0), "uAtmosphereColorTexture", 0);
    displayPass->setAttribute(gViewDataUBB);
    displayPass->setAttribute(gAtmosphereParametersUBB);
#if FAST_RENDER
    displayPass->applyTexture(taaPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCloudColorTexture", 1);
#else
    displayPass->applyTexture(volumetricCloudPass->getBufferTexture(BufferType::COLOR_BUFFER0), "uCloudColorTexture", 1);
#endif

    //viewer->setCameraManipulator(new ControllerManipulator(1000));
    viewer->setCameraManipulator(new osgEarth::EarthManipulator);
    viewer->addEventHandler(new xxx::ImGuiHandler(viewer, displayPass->getCamera()));
    viewer->addEventHandler(new osgViewer::StatsHandler);

    viewer->realize();
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
