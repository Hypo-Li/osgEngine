#version 460 core
#extension GL_GOOGLE_include_directive : enable
#pragma import_defines(FAST_RENDER)
#ifndef FAST_RENDER
#define FAST_RENDER 0
#endif
in vec2 uv;
out vec4 fragData[2];

/**
* 当在云层下方时, 采用较大步长的固定次数采样, 兼顾云层可视范围, 渲染质量和效率
* 当在云层中时, 采用天气纹理sdf, 降低采样步长, 以获得较好的渲染质量和可视范围, 但渲染效率会有所降低
* 当在云层上方时, 禁用深度, 防止osgEarth的球体遮挡云层
*/

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

const float PI = 3.1415926535897932;

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

//const vec3 uAlbedo = vec3(1.0);
//const vec3 uExtinction = vec3(0.005);
const vec3 uWindDirection = vec3(0.8728715181, 0.2182178795, 0.4364357591);
//const float uWindSpeed = 2.0;
//const float uCloudLayerBottomRadiusKm = 6365.0;
//const float uCloudLayerThicknessKm = 10.0;
float uCloudLayerTopRadiusKm = uCloudLayerBottomRadiusKm + uCloudLayerThicknessKm;
const float uTracingStartMaxDistanceKm = 800.0;
const float uTracingMaxDistanceKm = 200.0;
const vec3 uStopTracingTransmittanceThreshold = vec3(0.005);
const uint uCloudSampleCountMin = 54;
const uint uCloudSampleCountMax = 96;
const float uSampleCountPerKm = 15.0;
const float uCloudShadowTracingMaxDistanceKm = 15.0;
const uint uCloudShadowSampleCountMax = 6;
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

void computeCloudLighting(vec3 samplePos, float density, float dtMeters, inout ParticipatingMediaPhaseContext pmpc, inout vec3 luminance, inout vec3 transmittanceToView)
{
    const float sampleHeight = length(samplePos);
    vec3 transmittanceToLight0;
    {
        const vec3 upVector = samplePos / sampleHeight;
        const float viewZenithCos = dot(uSunDirection, upVector);
        vec2 sampleUV;
        transmittanceLutParametersToUV(sampleHeight, viewZenithCos, sampleUV);
        transmittanceToLight0 = texture(uTransmittanceLutTexture, sampleUV).rgb;
    }
    ParticipatingMediaContext pmc = setupParticipatingMediaContext(uAlbedo, density * uExtinction, uMsScattFactor, uMsExtinFactor, transmittanceToLight0);

    const float maxTransmittanceToView = max(max(transmittanceToView.x, transmittanceToView.y), transmittanceToView.z);
    vec3 extinctionAcc[MS_COUNT];
    const float shadowLengthTest = uCloudShadowTracingMaxDistanceKm;
    const float shadowStepCount = float(uCloudShadowSampleCountMax);
    const float invShadowStepCount = 1.0 / shadowStepCount;
    const float shadowJitteringSeed = float(osg_FrameNumber / 16 % 8) + pseudoRandom(gFullResCoord);
    const float shadowJitterNorm = 0.5; //interleavedGradientNoise(gl_FragCoord.xy, shadowJitteringSeed) - 0.5;

    for (uint ms = 0; ms < MS_COUNT; ++ms)
        extinctionAcc[ms] = vec3(0.0);

    const float shadowDtMeter = shadowLengthTest * 1000.0;
    float previousNormT = 0.0;
    for (float shadowT = invShadowStepCount; shadowT <= 1.00001; shadowT += invShadowStepCount)
    {
        float currentNormT = shadowT * shadowT;
        const float detalNormT = currentNormT - previousNormT;
        const float extinctionFactor = detalNormT;
        const float shadowSampleDistance = shadowLengthTest * (previousNormT + detalNormT * shadowJitterNorm);
        const vec3 shadowSamplePos = samplePos + uSunDirection * shadowSampleDistance;
        const float shadowSampleNormalizedHeight = saturate((length(shadowSamplePos) - uCloudLayerBottomRadiusKm) / uCloudLayerThicknessKm);
        float shadowSampleDensity = sampleCloudDensity(shadowSamplePos, sampleWeather(shadowSamplePos), shadowSampleNormalizedHeight);
        previousNormT = currentNormT;

        if (shadowSampleDensity <= 0)
            continue;
        
        ParticipatingMediaContext shadowPMC = setupParticipatingMediaContext(vec3(0), shadowSampleDensity * uExtinction, uMsScattFactor, uMsExtinFactor, vec3(0));

        for (uint ms = 0; ms < MS_COUNT; ++ms)
            extinctionAcc[ms] += shadowPMC.extinctionCoeff[ms] * extinctionFactor;
    }

    for (uint ms = 0; ms < MS_COUNT; ++ms)
        pmc.transmittanceToLight0[ms] *= exp(-extinctionAcc[ms] * shadowDtMeter);

    const float h = saturate((sampleHeight - uCloudLayerBottomRadiusKm) / uCloudLayerThicknessKm);
    const vec3 distantLightLuminance = gDistantSkyLight * saturate(0.5 + h);
    for (uint ms = MS_COUNT - 1; ms >= 0; --ms)
    {
        const vec3 scatteringCoeff = pmc.scatteringCoeff[ms];
        const vec3 extinctionCoeff = pmc.extinctionCoeff[ms];
        const vec3 transmittanceToLight0 = pmc.transmittanceToLight0[ms];
        vec3 sunSkyLuminance = transmittanceToLight0 * uSunIntensity * pmpc.phase0[ms];
        sunSkyLuminance += (ms == 0 ? distantLightLuminance : vec3(0));

        const vec3 scatteredLuminance = sunSkyLuminance * scatteringCoeff;
        const vec3 safeExtinctionThreshold = vec3(0.000001);
        const vec3 safeExtinctionCoeff = max(safeExtinctionThreshold, extinctionCoeff);
        const vec3 safePathSegmentTransmittance = exp(-safeExtinctionCoeff * dtMeters);
        vec3 luminanceIntegral = (scatteredLuminance - scatteredLuminance * safePathSegmentTransmittance) / safeExtinctionCoeff;
        luminance += transmittanceToView * luminanceIntegral;

        if (ms == 0)
            transmittanceToView *= safePathSegmentTransmittance;
    }
}

vec4 rayMarchCloud(in vec3 worldPos, in vec3 worldDir, in float tDepth, out float outDepth)
{
    vec3 cloudMapN = vec3(0, 0, 1);
    float sinTheta = max(length(cross(worldDir, cloudMapN)), 0.01);
    // max half float
    outDepth = 65504.0;
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

    if (tMax <= tMin || tMin > uTracingStartMaxDistanceKm)
        return vec4(0.0, 0.0, 0.0, 1.0);
    if (viewHeight < uCloudLayerTopRadiusKm && tMin > tDepth)
        return vec4(0.0, 0.0, 0.0, 1.0);

    float marchingDistance = min(tMax - tMin, uTracingMaxDistanceKm);
    if (viewHeight < uCloudLayerTopRadiusKm)
        marchingDistance = min(marchingDistance, tDepth);
    tMax = tMin + marchingDistance;

    const float cosTheta = dot(-worldDir, uSunDirection);
    const float phase = dualLobPhase(uPhaseG0, uPhaseG1, uPhaseBlend, cosTheta);

    ParticipatingMediaPhaseContext pmpc = setupParticipatingMediaPhaseContext(phase, uMsPhaseFactor);
    
    vec3 luminance = vec3(0.0);
    vec3 transmittanceToView = vec3(1.0);

    if (viewHeight < uCloudLayerBottomRadiusKm)
    {
        const uint stepCountUint = uint(clamp(marchingDistance * uSampleCountPerKm, uCloudSampleCountMin, uCloudSampleCountMax));
        const float stepCount = float(stepCountUint);
        const float stepT = (tMax - tMin) / stepCount;
        const float dtMeters = stepT * 1000.0;
        float t = tMin + getBlueNoise(ivec2(gFullResCoord), osg_FrameNumber / 16 % 8) * stepT * 0.5;

        for (uint i = 0; i < stepCountUint; ++i)
        {
            if (t >= tMax)
                break;
            const vec3 samplePos = worldPos + worldDir * t;
            const float sampleHeight = length(samplePos);
            const float h = saturate((sampleHeight - uCloudLayerBottomRadiusKm) / uCloudLayerThicknessKm);
            vec4 weatherData = sampleWeather(samplePos);
            float dt = stepT;
            if (weatherData.r == 0.0)
            {
                t += dt * 2;
                continue;
            }
            const float density = sampleCloudDensity(samplePos, weatherData, h);
            if (density <= 0.01)
            {
                t += dt;
                continue;
            }

            if (t < outDepth)
                outDepth = t;

            computeCloudLighting(samplePos, density, dtMeters, pmpc, luminance, transmittanceToView);

            if (all(lessThan(transmittanceToView, uStopTracingTransmittanceThreshold)))
                break;

            t += dt;
        }
    }
    else
    {
        const float basicStepT = 1.0 / 8.0;
        float stepT = basicStepT;
        float dtMeters = stepT * 1000.0;
        float t = tMin + getBlueNoise(ivec2(gFullResCoord), osg_FrameNumber / 16 % 8) * stepT * 0.5;

        uint validSampleCount = 0;
        while (t < tMax && validSampleCount < uCloudSampleCountMax)
        {
            const vec3 samplePos = worldPos + worldDir * t;
            const float sampleHeight = length(samplePos);
            const float h = saturate((sampleHeight - uCloudLayerBottomRadiusKm) / uCloudLayerThicknessKm);
            vec4 weatherData = sampleWeather(samplePos);
            float weatherSdfStep = weatherData.a / (uCloudMapScaleFactor * sinTheta);
            float dt = max(basicStepT * 8, weatherSdfStep);
            if (weatherData.r == 0.0)
            {
                t += dt;
                continue;
            }

            const float density = sampleCloudDensity(samplePos, weatherData, h);
            dt = max(basicStepT, weatherSdfStep);
            if (density <= 0.01)
            {
                t += dt;
                continue;
            }

            stepT *= 1.5;
            dtMeters = stepT * 1000.0;
            dt = stepT;
            validSampleCount++;

            if (t < outDepth)
                outDepth = t;

            computeCloudLighting(samplePos, density, dtMeters, pmpc, luminance, transmittanceToView);

            if (all(lessThan(transmittanceToView, uStopTracingTransmittanceThreshold)))
                break;

            t += dt;
        }
    }

    vec4 outColor = vec4(luminance, dot(transmittanceToView, vec3(1.0 / 3.0)));
    float slice = aerialPerspectiveDepthToSlice(tMin);
    float weight = 1.0;
    if (slice < 0.5)
    {
        weight = clamp(slice * 2.0, 0.0, 1.0);
        slice = 0.5;
    }
    float w = sqrt(slice / SLICE_COUNT);
    vec4 AP = weight * textureLod(uAerialPerspectiveLutTexture, vec3(uv, w), 0.0);
    AP.a = 1.0 - AP.a;
    outColor = vec4(AP.rgb * (1.0 - outColor.a) + AP.a * outColor.rgb, outColor.a);
    return outColor;
}

int bayerFilter4x4[] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5
};

void main()
{
    gDistantSkyLight = texelFetch(uDistantSkyLightLutTexture, ivec2(0, 0), 0).rgb;

#if FAST_RENDER
    uint bayerIndex = osg_FrameNumber % 16;
    ivec2 bayerOffset = ivec2(bayerFilter4x4[bayerIndex] % 4, bayerFilter4x4[bayerIndex] / 4);
    ivec2 iFullResCoord = ivec2(gl_FragCoord.xy) * 4 + bayerOffset;
    gFullResCoord = iFullResCoord + vec2(0.5);
    float depth = texelFetch(uDepthTexture, iFullResCoord, 0).r;

    vec2 fullRes = textureSize(uDepthTexture, 0);
    vec4 clipSpace = vec4(gFullResCoord / fullRes * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace.w;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * viewSpace.xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
#else
    gFullResCoord = gl_FragCoord.xy;
    float depth = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).r;
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace.w;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * viewSpace.xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
#endif
    if (viewHeight > 6550)
    {
        fragData[0] = vec4(0, 0, 0, 1);
        fragData[1] = vec4(65504);
        return;
    }
    float tDepth = depth == 1.0 ? uCloudLayerTopRadiusKm : -viewSpace.z / 1000.0;
    float outDepth;
    fragData[0] = vec4(rayMarchCloud(worldPos, worldDir, tDepth, outDepth));
    fragData[1] = vec4(outDepth);
}