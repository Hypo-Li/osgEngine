#version 460 core
#extension GL_GOOGLE_include_directive : enable
in vec2 uv;
out vec4 fragData[2];

uniform sampler2D uDepthTexture;
uniform sampler3D uBasicNoiseTexture;
uniform sampler3D uDetailNoiseTexture;
uniform sampler2D uCloudMapTexture;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uDistantSkyLightLutTexture;
uniform sampler3D uAerialPerspectiveLutTexture;
uniform sampler2D uBlueNoiseTexture;
uniform float osg_FrameTime;
uniform uint osg_FrameNumber;
uniform vec4 uResolution;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

layout (std140, binding = 1) uniform AtmosphereParameters
{
    vec3 uRayleighScatteringBase;
    float uMieScatteringBase;
    vec3 uOzoneAbsorptionBase;
    float uMieAbsorptionBase;
    float uRayleighDensityH;
    float uMieDensityH;
    float uOzoneCenterHeight;
    float uOzoneThickness;
    vec3 uGroundAlbedo;
    float uGroundRadius;
    vec3 uSunDirection;
    float uAtmosphereRadius;
    vec3 uSunIntensity;
};

#define MS_COUNT 2

layout (std140, binding = 2) uniform VolumetricCloudParameters
{
    vec3 uAlbedo;
    float uCloudLayerBottomRadiusKm;
    vec3 uExtinction;
    float uCloudLayerThicknessKm;
    float uWindSpeed;
    float uPhaseG0;
    float uPhaseG1;
    float uPhaseBlend;
    float uMsScattFactor;
    float uMsExtinFactor;
    float uMsPhaseFactor;
    float uCloudMapScaleFactor;
    float uBasicNoiseScaleFactor;
    float uDetailNoiseScaleFactor;
};

//const vec3 uAlbedo = vec3(1.0);
//const vec3 uExtinction = vec3(0.005);
const vec3 uWindDirection = vec3(0.8728715181, 0.2182178795, 0.4364357591);
//const float uWindSpeed = 2.0;
//const float uCloudLayerBottomRadiusKm = 6365.0;
//const float uCloudLayerThicknessKm = 10.0;
float uCloudLayerTopRadiusKm = uCloudLayerBottomRadiusKm + uCloudLayerThicknessKm;
const float uTracingStartMaxDistanceKm = 350.0;
const float uTracingMaxDistanceKm = 50.0;
const vec3 uStopTracingTransmittanceThreshold = vec3(0.005);
const uint uCloudSampleCountMin = 2;
const uint uCloudSampleCountMax = 96;
const float uSampleCountPerKm = 15.0;
const float uCloudShadowTracingMaxDistanceKm = 15.0;
const uint uCloudShadowSampleCountMax = 4;
//const float uMsScattFactor = 0.2;
//const float uMsExtinFactor = 0.2;
//const float uMsPhaseFactor = 0.2;
//const float uPhaseG0 = 0.5;
//const float uPhaseG1 = -0.5;
//const float uPhaseBlend = 0.2;

vec3 gDistantSkyLight = vec3(0.0);
vec2 gFullResCoord = vec2(0.0);

struct ParticipatingMediaPhaseContext
{
    float phase0[MS_COUNT];
};

const float PI = 3.1415926535897932;
const float E = 2.718281828459045;

float sampleCloudDensity(vec3 worldPos, vec4 weatherData, float h)
{
    const float cloudTopOffset = 10.0;
    worldPos += h * uWindDirection * cloudTopOffset;
    worldPos += uWindDirection * osg_FrameTime * uWindSpeed;

    vec4 basicNoise = textureLod(uBasicNoiseTexture, worldPos * uBasicNoiseScaleFactor, 0);
    float basicFbm = dot(basicNoise.gba, vec3(0.625, 0.25, 0.125));
    float basicCloud = remap(basicNoise.r, basicFbm - 1.0, 1.0, 0.0, 1.0);

    vec4 detailNoise = textureLod(uDetailNoiseTexture, worldPos * uDetailNoiseScaleFactor, 0);
    float detailFbm = dot(detailNoise.rgb, vec3(0.625, 0.25, 0.125));
    float detailCloud = 0.35 * pow(E, -0.75) * mix(detailFbm, 1.0 - detailFbm, saturate(h * 5.0));

    float bottomShape = saturate(remap(h, 0, 0.07, 0.0, 1.0));
    float topShape = saturate(remap(h, 0.2, 1.0, 1.0, 0.0));
    float shape = bottomShape * topShape;
    const float anvil = 0.0;
    shape = pow(shape, saturate(remap(h, 0.65, 0.95, 1.0, 1.0 - anvil)));
    float density = saturate(remap(basicCloud * shape, 1.0 - weatherData.r, 1.0, 0.0, 1.0));
    density = remap(density, detailCloud, 1.0, 0.0, 1.0);
    return density;
}