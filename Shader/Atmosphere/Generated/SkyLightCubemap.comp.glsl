#version 460 core
#extension GL_GOOGLE_include_directive : enable
#line 1 "SkyLightCubemap.comp.glsl"

#extension GL_GOOGLE_include_directive : enable
layout(local_size_x = 32, local_size_y = 32)in;
layout(rgba16f, binding = 0)uniform imageCube uSkyLightCubemapImage;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;

layout(std140, binding = 0)uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uWorldToEnuMatrix;
    vec2 uNearFarPlane1;
    vec2 uNearFarPlane2;
    vec2 uTotalNearFarPlane;
};



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


    sampleCount = mix(4, 32, clamp(tMax * 0.01, 0.0, 1.0));
    float sampleCountFloor = floor(sampleCount);
    float tMaxFloor = tMax * sampleCountFloor / sampleCount;

    const float uniformPhase = 1.0 /(4.0 * PI);
    const float cosTheta = dot(sunDir, worldDir);
    const float miePhaseValue = miePhase(0.8, - cosTheta);
    const float rayleighPhaseValue = rayleighPhase(cosTheta);

    float t = 0.0, dt = tMax / sampleCount;
    for(float s = 0.0;s < sampleCount;s += 1.0)
    {

        float t0 = s / sampleCountFloor;
        float t1 =(s + 1.0)/ sampleCountFloor;
        t0 *= t0;
        t1 *= t1;
        t0 = tMaxFloor * t0;
        t1 = t1 > 1.0 ? tMax : tMaxFloor * t1;
        t = t0 +(t1 - t0)* pixelNoise;
        dt = t1 - t0;





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

        vec3 phaseTimesScattering = mieScattering * miePhaseValue + rayleighScattering * rayleighPhaseValue;



        float tEarth = rayIntersectSphere(newPos, sunDir, 0.001 * upVector, uGroundRadius);
        float earthShadow = step(tEarth, 0.0);

        vec3 multiScattering = texture(uMultiScatteringLutTexture, vec2(sunZenithCos * 0.5 + 0.5,(viewHeight - uGroundRadius)/(uAtmosphereRadius - uGroundRadius))). rgb;






        vec3 S = earthShadow * transmittanceToSun * phaseTimesScattering + multiScattering * scattering;
        lum += trans *(S - S * sampleTransmittance)/ extinction;
        fms += trans *(scattering - scattering * sampleTransmittance)/ extinction;
        trans *= sampleTransmittance;
    }














}

#line 23 "SkyLightCubemap.comp.glsl"


vec3 getSamplingVector()
{
    vec2 st = vec2(gl_GlobalInvocationID . xy + 0.5)/ vec2(imageSize(uSkyLightCubemapImage));
    vec2 uv = 2.0 * vec2(st . x, 1.0 - st . y)- 1.0;
    vec3 dir[6]= {
  vec3(1.0, uv . y, - uv . x),
  vec3(- 1.0, uv . y, uv . x),
  vec3(uv . x, 1.0, - uv . y),
  vec3(uv . x, - 1.0, uv . y),
  vec3(uv . x, uv . y, 1.0),
  vec3(- uv . x, uv . y, - 1.0)
 };
    return normalize(dir[gl_GlobalInvocationID . z]);
}


vec3 worldToCubemap(vec3 dir)
{
    return vec3(dir . x, dir . z, - dir . y);
}


vec3 cubemapToWorld(vec3 dir)
{
    return vec3(dir . x, - dir . z, dir . y);
}

void main()
{
    vec3 worldPos = uInverseViewMatrix[3]. xyz / 1000.0;
    float viewHeight = length(worldPos);
    vec3 worldDir = transpose(mat3(uWorldToEnuMatrix))* cubemapToWorld(getSamplingVector());
    float distanceToGround = rayIntersectSphere(worldPos, worldDir, uGroundRadius);
    vec3 groundNormal = normalize(worldPos + distanceToGround + worldDir);
    vec3 groundColor = uGroundAlbedo * step(0.0, distanceToGround)* max(dot(groundNormal, uSunDirection), 0.0);
    vec3 lum, fms, trans;
    rayMarchAtmosphere(0.3, worldPos, worldDir, uSunDirection, 30.0, 9000000.0, lum, fms, trans);
    imageStore(uSkyLightCubemapImage, ivec3(gl_GlobalInvocationID), vec4(lum * uSunIntensity + groundColor, 1.0));
}
