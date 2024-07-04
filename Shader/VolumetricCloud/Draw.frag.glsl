#version 460 core
in vec2 uv;
out vec4 fragData;

uniform sampler3D uNoise1Texture;
uniform sampler3D uNoise2Texture;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
uniform sampler2D uSkyViewLutTexture;

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
    return pos / 1000.0 + vec3(0.0, 0.0, uGroundRadius);
#else
    return pos / 1000.0;
#endif
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

vec3 lookupSkyLight(vec3 worldDir, vec3 worldPos, float viewHeight, vec3 upVector)
{
    float viewZenithCos = dot(worldDir, upVector);
    vec3 sideVector = normalize(cross(upVector, worldDir));
    vec3 forwardVector = normalize(cross(sideVector, upVector));
    vec2 lightOnPlane = vec2(dot(uSunDirection, forwardVector), dot(uSunDirection, sideVector));
    lightOnPlane = normalize(lightOnPlane);
    float lightViewCos = lightOnPlane.x;
    vec2 skyViewUV;
    skyViewLutParametersToUV(false, viewZenithCos, lightViewCos, viewHeight, skyViewUV);
    vec3 luminance = textureLod(uSkyViewLutTexture, skyViewUV, 0.0).rgb * uSunIntensity;
    return luminance;
}

const float cloudBottomRadius = 6361.0;
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

float remap(float value, float orignalMin, float orignalMax, float newMin, float newMax)
{
    return newMin + (saturate((value - orignalMin) / (orignalMax - orignalMin)) * (newMax - newMin));
}

float getCloudDensity(vec3 p)
{
    vec3 uvw = p * 0.01;
    vec4 noise = textureLod(uNoise1Texture, uvw/* + mod(osg_FrameTime * 0.1, 1.0)*/, 0.0);
    float wfbm = dot(noise.gba, vec3(0.625, 0.125, 0.25));
    float density = remap(noise.r, wfbm - 1.0, 1.0, 0.0, 1.0);
    return clamp(remap(density, 0.85, 1., 0., 1.), 0.0, 1.0);
}

#define TRANS_STEPS 8

// Scattering and absorption coefficients
const vec3 sigmaS = vec3(1);
const vec3 sigmaA = vec3(0.0);
// Extinction coefficient.
const vec3 sigmaE = max(sigmaS + sigmaA, vec3(1e-6));

vec3 rayMarchTransmittance(vec3 ro, vec3 rd, float dist)
{
    const float dt = dist / TRANS_STEPS;
    vec3 trans = vec3(1.0);
    vec3 p = ro;
    
    for (uint i = 0; i < TRANS_STEPS; ++i)
    {
        float density = getCloudDensity(p);
        trans *= exp(-dt * density * sigmaE);
        p += rd * dt;
    }
    return trans;
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

    if (tMax <= tMin || tMin > cloudTracingStartMaxDistance)
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    const float marchingDistance = min(cloudTracingStartMaxDistance, tMax - tMin);
    tMax = tMin + marchingDistance;

    const uint stepCountUint = uint(cloudMarchingStepCount);
    const float stepCount = cloudMarchingStepCount;
    const float stepT = (tMax - tMin) / stepCount;

    float sampleT = tMin + 0.001 * stepT; // Slightly delta avoid self intersect.

    // sampleT += stepT * blueNoise;

    vec3 sunColor = uSunIntensity;
    float vdl = dot(worldDir, uSunDirection);

    const float cosTheta = -vdl;
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
        float normalizedHeight = (sampleHeight - cloudBottomRadius) / cloudThickness;
        float density = getCloudDensity(samplePos);//cloudMap(samplePosMeter, normalizedHeight);
        vec3 sampleSigmaS = sigmaS * density;
        vec3 sampleSigmaE = sigmaE * density;

        if (density > 0)
        {
            float distFromSamplePosToCloudTop = raySphereIntersectInside(samplePos, uSunDirection, vec3(0.0), cloudTopRadius);
            vec3 cloudTopPos = samplePos + distFromSamplePosToCloudTop * uSunDirection;
            vec3 transFromCloudTopToSun;
            {
                const vec3 upVector = cloudTopPos / cloudTopRadius;
                float viewZenithCos = dot(uSunDirection, upVector);
                vec2 sampleUV;
                transmittanceLutParametersToUV(cloudTopRadius, viewZenithCos, sampleUV);
                transFromCloudTopToSun = texture(uTransmittanceLutTexture, sampleUV).rgb;
            }
            vec3 cloudTransmittance = rayMarchTransmittance(samplePos, uSunDirection, distFromSamplePosToCloudTop);
            vec3 transToSun = transFromCloudTopToSun * cloudTransmittance;
            vec3 luminance = uSunIntensity * sampleSigmaS * phase * transToSun;
            vec3 sampleTrans = exp(-sampleSigmaE * stepT);
            vec3 Sint = (luminance - luminance * sampleTrans) / sampleSigmaE;
            scatteredLuminance += transmittance * Sint;
            transmittance *= sampleTrans;
            if (length(transmittance) <= 0.001)
                break;
        }
        sampleT += stepT;
    }
    return vec4(scatteredLuminance, dot(transmittance, vec3(0.3333333)));
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