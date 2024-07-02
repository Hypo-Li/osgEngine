//#version 460 core
#extension GL_GOOGLE_include_directive : enable
#line 1 "RayMarching.frag.glsl"

#extension GL_GOOGLE_include_directive : enable
in vec2 texcoord;
out vec4 fragData;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
uniform sampler2D uSkyViewLutTexture;
uniform sampler3D uAerialPerspectiveLutTexture;
uniform sampler2D uSceneDepthTexture;
uniform uint osg_FrameNumber;
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
            vec3 upOffset = upVector * - 0.1;
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

    in float sceneDepth,
    in float tDepth,




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

    if(sceneDepth < 1.0)
        tMax = min(tDepth, tMax);

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

#line 25 "RayMarching.frag.glsl"
 const float cosHalfApex = 0.99998;

float interleavedGradientNoise(vec2 uv, float frameId)
{
 uv += frameId *(vec2(47, 17)* 0.695);
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic . z * fract(dot(uv, magic . xy)));
}

void main()
{
    vec4 clipSpace = vec4(texcoord * 2.0 - 1.0, 1.0, 1.0);
    vec4 viewSpace = uInverseProjectionMatrix * clipSpace;

    vec3 worldDir = normalize(mat3(uInverseViewMatrix)* viewSpace . xyz);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3]. xyz);
    float viewHeight = length(worldPos);
    float sceneDepth = texture(uSceneDepthTexture, texcoord). r;
    bool intersectGround = rayIntersectSphere(worldPos, worldDir, uGroundRadius)>= 0.0;

    vec3 outLuminance = vec3(0.0);
    if(sceneDepth == 1.0)
    {
        float vdl = dot(worldDir, uSunDirection);
        vec3 upVector = worldPos / viewHeight;

        if(! intersectGround && vdl > cosHalfApex)
        {
            float viewZenithCos = max(dot(uSunDirection, upVector), 0.0);
            vec2 transmittanceLutUV;
            transmittanceLutParametersToUV(viewHeight, viewZenithCos, transmittanceLutUV);
            vec3 trans = texture(uTransmittanceLutTexture, transmittanceLutUV). rgb;
            float softEdge = clamp(2.0 *(vdl - cosHalfApex)/(1.0 - cosHalfApex), 0.0, 1.0);
            outLuminance += trans * uSunIntensity * 1000.0 * softEdge;
        }

        if(viewHeight < uAtmosphereRadius)
        {
            float viewZenithCos = dot(worldDir, upVector);
            vec3 sideVector = normalize(cross(upVector, worldDir));
            vec3 forwardVector = normalize(cross(sideVector, upVector));
            vec2 lightOnPlane = vec2(dot(uSunDirection, forwardVector), dot(uSunDirection, sideVector));
            lightOnPlane = normalize(lightOnPlane);
            float lightViewCos = lightOnPlane . x;

            vec2 skyViewUV;
            skyViewLutParametersToUV(intersectGround, viewZenithCos, lightViewCos, viewHeight, skyViewUV);
            outLuminance += texture(uSkyViewLutTexture, skyViewUV). rgb * uSunIntensity;
            fragData = vec4(outLuminance, 1.0);
            return;
        }
    }

    if(! moveToTopAtmosphere(worldPos, worldDir))
    {
        fragData = vec4(outLuminance, 1.0);
        return;
    }

    clipSpace = vec4(texcoord * 2.0 - 1.0, sceneDepth * 2.0 - 1.0, 1.0);
    viewSpace = uInverseProjectionMatrix * clipSpace;
    viewSpace *= 1.0 / viewSpace . w;
    float tDepth = abs(viewSpace . z)/ 1000.0;

    if((viewHeight - uGroundRadius)<(uAtmosphereRadius - uGroundRadius)* 0.05)
    {
        float slice = aerialPerspectiveDepthToSlice(tDepth);
        float weight = 1.0;
        if(slice < 0.5)
        {
            weight = clamp(slice * 2.0, 0.0, 1.0);
            slice = 0.5;
        }
        float w = sqrt(slice / 32.0);
        vec4 AP = weight * texture(uAerialPerspectiveLutTexture, vec3(texcoord, w));
        fragData = vec4(AP . rgb * uSunIntensity, 1.0 - AP . a);
    }
    else
    {
        vec3 lum, fms, trans;
        rayMarchAtmosphere(interleavedGradientNoise(gl_FragCoord . xy, osg_FrameNumber % 8), worldPos, worldDir, uSunDirection, 0.0, 9000000, sceneDepth, tDepth, lum, fms, trans);
        fragData = vec4(lum * uSunIntensity, dot(trans, vec3(0.3333333)));
        //fragData = vec4(lum, 0.0);
    }
}
