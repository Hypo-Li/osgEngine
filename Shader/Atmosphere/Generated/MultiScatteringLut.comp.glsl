#version 460 core
#extension GL_GOOGLE_include_directive : enable
#line 1 "MultiScatteringLut.comp.glsl"

#extension GL_GOOGLE_include_directive : enable
layout(local_size_x = 1, local_size_y = 1, local_size_z = 64)in;
layout(rgba16f, binding = 0)uniform image2D uMultiScatteringLutImage;
uniform sampler2D uTransmittanceLutTexture;


#line 1 "AtmosphereCommon.glsl"
































const float PI = 3.14159265358979323846;

layout(std140, binding = 1)uniform AtmosphereParameters
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

vec3 getWorldPos(vec3 pos)
{



    return pos / 1000.0;

}

float aerialPerspectiveDepthToSlice(float depth)
{
    return depth *(1.0f / 4.0);
}

float aerialPerspectiveSliceToDepth(float slice)
{
    return slice * 4.0;
}



float rayIntersectSphere(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius)
{
    vec3 localPos = rayOrigin - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, localPos);
    float c = dot(localPos, localPos)-(sphereRadius * sphereRadius);
    float delta = b * b - 4.0 * a * c;
    if(delta < 0.0 || a == 0.0)return - 1.0;
    float sol0 =(- b - sqrt(delta))/(2.0 * a);
    float sol1 =(- b + sqrt(delta))/(2.0 * a);
    if(sol0 < 0.0 && sol1 < 0.0)return - 1.0;
    if(sol0 < 0.0)return max(0.0, sol1);
    else if(sol1 < 0.0)return max(0.0, sol0);
    return max(0.0, min(sol0, sol1));
}

float rayIntersectSphere(vec3 rayOrigin, vec3 rayDir, float sphereRadius)
{
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, rayOrigin);
    float c = dot(rayOrigin, rayOrigin)-(sphereRadius * sphereRadius);
    float delta = b * b - 4.0 * a * c;
    if(delta < 0.0 || a == 0.0)return - 1.0;
    float sol0 =(- b - sqrt(delta))/(2.0 * a);
    float sol1 =(- b + sqrt(delta))/(2.0 * a);
    if(sol0 < 0.0 && sol1 < 0.0)return - 1.0;
    if(sol0 < 0.0)return max(0.0, sol1);
    else if(sol1 < 0.0)return max(0.0, sol0);
    return max(0.0, min(sol0, sol1));
}

bool moveToTopAtmosphere(inout vec3 worldPos, in vec3 worldDir)
{
    float viewHeight = length(worldPos);
    if(viewHeight > uAtmosphereRadius)
    {
        float tTop = rayIntersectSphere(worldPos, worldDir, uAtmosphereRadius);
        if(tTop >= 0.0)
        {
            vec3 upVector = vec3(worldPos / viewHeight);
            vec3 upOffset = upVector * - 0.001;
            worldPos = worldPos + worldDir * tTop + upOffset;
        }
        else

            return false;
    }
    return true;
}

void uvToTransmittanceLutParameters(in vec2 uv, out float viewHeight, out float viewZenithCos)
{
    float H = sqrt(uAtmosphereRadius * uAtmosphereRadius - uGroundRadius * uGroundRadius);
    float rho = H * uv . y;
    viewHeight = sqrt(rho * rho + uGroundRadius * uGroundRadius);
    float dMin = uAtmosphereRadius - viewHeight;
    float dMax = rho + H;
    float d = dMin + uv . x *(dMax - dMin);
    viewZenithCos = d == 0.0 ? 1.0 :(H * H - rho * rho - d * d)/(2.0 * viewHeight * d);
    viewZenithCos = clamp(viewZenithCos, - 1.0, 1.0);
}

void transmittanceLutParametersToUV(in float viewHeight, in float viewZenithCos, out vec2 uv)
{
    float H = sqrt(max(0.0f, uAtmosphereRadius * uAtmosphereRadius - uGroundRadius * uGroundRadius));
    float rho = sqrt(max(0.0f, viewHeight * viewHeight - uGroundRadius * uGroundRadius));
    float discriminant = viewHeight * viewHeight *(viewZenithCos * viewZenithCos - 1.0)+ uAtmosphereRadius * uAtmosphereRadius;
    float d = max(0.0,(- viewHeight * viewZenithCos + sqrt(discriminant)));
    float dMin = uAtmosphereRadius - viewHeight;
    float dMax = rho + H;
    float u =(d - dMin)/(dMax - dMin);
    float v = rho / H;
    uv = vec2(u, v);
}

void uvToSkyViewLutParameters(in float viewHeight, in vec2 uv, out float viewZenithCos, out float lightViewCos)
{
    float vHorizon = sqrt(viewHeight * viewHeight - uGroundRadius * uGroundRadius);
    float cosBeta = float(vHorizon / viewHeight);
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;
    if(uv . y < 0.5)
    {
        float coord = 2.0 * uv . y;
        coord = 1.0 - coord;
        coord *= coord;
        coord = 1.0 - coord;
        viewZenithCos = cos(zenithHorizonAngle * coord);
    }
    else
    {
        float coord = uv . y * 2.0 - 1.0;
        coord *= coord;
        viewZenithCos = cos(zenithHorizonAngle + beta * coord);
    }
    float coord = uv . x;
    coord *= coord;
    lightViewCos = -(coord * 2.0 - 1.0);
}

void skyViewLutParametersToUV(in bool intersectGround, in float viewZenithCos, in float lightViewCos, in float viewHeight, out vec2 uv)
{
    float vHorizon = sqrt(viewHeight * viewHeight - uGroundRadius * uGroundRadius);
    float cosBeta = float(vHorizon / viewHeight);
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;
    if(! intersectGround)
    {
        float coord = acos(viewZenithCos)/ zenithHorizonAngle;
        coord = 1.0 - coord;
        coord = sqrt(coord);
        coord = 1.0 - coord;
        uv . y = coord * 0.5;
    }
    else
    {
        float coord =(acos(viewZenithCos)- zenithHorizonAngle)/ beta;
        coord = sqrt(coord);
        uv . y = coord * 0.5 + 0.5;
    }
    float coord = - lightViewCos * 0.5 + 0.5;
    coord = sqrt(coord);
    uv . x = coord;
}

void getScatteringAndExtinction(in vec3 worldPos, out vec3 rayleighScattering, out float mieScattering, out vec3 extinction)
{
    float altitude = length(worldPos)- uGroundRadius;
    float rayleighDensity = exp(- altitude / uRayleighDensityH);
    rayleighScattering = uRayleighScatteringBase * rayleighDensity;
    float mieDensity = exp(- altitude / uMieDensityH);
    mieScattering = uMieScatteringBase * mieDensity;
    float mieAbsorption = uMieAbsorptionBase * mieDensity;
    vec3 ozoneAbsorption = uOzoneAbsorptionBase * max(0.0, 1.0 - abs(altitude - uOzoneHeight)/ uOzoneThickness);
    extinction = rayleighScattering + mieScattering + mieAbsorption + ozoneAbsorption;
}

vec3 getOpticalDepth(vec3 worldPos, vec3 worldDir, float sampleCount)
{
    if(rayIntersectSphere(worldPos, worldDir, uGroundRadius)> 0.0)
        return vec3(0.0);
    float atmoDist = rayIntersectSphere(worldPos, worldDir, uAtmosphereRadius);
    float t = 0.0;
    vec3 opticalDepth = vec3(0.0);
    for(float s = 0.0;s < sampleCount;s += 1.0)
    {
        float newT =((s + 0.3)/ sampleCount)* atmoDist;
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




    float denom = 1.0 + g2 + 2.0 * g * cosTheta;
    return(1.0 - g2)/(4.0 * PI * denom * sqrt(denom));

}

float rayleighPhase(float cosTheta)
{
    float k = 3.0 /(16.0 * PI);
    return k *(1.0 + cosTheta * cosTheta);
}


void rayMarchAtmosphere(
    in float pixelNoise,
    in vec3 worldPos,
    in vec3 worldDir,
    in vec3 sunDir,
    in float sampleCount,
    in float tMaxMax,







    out vec3 lum,
    out vec3 fms,
    out vec3 trans)
{



    lum = vec3(0.0), fms = vec3(0.0), trans = vec3(1.0);
    float tGround = rayIntersectSphere(worldPos, worldDir, uGroundRadius);
    float tAtmosphere = rayIntersectSphere(worldPos, worldDir, uAtmosphereRadius);
    float tMax = 0.0;




    if(tAtmosphere < 0)return;
    else if(tGround < 0)tMax = tAtmosphere;
    else tMax = max(0.0, tGround);




    tMax = min(tMax, tMaxMax);






    const float uniformPhase = 1.0 /(4.0 * PI);
    const float cosTheta = dot(sunDir, worldDir);
    const float miePhaseValue = miePhase(0.8, - cosTheta);
    const float rayleighPhaseValue = rayleighPhase(cosTheta);

    float t = 0.0, dt = tMax / sampleCount;
    for(float s = 0.0;s < sampleCount;s += 1.0)
    {










        float newT = tMax *(s + pixelNoise)/ sampleCount;
        dt = newT - t;
        t = newT;

        vec3 newPos = worldPos + t * worldDir;
        vec3 rayleighScattering, extinction;
        float mieScattering;
        getScatteringAndExtinction(newPos, rayleighScattering, mieScattering, extinction);
        vec3 scattering = mieScattering + rayleighScattering;
        vec3 sampleTransmittance = exp(- extinction * dt);
        float viewHeight = length(newPos);
        vec3 upVector = newPos / viewHeight;
        float sunZenithCos = dot(sunDir, upVector);
        vec2 uv;
        transmittanceLutParametersToUV(viewHeight, sunZenithCos, uv);
        vec3 transmittanceToSun = texture(uTransmittanceLutTexture, uv). rgb;



        vec3 phaseTimesScattering = scattering * uniformPhase;

        float tEarth = rayIntersectSphere(newPos, sunDir, 0.001 * upVector, uGroundRadius);
        float earthShadow = step(tEarth, 0.0);



        vec3 multiScattering = vec3(0.0);




        vec3 S = earthShadow * transmittanceToSun * phaseTimesScattering + multiScattering * scattering;
        lum += trans *(S - S * sampleTransmittance)/ extinction;
        fms += trans *(scattering - scattering * sampleTransmittance)/ extinction;
        trans *= sampleTransmittance;
    }

    if(tMax == tGround && tGround > 0.0)
    {
        vec3 newPos = worldPos + tGround * worldDir;
        float viewHeight = length(newPos);
        vec3 upVector = newPos / viewHeight;
        float sunZenithCos = dot(sunDir, upVector);
        vec2 uv;
        transmittanceLutParametersToUV(viewHeight, sunZenithCos, uv);
        vec3 transmittanceToSun = texture(uTransmittanceLutTexture, uv). rgb;
        float ndl = max(dot(upVector, sunDir), 0.0);
        lum += transmittanceToSun * trans * ndl * uGroundAlbedo / PI;
    }

}

#line 9 "MultiScatteringLut.comp.glsl"



shared vec3 lumMem[64];
shared vec3 fmsMem[64];

void main()
{
    vec2 uv =(gl_GlobalInvocationID . xy + 0.5)/ vec2(32, 32);
    float sunZenithCos = uv . x * 2.0 - 1.0;
    vec3 sunDir = vec3(0.0, sqrt(1.0 - sunZenithCos * sunZenithCos), sunZenithCos);
    float viewHeight = uGroundRadius + uv . y *(uAtmosphereRadius - uGroundRadius);
    vec3 worldPos = vec3(0.0, 0.0, viewHeight);
    vec3 worldDir = vec3(0.0, 0.0, 1.0);

    const float sqrtSample = 8;
    const float invSample = 1.0 /(sqrtSample * sqrtSample);
    uint z = gl_GlobalInvocationID . z;
    {
        float i = 0.5 + float(z / 8);
        float j = 0.5 + float(z - float((z / 8)* 8));
        float randA = i / sqrtSample;;
        float randB = j / sqrtSample;
        float theta = 2.0 * PI * randA;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);
        float phi = acos(1.0 - 2.0 * randB);
        float cosPhi = cos(phi);
        float sinPhi = sin(phi);
        worldDir . x = cosTheta * sinPhi;
        worldDir . y = sinTheta * sinPhi;
        worldDir . z = cosPhi;
        vec3 lum, fms, trans;
        rayMarchAtmosphere(0.3, worldPos, worldDir, sunDir, 20.0, 9000000.0, lum, fms, trans);
        lumMem[z]= lum * invSample;
        fmsMem[z]= fms * invSample;
    }
    barrier();
    if(z < 32)
    {
        lumMem[z]+= lumMem[z + 32];
        fmsMem[z]+= fmsMem[z + 32];
    }
    barrier();
    if(z < 16)
    {
        lumMem[z]+= lumMem[z + 16];
        fmsMem[z]+= fmsMem[z + 16];
    }
    barrier();
    if(z < 8)
    {
        lumMem[z]+= lumMem[z + 8];
        fmsMem[z]+= fmsMem[z + 8];
    }
    barrier();
    if(z < 4)
    {
        lumMem[z]+= lumMem[z + 4];
        fmsMem[z]+= fmsMem[z + 4];
    }
    barrier();
    if(z < 2)
    {
        lumMem[z]+= lumMem[z + 2];
        fmsMem[z]+= fmsMem[z + 2];
    }
    barrier();
    if(z < 1)
    {
        lumMem[z]+= lumMem[z + 1];
        fmsMem[z]+= fmsMem[z + 1];
    }
    barrier();
    if(z > 0)
        return;
    imageStore(uMultiScatteringLutImage, ivec2(gl_GlobalInvocationID . xy), vec4(lumMem[0]/(1.0 - fmsMem[0]), 1.0));
}
