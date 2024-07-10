#version 460 core
#extension GL_GOOGLE_include_directive : enable
#include "intersection.glsl"
in vec2 uv;
out vec4 fragData;
uniform sampler2D uCloudMapTexture;
uniform sampler3D uBasicNoiseTexture;
uniform sampler3D uDetailNoiseTexture;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
uniform sampler2D uSkyViewLutTexture;
uniform sampler3D uAerialPerspectiveLutTexture;
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

const float PI = 3.1415926535897932;

const float cloudBottomRadius = 6365.0;
const float cloudThickness = 10.0;
const float cloudTopRadius = cloudBottomRadius + cloudThickness;
const float cloudTracingStartMaxDistance = 50.0;
//const float cloudMarchingStepCount = 128;
const vec3 windDirection = vec3(0.8728715181, 0.2182178795, 0.4364357591);
const float windSpeed = 0.01;
const float cloudPhaseForward = 0.5;
const float cloudPhaseBackward = -0.5;
const float cloudPhaseMixFactor = 0.2;
const float tracingStartMaxDistance = 350.0; // km
const float tracingMaxDistance = 50.0;
const uint sampleCountMin = 2;
const uint sampleCountMax = 128;
const float invDistanceToSampleCountMax = 0.06667;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }

float remap(float value, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + (saturate((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin));
}

#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
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

// float getDensityHeightGradient(in float normalizedHeight, in float cloudType)
// {
//     // 根据2017年的分享，两个Remap相乘重建云属
//     float cumulus = max(0.0, remap(normalizedHeight, 0.01, 0.3, 0.0, 1.0) * remap(normalizedHeight, 0.6, 0.95, 1.0, 0.0));
//     float stratocumulus = max(0.0, remap(normalizedHeight, 0.0, 0.25, 0.0, 1.0) * remap(normalizedHeight, 0.3, 0.65, 1.0, 0.0));
//     float stratus = max(0.0, remap(normalizedHeight, 0, 0.1, 0.0, 1.0) * remap(normalizedHeight, 0.2, 0.3, 1.0, 0.0));

//     // 云属过渡
//     float a = mix(stratus, stratocumulus, clamp(cloudType * 2.0, 0.0, 1.0));
//     float b = mix(stratocumulus, cumulus, clamp((cloudType - 0.5) * 2.0, 0.0, 1.0));
//     return mix(a, b, cloudType);
// }

float sampleGradient(vec4 gradient, float normalizedHeight)
{
    return smoothstep(gradient.x, gradient.y, normalizedHeight) - smoothstep(gradient.z, gradient.w, normalizedHeight);
}

float getDensityHeightGradient(float normalizedHeight, float cloudType)
{
    const vec4 cloudGradient1 = vec4(0.0, 0.065, 0.203, 0.371); //stratus
    const vec4 cloudGradient2 = vec4(0.0, 0.156, 0.468, 0.674); //cumulus
    const vec4 cloudGradient3 = vec4(0.0, 0.188, 0.818, 1); //cumulonimbus
    vec4 gradient = mix(mix(cloudGradient1, cloudGradient2, cloudType * 2.0), cloudGradient3, saturate(cloudType - 0.5) * 2.0);
    return sampleGradient(gradient, normalizedHeight);
}

vec3 sampleWeather(vec3 worldPos)
{
    return textureLod(uCloudMapTexture, worldPos.xy * 0.006, 0.0).rgb;
}

float sampleCloudDensity(vec3 worldPos)
{
    const float basicNoiseScale = 0.007;
    const float detailNoiseScale = 0.1;

    vec3 weatherData = sampleWeather(worldPos);
    float normalizedHeight = saturate((length(worldPos) - cloudBottomRadius) / cloudThickness);

    vec3 wind_direction = windDirection;
    float cloud_speed = 1.0; // PPT中给的是10
    // cloud_top offset ，用于偏移高处受风力影响的程度
    float cloud_top_offset = 10.0;

    // 为采样位置增加高度梯度，风向影响;
    worldPos += normalizedHeight * wind_direction * cloud_top_offset;
    // 增加一些沿着风向的时间偏移
    worldPos += (wind_direction + vec3(0.0, 0.1, 0.0)  ) * osg_FrameTime * cloud_speed;

    vec3 basicUVW = worldPos * basicNoiseScale;
    vec4 low_frequency_noises = textureLod(uBasicNoiseTexture, basicUVW, 0.0);
    float low_freq_FBM = dot(low_frequency_noises.gba, vec3(0.625, 0.25, 0.125));
    float base_cloud = remap(low_frequency_noises.r, -(1.0 - low_freq_FBM), 1.0, 0.0, 1.0);

    float density_height_gradiant = getDensityHeightGradient(normalizedHeight, weatherData.g);

    base_cloud *= density_height_gradiant;

    float cloud_coverage = pow(weatherData.r, remap(normalizedHeight, 0.7, 0.8, 1.0, mix(1.0, 0.5, 0.0)));
    float base_cloud_with_coverage = remap(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);

    base_cloud_with_coverage *= cloud_coverage;

    vec3 detailUVW = worldPos * detailNoiseScale;
    vec3 high_frequency_noise = textureLod(uDetailNoiseTexture, detailUVW, 0.0).rgb;
    float high_freq_FBM = dot(high_frequency_noise, vec3(0.625, 0.25, 0.125));
    float high_freq_noise_modiffer = mix(high_freq_FBM, 1.0 - high_freq_FBM, clamp(normalizedHeight * 10.0, 0.0, 1.0));
    float final_cloud = remap(base_cloud_with_coverage, high_freq_noise_modiffer, 1.0, 0.0, 1.0);
    
    return final_cloud;
}

#define TRANS_STEPS 8

// Scattering and absorption coefficients
const vec3 sigmaS = vec3(2.0);
const vec3 sigmaA = vec3(0.0);
// Extinction coefficient.
const vec3 sigmaE = max(sigmaS + sigmaA, vec3(1e-6));

vec3 rayMarchTransmittance(vec3 ro, vec3 rd, float dist, out vec3 opticalDepth)
{
    const float dt = dist / TRANS_STEPS;
    vec3 trans = vec3(1.0);
    vec3 p = ro;
    opticalDepth = vec3(0);

    for (uint i = 0; i < TRANS_STEPS; ++i)
    {
        float density = sampleCloudDensity(p);
        opticalDepth += dt * density * sigmaE;
        p += rd * dt;
    }
    return exp(-opticalDepth);
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
#define KM_PER_SLICE 4.0

float aerialPerspectiveDepthToSlice(float depth)
{
    return depth * (1.0 / KM_PER_SLICE);
}

vec4 rayMarchCloud(vec3 worldPos, vec3 worldDir)
{
    float viewHeight = length(worldPos);
    float tMin, tMax;
    if (viewHeight < cloudBottomRadius)
    {
        float tEarth = raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), uGroundRadius);
        if (tEarth > 0.0)
            return vec4(0.0, 0.0, 0.0, 1.0);
        
        tMin = raySphereIntersectInside(worldPos, worldDir, vec3(0.0), cloudBottomRadius);
        tMax = raySphereIntersectInside(worldPos, worldDir, vec3(0.0), cloudTopRadius);
    }
    else if(viewHeight > cloudTopRadius)
    {
        vec2 t0t1 = vec2(0.0);
        const bool bIntersectionEnd = raySphereIntersectOutSide(worldPos, worldDir, vec3(0.0), cloudTopRadius, t0t1);
        if (!bIntersectionEnd)
        {
            return vec4(0.0, 0.0, 0.0, 1.0);
        }

        vec2 t2t3 = vec2(0.0);
        const bool bIntersectionStart = raySphereIntersectOutSide(worldPos, worldDir, vec3(0.0), cloudBottomRadius, t2t3);
        if (bIntersectionStart)
        {
            tMin = t0t1.x;
            tMax = t2t3.x;
        }
        else
        {
            tMin = t0t1.x;
            tMax = t0t1.y;
        }
    }
    else
    {
        float tStart = raySphereIntersectNearest(worldPos, worldDir, vec3(0.0), cloudBottomRadius);
        if (tStart > 0.0)
        {
            tMax = tStart;
        }
        else
        {
            tMax = raySphereIntersectInside(worldPos, worldDir, vec3(0.0), cloudTopRadius);
        }
        tMin = 0.0;
    }
    tMin = max(tMin, 0.0);
    tMax = max(tMax, 0.0);

    if (tMax <= tMin || tMin > tracingStartMaxDistance)
        return vec4(0.0, 0.0, 0.0, 1.0);

    float marchingDistance = min(tMax - tMin, tracingMaxDistance);
    tMax = tMin + tracingMaxDistance;

    uint stepCount = max(sampleCountMin, uint(sampleCountMax * saturate((tMax - tMin) * invDistanceToSampleCountMax)));

    const float stepSize = (tMax - tMin) / stepCount;
    float sampleT = tMin + stepSize * 0.5;
    vec3 totalLuminance = vec3(0.0);
    vec3 totalTransmitance = vec3(1.0);

    const float cosTheta = dot(-worldDir, uSunDirection);
    float phase = dualLobPhase(cloudPhaseForward, cloudPhaseBackward, cloudPhaseMixFactor, cosTheta);

    // uvec2 offset = uvec2(vec2(0.754877669, 0.569840296) * (osg_FrameNumber) * uvec2(128));
    // uvec2 offsetId = uvec2(gl_FragCoord.xy) + offset;
    // offsetId.x = offsetId.x % 128;
    // offsetId.y = offsetId.y % 128;
    // float blueNoise2 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d(offsetId.x, offsetId.y, 0, 0u);

    for (uint i = 0; i < stepCount; ++i)
    {
        vec3 samplePos = worldPos + sampleT * worldDir;
        float sampleHeight = length(samplePos);
        float density = sampleCloudDensity(samplePos);

        if (density > 0)
        {
            float normalizedHeight = saturate((sampleHeight - cloudBottomRadius) / cloudThickness);
            vec3 sampleSigmaS = sigmaS * density;
            vec3 sampleSigmaE = max(sigmaE * density, vec3(1e-6));

            float distOfSampleToCloudTop = raySphereIntersectInside(samplePos, uSunDirection, vec3(0.0), cloudTopRadius);
            vec3 cloudTopPos = samplePos + distOfSampleToCloudTop * uSunDirection;
            vec3 opticalDepth;
            vec3 transOfSampleToCloudTop = rayMarchTransmittance(samplePos, uSunDirection, distOfSampleToCloudTop, opticalDepth);
            transOfSampleToCloudTop = max(transOfSampleToCloudTop, exp(-opticalDepth * 0.25) * 0.7);
            vec3 transOfCloudTopToSun;
            {
                const vec3 upVector = cloudTopPos / cloudTopRadius;
                float viewZenithCos = dot(uSunDirection, upVector);
                vec2 sampleUV;
                transmittanceLutParametersToUV(cloudTopRadius, viewZenithCos, sampleUV);
                transOfCloudTopToSun = texture(uTransmittanceLutTexture, sampleUV).rgb;
            }
            vec3 transToSun = transOfSampleToCloudTop * transOfCloudTopToSun;

            vec3 upVector = samplePos * (1.0 / sampleHeight);
            float sunZenithCos = dot(uSunDirection, upVector);
            vec3 multiScattering = textureLod(uMultiScatteringLutTexture, vec2(sunZenithCos * 0.5 + 0.5, (sampleHeight - uGroundRadius) / (uAtmosphereRadius - uGroundRadius)), 0.0).rgb;

            vec3 luminance = uSunIntensity * (sampleSigmaS * phase * transToSun + multiScattering * sampleSigmaS);
            //vec3 luminance = uSunIntensity * sampleSigmaS * phase * transToSun + multiScattering * sampleSigmaS;
            vec3 sampleTransmittance = exp(-sampleSigmaE * stepSize);
            vec3 Sint = (luminance - luminance * sampleTransmittance) / sampleSigmaE;
            totalLuminance += totalTransmitance * Sint;
            totalTransmitance *= sampleTransmittance;
            if (length(totalTransmitance) <= 0.001)
                break;
        }

        sampleT += stepSize;
    }
    vec4 result = vec4(totalLuminance, dot(totalTransmitance, vec3(0.333333)));
    if (any(greaterThan(totalLuminance, vec3(0.0))))
    {
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
        result = vec4(AP.rgb * (1.0 - result.a) + AP.a * result.rgb, result.a);
    }
    return result;
}

void main()
{
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace.w;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * viewSpace.xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
    fragData = vec4(rayMarchCloud(worldPos, worldDir));
}
