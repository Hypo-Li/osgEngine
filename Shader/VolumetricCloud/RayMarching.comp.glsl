#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 32, local_size_y = 32) in;
layout (rgba16f, binding = 0) uniform image2D uVolumetricCloudImage;

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

const float PI = 3.1415926535897932;
const float E = 2.718281828459045;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2 saturate(vec2 x) { return clamp(x, vec2(0.0), vec2(1.0)); }
vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }
vec4 saturate(vec4 x) { return clamp(x, vec4(0.0), vec4(1.0)); }

float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + (saturate((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin));
}

//#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
vec3 getWorldPos(vec3 pos)
{
#ifdef PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
    vec3 worldPos = pos / 1000.0 + vec3(0.0, 0.0, uGroundRadius);
#else
    vec3 worldPos = pos / 1000.0;
#endif
    float viewHeight = length(worldPos);
    vec3 upVector = worldPos / viewHeight;
    return upVector * max(viewHeight, uGroundRadius + 0.005);
}

bool rayIntersectSphereSolution(in vec3 ro, in vec3 rd, in vec4 sphere, out vec2 solutions)
{
    vec3 localPosition = ro - sphere.xyz;
    float localPositionSqr = dot(localPosition, localPosition);

    vec3 quadraticCoef;
    quadraticCoef.x = dot(rd, rd);
	quadraticCoef.y = 2 * dot(rd, localPosition);
	quadraticCoef.z = localPositionSqr - sphere.w * sphere.w;

	float discriminant = quadraticCoef.y * quadraticCoef.y - 4 * quadraticCoef.x * quadraticCoef.z;

	if (discriminant >= 0)
	{
		float sqrtDiscriminant = sqrt(discriminant);
		solutions = (-quadraticCoef.y + vec2(-1, 1) * sqrtDiscriminant) / (2 * quadraticCoef.x);
		return true;
	}

	return false;
}

// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float hgPhase(float g, float cosTheta)
{
	float numer = 1.0 - g * g;
	float denom = 1.0 + g * g + 2.0 * g * cosTheta;
	return numer / (4.0 * PI * denom * sqrt(denom));
}

float dualLobPhase(float g0, float g1, float w, float cosTheta)
{
	return mix(hgPhase(g0, cosTheta), hgPhase(g1, cosTheta), w);
}

float getBlueNoise(uvec2 screenCoord, uint frameIndex)
{
    uvec3 wrappedCoord = uvec3(screenCoord, frameIndex) & uvec3(127, 127, 63);
    uvec3 textureCoord = uvec3(wrappedCoord.x, wrappedCoord.z * 128 + wrappedCoord.y, 0);
    return texelFetch(uBlueNoiseTexture, ivec2(textureCoord.xy), 0).r;
}

float pseudoRandom(vec2 xy)
{
	vec2 pos = fract(xy / 128.0) * 128.0 + vec2(-64.340622, -72.465622);
	return fract(dot(pos.xyx * pos.xyy, vec3(20.390625, 60.703125, 2.4281209)));
}

float interleavedGradientNoise(vec2 uv, float frameId)
{
	uv += frameId * (vec2(47, 17) * 0.695);
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

#define MS_COUNT 2

struct ParticipatingMediaPhaseContext
{
    float phase0[MS_COUNT];
};

ParticipatingMediaPhaseContext setupParticipatingMediaPhaseContext(float basePhase0, float msPhaseFactor)
{
    ParticipatingMediaPhaseContext pmpc;
    pmpc.phase0[0] = basePhase0;
    for (uint ms = 1; ms < MS_COUNT; ++ms)
    {
        pmpc.phase0[ms] = mix(0.25 * PI, pmpc.phase0[0], msPhaseFactor);
        msPhaseFactor *= msPhaseFactor;
    }
    return pmpc;
}

struct ParticipatingMediaContext
{
    vec3 scatteringCoeff[MS_COUNT];
    vec3 extinctionCoeff[MS_COUNT];
    vec3 transmittanceToLight0[MS_COUNT];
};

ParticipatingMediaContext setupParticipatingMediaContext(vec3 baseAlbedo, vec3 baseExtinctionCoeff, float msSFactor, float msEFactor, vec3 initialTransmittanceToLight0)
{
    const vec3 scatteringCoeff = baseAlbedo * baseExtinctionCoeff;

    ParticipatingMediaContext pmc;
    pmc.scatteringCoeff[0] = scatteringCoeff;
    pmc.extinctionCoeff[0] = baseExtinctionCoeff;
    pmc.transmittanceToLight0[0] = initialTransmittanceToLight0;

    for (uint ms = 1; ms < MS_COUNT; ++ms)
    {
        pmc.scatteringCoeff[ms] = pmc.scatteringCoeff[ms - 1] * msSFactor;
        pmc.extinctionCoeff[ms] = pmc.extinctionCoeff[ms - 1] * msEFactor;
        msSFactor *= msSFactor;
        msEFactor *= msEFactor;

        pmc.transmittanceToLight0[ms] = initialTransmittanceToLight0;
    }
    return pmc;
}

void transmittanceLutParametersToUV(in float viewHeight, in float viewZenithCos, out vec2 uv)
{
    float H = sqrt(max(0.0f, uAtmosphereRadius * uAtmosphereRadius - uGroundRadius * uGroundRadius));
    float rho = sqrt(max(0.0f, viewHeight * viewHeight - uGroundRadius * uGroundRadius));
    float discriminant = viewHeight * viewHeight * (viewZenithCos * viewZenithCos - 1.0) + uAtmosphereRadius * uAtmosphereRadius;
    float d = max(0.0, (-viewHeight * viewZenithCos + sqrt(discriminant))); // Distance to atmosphere boundary
    float dMin = uAtmosphereRadius - viewHeight;
    float dMax = rho + H;
    float u = (d - dMin) / (dMax - dMin);
    float v = rho / H;
    uv = vec2(u, v); 
}

#define SLICE_COUNT 16.0
#define KM_PER_SLICE 8.0

float aerialPerspectiveDepthToSlice(float depth)
{
    return depth * (1.0 / KM_PER_SLICE);
}

vec4 sampleWeather(vec3 worldPos)
{
    return textureLod(uCloudMapTexture, worldPos.xy * uCloudMapScaleFactor, 0.0);
}

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
    float detailModifier = mix(detailFbm, 1.0 - detailFbm, saturate(h * 5.0));
    detailModifier *= 0.35 * exp(-0.75);

    const float anvil = 0.0;
    float shape = saturate(remap(h, 0.0, 0.07, 0.0, 1.0));
    shape *= saturate(remap(h, 0.2, 1.0, 1.0, 0.0));
    shape = pow(shape, saturate(remap(h, 0.65, 0.95, 1.0, 1.0 - anvil)));

    float density = h;
    density *= saturate(remap(h, 0.0, 0.2, 0.0, 1.0)) * 2;
    density *= mix(1.0, saturate(remap(pow(h, 0.5), 0.4, 0.95, 1.0, 0.2)), anvil);
    density *= saturate(remap(h, 0.9, 1.0, 1.0, 0.0));

    float finalDensity = saturate(remap(basicCloud * shape, 1.0 - weatherData.r, 1.0, 0.0, 1.0));
    finalDensity = remap(finalDensity, detailModifier, 1.0, 0.0, 1.0) * density;
    return finalDensity;
}

float sampleCloudDensityWithoutDetail(vec3 worldPos, vec4 weatherData, float h)
{
    const float cloudTopOffset = 10.0;
    worldPos += h * uWindDirection * cloudTopOffset;
    worldPos += uWindDirection * osg_FrameTime * uWindSpeed;

    vec4 basicNoise = textureLod(uBasicNoiseTexture, worldPos * uBasicNoiseScaleFactor, 0);
    float basicFbm = dot(basicNoise.gba, vec3(0.625, 0.25, 0.125));
    float basicCloud = remap(basicNoise.r, basicFbm - 1.0, 1.0, 0.0, 1.0);

    const float anvil = 0.0;
    float shape = saturate(remap(h, 0.0, 0.07, 0.0, 1.0));
    shape *= saturate(remap(h, 0.2, 1.0, 1.0, 0.0));
    shape = pow(shape, saturate(remap(h, 0.65, 0.95, 1.0, 1.0 - anvil)));

    float density = h;
    density *= saturate(remap(h, 0.0, 0.2, 0.0, 1.0)) * 2;
    density *= mix(1.0, saturate(remap(pow(h, 0.5), 0.4, 0.95, 1.0, 0.2)), anvil);
    density *= saturate(remap(h, 0.9, 1.0, 1.0, 0.0));

    float finalDensity = saturate(remap(basicCloud * shape, 1.0 - weatherData.r, 1.0, 0.0, 1.0));
    finalDensity *= density;
    return finalDensity;
}

vec4 rayMarchCloud(in vec3 worldPos, in vec3 worldDir, inout float tDepth)
{
    float viewHeight = length(worldPos);
    float tMin, tMax;
    vec2 tTop2 = vec2(-1);
    vec2 tBottom2 = vec2(-1);
    vec3 planetCenter = vec3(0);
    if (rayIntersectSphereSolution(worldPos, worldDir, vec4(planetCenter, uCloudLayerTopRadiusKm), tTop2))
    {
        if (rayIntersectSphereSolution(worldPos, worldDir, vec4(planetCenter, uCloudLayerBottomRadiusKm), tBottom2))
        {
            float tempTop = all(greaterThan(tTop2, vec2(0))) ? min(tTop2.x, tTop2.y) : max(tTop2.x, tTop2.y);
            float tempBottom = all(greaterThan(tBottom2, vec2(0))) ? min(tBottom2.x, tBottom2.y) : max(tBottom2.x, tBottom2.y);

            if (all(greaterThan(tBottom2, vec2(0))))
                tempTop = max(0.0, min(tTop2.x, tTop2.y));

            tMin = min(tempBottom, tempTop);
            tMax = max(tempBottom, tempTop);
        }
        else
        {
            tMin = tTop2.x;
            tMax = tTop2.y;
        }
    }
    else
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }
    tMin = max(tMin, 0.0);
    tMax = max(tMax, 0.0);

    if (tMax <= tMin || tMin > uTracingStartMaxDistanceKm || tMin > tDepth)
        return vec4(0.0, 0.0, 0.0, 1.0);

    float marchingDistance = min(tMax - tMin, uTracingMaxDistanceKm);
    tMax = min(tMin + uTracingMaxDistanceKm, tDepth);

    return vec4(0);
}
