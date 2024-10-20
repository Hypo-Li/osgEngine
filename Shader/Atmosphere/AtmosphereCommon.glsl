#ifdef TRANSMITTANCE_LUT
#define NO_TRANSMITTANCE_LUT_TEXTURE
#endif

#ifdef MULTISCATTERING_LUT
#define GROUND_ALBEDO_ENABLE
#define ILLUMINANCE_IS_ONE
#endif

#ifdef DISTANT_SKYLIGHT_LUT
#define MULTISCATTERING_APPROX_SAMPLING_ENABLED
#endif

#ifdef SKY_VIEW_LUT
#define VARIABLE_SAMPLE_COUNT_ENABLE
#define MIE_RAYLEIGH_PHASE_ENABLE
#define MULTISCATTERING_APPROX_SAMPLING_ENABLED
#endif

#ifdef AERIAL_PERSPECTIVE_LUT
#define MIE_RAYLEIGH_PHASE_ENABLE
#define MULTISCATTERING_APPROX_SAMPLING_ENABLED
#endif

#ifdef ATMOSPHERE_RAY_MARCHING
#define VARIABLE_SAMPLE_COUNT_ENABLE
#define MIE_RAYLEIGH_PHASE_ENABLE
#define MULTISCATTERING_APPROX_SAMPLING_ENABLED
//#define RAY_MARCHING_WITH_DEPTH
#endif

#define RAY_MARCHING_MIN_SPP 4
#define RAY_MARCHING_MAX_SPP 32
#define SLICE_COUNT 16.0
#define KM_PER_SLICE 8.0

const float PI = 3.1415926535897932;

#define PLANET_RADIUS_OFFSET 0.001
layout (std140, binding = BINDING_INDEX) uniform AtmosphereParameters
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

float aerialPerspectiveDepthToSlice(float depth)
{
    return depth * (1.0f / KM_PER_SLICE);
}

float aerialPerspectiveSliceToDepth(float slice)
{
    return slice * KM_PER_SLICE;
}

// return distance from rayOrigin to closest intersection with sphere
// or -1 if no intersection
float rayIntersectSphere(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius)
{
    vec3 localPos = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, localPos);
    float c = dot(localPos, localPos) - (sphereRadius * sphereRadius);
    float delta = b * b - 4.0 * a * c;
    if (delta < 0.0 || a == 0.0) return -1.0;
    float sol0 = (-b - sqrt(delta)) / (2.0*a);
    float sol1 = (-b + sqrt(delta)) / (2.0*a);
    if (sol0 < 0.0 && sol1 < 0.0) return -1.0;
    if (sol0 < 0.0) return max(0.0, sol1);
    else if (sol1 < 0.0) return max(0.0, sol0);
    return max(0.0, min(sol0, sol1));
}

float rayIntersectSphere(vec3 rayOrigin, vec3 rayDir, float sphereRadius)
{
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, rayOrigin);
    float c = dot(rayOrigin, rayOrigin) - (sphereRadius * sphereRadius);
    float delta = b * b - 4.0 * a * c;
    if (delta < 0.0 || a == 0.0) return -1.0;
    float sol0 = (-b - sqrt(delta)) / (2.0*a);
    float sol1 = (-b + sqrt(delta)) / (2.0*a);
    if (sol0 < 0.0 && sol1 < 0.0) return -1.0;
    if (sol0 < 0.0) return max(0.0, sol1);
    else if (sol1 < 0.0) return max(0.0, sol0);
    return max(0.0, min(sol0, sol1));
}

bool moveToTopAtmosphere(inout vec3 worldPos, in vec3 worldDir)
{
    float viewHeight = length(worldPos);
    if (viewHeight > uAtmosphereRadius)
    {
        float tTop = rayIntersectSphere(worldPos, worldDir, uAtmosphereRadius);
        if (tTop >= 0.0)
        {
            vec3 upVector = vec3(worldPos / viewHeight);
            vec3 upOffset = upVector * -(PLANET_RADIUS_OFFSET * 100.0);
            worldPos = worldPos + worldDir * tTop + upOffset;
        }
        else
            // Ray is not intersecting the atmosphere
            return false;
    }
    return true; // ok to start tracing
}

void uvToTransmittanceLutParameters(in vec2 uv, out float viewHeight, out float viewZenithCos)
{
    float H = sqrt(uAtmosphereRadius * uAtmosphereRadius - uGroundRadius * uGroundRadius);
    float rho = H * uv.y;
    viewHeight = sqrt(rho * rho + uGroundRadius * uGroundRadius);
    float dMin = uAtmosphereRadius - viewHeight;
    float dMax = rho + H;
    float d = dMin + uv.x * (dMax - dMin);
    viewZenithCos = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * viewHeight * d);
    viewZenithCos = clamp(viewZenithCos, -1.0, 1.0);
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

void uvToSkyViewLutParameters(in float viewHeight, in vec2 uv, out float viewZenithCos, out float lightViewCos)
{
    float vHorizon = sqrt(viewHeight * viewHeight - uGroundRadius * uGroundRadius);
    float cosBeta = float(vHorizon / viewHeight);
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;
    if (uv.y < 0.5)
    {
        float coord = 2.0 * uv.y;
        coord = 1.0 - coord;
        coord *= coord;
        coord = 1.0 - coord;
        viewZenithCos = cos(zenithHorizonAngle * coord);
    }
    else
    {
        float coord = uv.y * 2.0 - 1.0;
        coord *= coord;
        viewZenithCos = cos(zenithHorizonAngle + beta * coord);
    }
    float coord = uv.x;
    coord *= coord;
    lightViewCos = -(coord * 2.0 - 1.0);
}

void skyViewLutParametersToUV(in bool intersectGround, in float viewZenithCos, in float lightViewCos, in float viewHeight, out vec2 uv)
{
    float vHorizon = sqrt(viewHeight * viewHeight - uGroundRadius * uGroundRadius);
    float cosBeta = vHorizon / viewHeight; // GroundToHorizonCos
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;
    float viewZenithAngle = acos(viewZenithCos);
    if (!intersectGround)
    {
        float coord = viewZenithAngle / zenithHorizonAngle;
        coord = 1.0 - coord;
        coord = sqrt(coord);
        coord = 1.0 - coord;
        uv.y = coord * 0.5;
    }
    else
    {
        float coord = (viewZenithAngle - zenithHorizonAngle) / beta;
        coord = sqrt(coord);
        uv.y = coord * 0.5 + 0.5;
    }
    float coord = -lightViewCos * 0.5 + 0.5;
    coord = sqrt(coord);
    uv.x = coord;
}

void getScatteringAndExtinction(in vec3 worldPos, out vec3 rayleighScattering, out float mieScattering, out vec3 extinction)
{
    float altitude = length(worldPos) - uGroundRadius;
    float rayleighDensity = exp(-altitude / uRayleighDensityH);
    rayleighScattering = uRayleighScatteringBase * rayleighDensity;
    float mieDensity = exp(-altitude / uMieDensityH);
    mieScattering = uMieScatteringBase * mieDensity;
    float mieAbsorption = uMieAbsorptionBase * mieDensity;
    vec3 ozoneAbsorption = uOzoneAbsorptionBase * max(0.0, 1.0 - abs(altitude - uOzoneCenterHeight)/uOzoneThickness);
    extinction = rayleighScattering + mieScattering + mieAbsorption + ozoneAbsorption;
}

vec3 getOpticalDepth(vec3 worldPos, vec3 worldDir, float sampleCount)
{
    if (rayIntersectSphere(worldPos, worldDir, uGroundRadius) > 0.0)
        return vec3(0.0);
    float atmoDist = rayIntersectSphere(worldPos, worldDir, uAtmosphereRadius);
    float t = 0.0;
    vec3 opticalDepth = vec3(0.0);
    for (float s = 0.0; s < sampleCount; s += 1.0)
    {
        float newT = ((s + 0.3) / sampleCount) * atmoDist;
        float dt = newT - t;
        t = newT;
        vec3 newPos = worldPos + t * worldDir;
        vec3 rayleighScattering, extinction;
        float mieScattering;
        getScatteringAndExtinction(newPos, rayleighScattering, mieScattering, extinction);
        opticalDepth += dt * extinction;
    }
    return opticalDepth;
}

float miePhase(float g, float cosTheta)
{
    float g2 = g * g;
#ifdef USE_CORNETTE_SHANKS_MIE_PHASE
    float k = 3.0 / (8.0 * PI) * (1.0 - g2) / (2.0 + g2);
    return k * (1.0 + cosTheta * cosTheta) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
#else
    float denom = 1.0 + g2 + 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * PI * denom * sqrt(denom));
#endif
}

float rayleighPhase(float cosTheta)
{
    float k = 3.0 / (16.0 * PI);
    return k * (1.0 + cosTheta * cosTheta);
}

#ifndef NO_TRANSMITTANCE_LUT_TEXTURE
void rayMarchAtmosphere(
    in float pixelNoise,
    in vec3 worldPos,
    in vec3 worldDir,
    in vec3 sunDir,
    in float sampleCount,
    in float tMaxMax,
#ifdef RAY_MARCHING_WITH_DEPTH
    in float sceneDepth,
    in float tDepth,
#endif
    out vec3 lum,
    out vec3 fms,
    out vec3 trans)
{
    lum = vec3(0.0), fms = vec3(0.0), trans = vec3(1.0);
    float tGround = rayIntersectSphere(worldPos, worldDir, uGroundRadius);
    float tAtmosphere = rayIntersectSphere(worldPos, worldDir, uAtmosphereRadius);
    float tMax = 0.0;

    if (tAtmosphere < 0) return;
    else if (tGround < 0) tMax = tAtmosphere;
    else tMax = max(0.0, tGround);
#ifdef RAY_MARCHING_WITH_DEPTH
    if (sceneDepth < 1.0)
        tMax = min(tDepth, tMax);
#endif
    tMax = min(tMax, tMaxMax);

#ifdef VARIABLE_SAMPLE_COUNT_ENABLE
    sampleCount = mix(RAY_MARCHING_MIN_SPP, RAY_MARCHING_MAX_SPP, clamp(tMax * 0.01, 0.0, 1.0));
    float sampleCountFloor = floor(sampleCount);
    float tMaxFloor = tMax * sampleCountFloor / sampleCount;
#endif

    const float uniformPhase = 1.0 / (4.0 * PI);
    const float cosTheta = dot(sunDir, worldDir);
    const float miePhaseValue = miePhase(0.8, -cosTheta);
    const float rayleighPhaseValue = rayleighPhase(cosTheta);

    float t = 0.0, dt = tMax / sampleCount;
    for (float s = 0.0; s < sampleCount; s += 1.0)
    {
#ifdef VARIABLE_SAMPLE_COUNT_ENABLE
        float t0 = s / sampleCountFloor;
        float t1 = (s + 1.0) / sampleCountFloor;
        t0 *= t0;
        t1 *= t1;
        t0 = tMaxFloor * t0;
        t1 = t1 > 1.0 ? tMax : tMaxFloor * t1;
        t = t0 + (t1 - t0) * pixelNoise;
        dt = t1 - t0;
#else
        float newT = tMax * (s + pixelNoise) / sampleCount;
        dt = newT - t;
        t = newT;
#endif

        vec3 newPos = worldPos + t * worldDir;
        vec3 rayleighScattering, extinction;
        float mieScattering;
        getScatteringAndExtinction(newPos, rayleighScattering, mieScattering, extinction);
        vec3 scattering = mieScattering + rayleighScattering;
        vec3 sampleTransmittance = exp(-extinction * dt);
        float viewHeight = length(newPos);
        vec3 upVector = newPos / viewHeight;
        float sunZenithCos = dot(sunDir, upVector);
        vec2 uv;
        transmittanceLutParametersToUV(viewHeight, sunZenithCos, uv);
        vec3 transmittanceToSun = textureLod(uTransmittanceLutTexture, uv, 0.0).rgb;

#ifdef MIE_RAYLEIGH_PHASE_ENABLE
        vec3 phaseTimesScattering = mieScattering * miePhaseValue + rayleighScattering * rayleighPhaseValue;
#else
        vec3 phaseTimesScattering = scattering * uniformPhase;
#endif

        float tEarth = rayIntersectSphere(newPos, sunDir, PLANET_RADIUS_OFFSET * upVector, uGroundRadius);
        float earthShadow = step(tEarth, 0.0);

#ifdef MULTISCATTERING_APPROX_SAMPLING_ENABLED
        vec3 multiScattering = textureLod(uMultiScatteringLutTexture, vec2(sunZenithCos * 0.5 + 0.5, (viewHeight - uGroundRadius) / (uAtmosphereRadius - uGroundRadius)), 0.0).rgb;
#else
        vec3 multiScattering = vec3(0.0);
#endif

#ifdef ILLUMINANCE_IS_ONE
        vec3 globalL = vec3(1.0);
#else
        vec3 globalL = uSunIntensity;
#endif
        vec3 S = globalL * (earthShadow * transmittanceToSun * phaseTimesScattering + multiScattering * scattering);
        vec3 Sint = (S - S * sampleTransmittance) / extinction;
        lum += trans * Sint;
        fms += trans * scattering * 1.0 * dt;
        //fms += trans * (scattering - scattering * sampleTransmittance) / extinction;
        trans *= sampleTransmittance;
    }

#ifdef GROUND_ALBEDO_ENABLE
    if (tMax == tGround && tGround > 0.0)
    {
        vec3 newPos = worldPos + tGround * worldDir;
        float viewHeight = length(newPos);
        vec3 upVector = newPos / viewHeight;
        float sunZenithCos = dot(sunDir, upVector);
        vec2 uv;
        transmittanceLutParametersToUV(viewHeight, sunZenithCos, uv);
        vec3 transmittanceToSun = textureLod(uTransmittanceLutTexture, uv, 0.0).rgb;
        float ndl = max(dot(upVector, sunDir), 0.0);
#ifdef ILLUMINANCE_IS_ONE
        vec3 globalL = vec3(1.0);
#else
        vec3 globalL = uSunIntensity;
#endif
        lum += globalL * transmittanceToSun * trans * ndl * uGroundAlbedo / PI;
    }
#endif
}
#endif