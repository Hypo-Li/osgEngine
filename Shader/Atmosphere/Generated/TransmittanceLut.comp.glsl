#version 460 core
#extension GL_GOOGLE_include_directive : enable
#line 1 "TransmittanceLut.comp.glsl"

#extension GL_GOOGLE_include_directive : enable
layout(local_size_x = 32, local_size_y = 32)in;
layout(rgba16f, binding = 0)uniform image2D uTransmittanceLutImage;


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
            vec3 upOffset = upVector * -(0.001 * 100.0);
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



















































































































#line 8 "TransmittanceLut.comp.glsl"



void main()
{
    vec2 uv =(gl_GlobalInvocationID . xy + 0.5)/ vec2(256, 64);
    float viewHeight, viewZenithCos;
    uvToTransmittanceLutParameters(uv, viewHeight, viewZenithCos);
    vec3 worldPos = vec3(0.0, 0.0, viewHeight);
    vec3 worldDir = vec3(0.0, sqrt(1.0 - viewZenithCos * viewZenithCos), viewZenithCos);
    vec3 transmittance = exp(- getOpticalDepth(worldPos, worldDir, 40.0));
    imageStore(uTransmittanceLutImage, ivec2(gl_GlobalInvocationID . xy), vec4(transmittance, 1.0));
}
