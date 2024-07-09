#version 460 core
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

#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
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

// Ray sphere intersection. 
// https://zhuanlan.zhihu.com/p/136763389
// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
// Returns distance from r0 to first intersecion with sphere, or -1.0 if no intersection.
float raySphereIntersectNearest(
      vec3  r0  // ray origin
    , vec3  rd  // normalized ray direction
    , vec3  s0  // sphere center
    , float sR) // sphere radius
{
	float a = dot(rd, rd);

	vec3 s02r0 = r0 - s0;
	float b = 2.0 * dot(rd, s02r0);

	float c = dot(s02r0, s02r0) - (sR * sR);
	float delta = b * b - 4.0 * a * c;

    // No intersection state.
	if (delta < 0.0 || a == 0.0)
	{
		return -1.0;
	}

	float sol0 = (-b - sqrt(delta)) / (2.0 * a);
	float sol1 = (-b + sqrt(delta)) / (2.0 * a);
	// sol1 > sol0


    // Intersection on negative direction, no suitable for ray.
	if (sol1 < 0.0) // When sol1 < 0.0, sol0 < 0.0 too.
	{
		return -1.0;
	}

    // Maybe exist one positive intersection.
	if (sol0 < 0.0)
	{
		return max(0.0, sol1);
	}

    // Two positive intersection, return nearest one.
	return max(0.0, min(sol0, sol1));
}

// When ensure r0 is inside of sphere.
// Only exist one positive result, use it.
float raySphereIntersectInside(
      vec3  r0  // ray origin
    , vec3  rd  // normalized ray direction
    , vec3  s0  // sphere center
    , float sR) // sphere radius
{
	float a = dot(rd, rd);

	vec3 s02r0 = r0 - s0;
	float b = 2.0 * dot(rd, s02r0);

	float c = dot(s02r0, s02r0) - (sR * sR);
	float delta = b * b - 4.0 * a * c;

	// float sol0 = (-b - sqrt(delta)) / (2.0 * a);
	float sol1 = (-b + sqrt(delta)) / (2.0 * a);

	// sol1 > sol0, so just return sol1
	return sol1;
}

// Ray intersection from outside of sphere.
// Return true if exist intersect. don't care about tangent case.
bool raySphereIntersectOutSide(
      vec3  r0  // ray origin
    , vec3  rd  // normalized ray direction
    , vec3  s0  // sphere center
    , float sR  // sphere radius
	, out vec2 t0t1) 
{
	float a = dot(rd, rd);

	vec3 s02r0 = r0 - s0;
	float b = 2.0 * dot(rd, s02r0);

	float c = dot(s02r0, s02r0) - (sR * sR);
	float delta = b * b - 4.0 * a * c;

    // No intersection state.
	if (delta < 0.0 || a == 0.0)
	{
		return false;
	}

	float sol0 = (-b - sqrt(delta)) / (2.0 * a);
	float sol1 = (-b + sqrt(delta)) / (2.0 * a);
	

    // Intersection on negative direction, no suitable for ray.
	if (sol1 <= 0.0 || sol0 <= 0.0)
	{
		return false;
	}

    // Two positive intersection, return nearest one.
	t0t1 = vec2(sol0, sol1); // sol1 > sol0
	return true; 
}

// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float hgPhase(float g, float cosTheta)
{
	float numer = 1.0f - g * g;
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	return numer / (4.0f * PI * denom * sqrt(denom));
}

float dualLobPhase(float g0, float g1, float w, float cosTheta)
{
	return mix(hgPhase(g0, cosTheta), hgPhase(g1, cosTheta), w);
}

void skyViewLutParametersToUV(in bool intersectGround, in float viewZenithCos, in float lightViewCos, in float viewHeight, out vec2 uv)
{
    float vHorizon = sqrt(viewHeight * viewHeight - uGroundRadius * uGroundRadius);
    float cosBeta = float(vHorizon / viewHeight); // GroundToHorizonCos
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;
    if (!intersectGround)
    {
        float coord = acos(viewZenithCos) / zenithHorizonAngle;
        coord = 1.0 - coord;
        coord = sqrt(coord);
        coord = 1.0 - coord;
        uv.y = coord * 0.5;
    }
    else
    {
        float coord = (acos(viewZenithCos) - zenithHorizonAngle) / beta;
        coord = sqrt(coord);
        uv.y = coord * 0.5 + 0.5;
    }
    float coord = -lightViewCos * 0.5 + 0.5;
    coord = sqrt(coord);
    uv.x = coord;
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

const float cloudBottomRadius = 6370.0;
const float cloudThickness = 10.0;
const float cloudTopRadius = cloudBottomRadius + cloudThickness;
const float cloudTracingStartMaxDistance = 50.0;
const float cloudMarchingStepCount = 128;
const float cloudPhaseForward = 0.5;
const float cloudPhaseBackward = -0.5;
const float cloudPhaseMixFactor = 0.2;
const float cloudCoverage = 0.5;
const float cloudDensity = 0.1;
const vec3 windDirection = vec3(0.8728715181, 0.2182178795, 0.4364357591);
const float cloudSpeed = 0.1;
const float cloudBasicNoiseScale = 0.15;
const float cloudDetailNoiseScale = 0.3;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }

float remap(float value, float orignalMin, float orignalMax, float newMin, float newMax)
{
    return newMin + (saturate((value - orignalMin) / (orignalMax - orignalMin)) * (newMax - newMin));
}

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

float sampleCloudDensity(vec3 p, float normalizedHeight)
{
    vec3 wind_direction = vec3(1.0, 0.0, 0.0);
    float cloud_speed = 0.01;
    float cloud_top_offset = 2.0;

    p += normalizedHeight * wind_direction * cloud_top_offset;
    p += (wind_direction + vec3(0, 0.1, 0)) * osg_FrameTime * cloud_speed;

    vec3 basicUVW = p * 0.007;
    vec4 low_frequency_noises = textureLod(uBasicNoiseTexture, basicUVW, 0.0);
    float low_freq_FBM = dot(low_frequency_noises.gba, vec3(0.625, 0.25, 0.125));
    float base_cloud = remap(low_frequency_noises.r, -(1.0 - low_freq_FBM), 1.0, 0.0, 1.0);

    vec3 weather_data = textureLod(uCloudMapTexture, p.xy * 0.006, 0.0).rgb;

    float density_height_gradiant = getDensityHeightGradient(normalizedHeight, 0.5);

    base_cloud *= density_height_gradiant;

    float cloud_coverage = pow(weather_data.r, remap(normalizedHeight, 0.7, 0.8, 1.0, mix(1.0, 0.5, 0.0)));
    float base_cloud_with_coverage = remap(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);

    base_cloud_with_coverage *= cloud_coverage;

    vec3 detailUVW = p * 0.1;
    vec3 high_frequency_noise = textureLod(uDetailNoiseTexture, detailUVW, 0.0).rgb;
    float high_freq_FBM = dot(high_frequency_noise, vec3(0.625, 0.25, 0.125));
    float high_freq_noise_modiffer = mix(high_freq_FBM, 1.0 - high_freq_FBM, clamp(normalizedHeight * 10.0, 0.0, 1.0));
    float final_cloud = remap(base_cloud_with_coverage, high_freq_noise_modiffer * 0.8, 1.0, 0.0, 1.0);
    
    return final_cloud;
}

#define TRANS_STEPS 8

// Scattering and absorption coefficients
const vec3 sigmaS = vec3(1.0);
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
        float density = sampleCloudDensity(p, saturate((length(p) - cloudBottomRadius) / cloudThickness));
        opticalDepth += dt * density * sigmaE;
        p += rd * dt;
    }
    return exp(-opticalDepth);
}

vec4 getCloudColor(vec3 worldPos, vec3 worldDir)
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

    if (tMax <= tMin)
        return vec4(0.0, 0.0, 0.0, 1.0);

    const float marchingDistance = min(cloudTracingStartMaxDistance, tMax - tMin);
    tMax = tMin + marchingDistance;

    // vec4 AP;
    // if (viewHeight < cloudBottomRadius)
    // {
    //     float slice = aerialPerspectiveDepthToSlice(tMin);
    //     float weight = 1.0;
    //     if (slice < 0.5)
    //     {
    //         weight = clamp(slice * 2.0, 0.0, 1.0);
    //         slice = 0.5;
    //     }
    //     float w = sqrt(slice / SLICE_COUNT);
    //     AP = weight * textureLod(uAerialPerspectiveLutTexture, vec3(uv, w), 0.0);
    // }
    // else
    // {
    //     AP = vec4(0.0, 0.0, 0.0, 0.0);
    // }
    // AP.a = 1.0 - AP.a;

    const uint stepCountUint = uint(cloudMarchingStepCount);
    const float stepCount = cloudMarchingStepCount;
    const float stepT = (tMax - tMin) / stepCount;

    float sampleT = tMin + 0.001 * stepT; // Slightly delta avoid self intersect.

    // sampleT += stepT * blueNoise;

    const float cosTheta = dot(-worldDir, uSunDirection);
    float phase = dualLobPhase(cloudPhaseForward, cloudPhaseBackward, cloudPhaseMixFactor, cosTheta);

    vec3 transmittance = vec3(1.0);
    vec3 scatteredLuminance = vec3(0.0);

    // vec3 cloudTopSkyLight = lookupSkyLight(worldDir, vec3(0.0, 0.0, cloudTopRadius), cloudTopRadius, vec3(0.0, 0.0, 1.0));
    // vec3 cloudBottomSkyLight = lookupSkyLight(worldDir, vec3(0.0, 0.0, cloudBottomRadius), cloudBottomRadius, vec3(0.0, 0.0, 1.0));
    // vec3 skyBackgroundColor = lookupSkyLight(worldDir, worldPos, viewHeight, normalize(worldPos));

    for (uint i = 0; i < stepCountUint; ++i)
    {
        vec3 samplePos = sampleT * worldDir + worldPos;
        float sampleHeight = length(samplePos);
        float normalizedHeight = saturate((sampleHeight - cloudBottomRadius) / cloudThickness);
        float density = sampleCloudDensity(samplePos, normalizedHeight);
        vec3 sampleSigmaS = sigmaS * density;
        vec3 sampleSigmaE = max(sigmaE * density, vec3(1e-6));

        if (density > 0)
        {
            float distOfSampleToCloudTop = raySphereIntersectInside(samplePos, uSunDirection, vec3(0.0), cloudTopRadius);
            vec3 cloudTopPos = samplePos + distOfSampleToCloudTop * uSunDirection;
            vec3 opticalDepth;
            vec3 transOfSampleToCloudTop = rayMarchTransmittance(samplePos, uSunDirection, distOfSampleToCloudTop, opticalDepth);
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

            vec3 powderEffectTerm;
            vec3 depthProbability = 0.05 + pow(opticalDepth, vec3(remap(normalizedHeight, 0.3, 0.85, 0.5, 2.0)));
            float verticalProbability = pow(remap(normalizedHeight, 0.07, 0.14, 0.1, 1.0), 0.8);
            powderEffectTerm = depthProbability * verticalProbability;

            //vec3 luminance = uSunIntensity * (sampleSigmaS * phase * transToSun + multiScattering * sampleSigmaS);
            vec3 luminance = uSunIntensity * sampleSigmaS * phase * transToSun * powderEffectTerm + multiScattering * sampleSigmaS;
            vec3 sampleTrans = exp(-sampleSigmaE * stepT);
            vec3 Sint = (luminance - luminance * sampleTrans) / sampleSigmaE;
            scatteredLuminance += transmittance * Sint;
            transmittance *= sampleTrans;
            if (length(transmittance) <= 0.001)
                break;
        }
        sampleT += stepT;
    }
    
    vec4 cloudColor = vec4(scatteredLuminance, dot(transmittance, vec3(0.33333)));
    return cloudColor;
    //return vec4(AP.rgb * uSunIntensity + AP.a * cloudColor.rgb, AP.a * cloudColor.a);
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

    if (tMax <= tMin)
        return vec4(0.0, 0.0, 0.0, 1.0);

    const float stepSize = 0.0625; // 16 sample per 1 km
    float sampleT = tMin + stepSize * 0.001;
    float sampleDensityPrevious = -1.0;
    uint zeroDensitySampleCount = 0;
    float cloudTest = 0.0;
    float alpha = 0.0;

    for (uint i = 0; i < cloudMarchingStepCount; ++i)
    {
        if (alpha <= 1.0)
        {
            vec3 samplePos = worldPos + sampleT * worldDir;
            float sampleHeight = length(samplePos);
            float normalizedHeight = saturate((sampleHeight - cloudBottomRadius) / cloudThickness);
            if (cloudTest > 0.0)
            {
                float sampleDensity = sampleCloudDensity(samplePos, normalizedHeight);

                if (sampleDensity == 0.0 && sampleDensityPrevious == 0.0)
                {
                    sampleDensityPrevious++;
                }

                if (zeroDensitySampleCount < 11 && sampleDensity != 0.0)
                {
                    // 昂贵的圆锥采样
                    alpha += sampleDensity;
                    // 获取光照
                }
                else
                {
                    // 回到便宜的采样
                    cloudTest = 0.0;
                    zeroDensitySampleCount = 0;
                }
                sampleT += stepSize;
                sampleDensityPrevious = sampleDensity;
            }
            else
            {
                cloudTest = sampleCloudDensity(samplePos, normalizedHeight);
                if (cloudTest == 0.0)
                {
                    sampleT += stepSize * 2;
                }
                else
                {
                    sampleT -= stepSize;
                }
            }
        }
    }
}

void main()
{
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace.w;
    vec3 worldDir = normalize(mat3(uInverseViewMatrix) * viewSpace.xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = length(worldPos);
    fragData = vec4(getCloudColor(worldPos, worldDir));
}