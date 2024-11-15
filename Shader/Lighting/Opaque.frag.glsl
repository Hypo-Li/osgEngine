#version 430 core
in vec2 uv;
out vec4 fragData;

uniform sampler2D uGBufferATexture;
uniform sampler2D uGBufferBTexture;
uniform sampler2D uGBufferCTexture;
uniform sampler2D uGBufferDTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uBRDFLutTexture;

#define PI 3.14159265358

#define MAX_DIRECTIONAL_LIGHT_COUNT 2
struct DirectionalLight
{
    vec3 color;
    vec3 direction;
};
uniform DirectionalLight uDirectionalLight[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform uint uDirectionalLightCount;

uniform bool uEnableIBL;
uniform samplerCube uPrefilterTexture;
uniform vec4 uSHCoeff[9];

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    vec4 uNearFar;
};

struct PixelParameters
{
    uint shadingModel;
    vec3 baseColor;
    float metallic;
    float roughness;
    float specular;
    vec3 normalWS;
    float occlusion;
    float fragDepth;
    vec3 fragPosVS;
    vec3 viewDirWS;
    vec3 reflDirWS;
    float NoV;
    vec3 F0;
};

void calcPixelParameters(vec4 gbufferA, vec4 gbufferB, vec4 gbufferC, vec4 gbufferD, float fragDepth, out PixelParameters pp)
{
    pp.shadingModel = uint(gbufferB.a * 255.0);
    pp.baseColor = gbufferC.rgb;
    pp.metallic = gbufferB.r;
    pp.roughness = max(gbufferB.g, 0.05);
    float a = pp.roughness * pp.roughness;
    float a2 = a * a;
    pp.specular = gbufferB.b;
    pp.normalWS = normalize(gbufferA.rgb * 2.0 - 1.0);
    pp.occlusion = gbufferC.a;
    pp.fragDepth = fragDepth;
    vec4 fragPosNdc = vec4(vec3(uv, fragDepth) * 2.0 - 1.0, 1.0);
    vec4 fragPosVS = uInverseProjectionMatrix * fragPosNdc;
    pp.fragPosVS = fragPosVS.xyz * (1.0 / fragPosVS.w);
    pp.viewDirWS = mat3(uInverseViewMatrix) * normalize(-pp.fragPosVS);
    pp.NoV = max(dot(pp.normalWS, pp.viewDirWS), 0.0);
    pp.reflDirWS = reflect(-pp.viewDirWS, pp.normalWS);
    pp.reflDirWS = mix(pp.reflDirWS, pp.normalWS, a2);
    pp.F0 = mix(vec3(pp.specular * 0.08), pp.baseColor, pp.metallic);
}

vec3 F_Schlick(float VoH, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

vec3 F_Schlick(float VoH, vec3 F0, vec3 F90)
{
    return F0 + (F90 - F0) * pow(1.0 - VoH, 5.0);
}

float D_GGX(float NoH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return clamp(a2 / (PI * d * d), 0.0, 1000.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    return clamp(0.5 / (lambdaV + lambdaL), 0.0, 1.0);
}

vec3 evaluateDirectionalLight(in PixelParameters pp, vec3 lightColor, vec3 lightDirection)
{
    vec3 lightDirWS = normalize(-lightDirection);
    vec3 halfDirWS = normalize(lightDirWS + pp.viewDirWS);
    float NoL = max(dot(pp.normalWS, lightDirWS), 0.0);
    float NoH = max(dot(pp.normalWS, halfDirWS), 0.0);
    float VoH = max(dot(pp.viewDirWS, halfDirWS), 0.0);

    vec3 F = F_Schlick(VoH, pp.F0);
    float D = D_GGX(NoH, pp.roughness);
    float V = V_SmithGGXCorrelated(pp.NoV, NoL, pp.roughness);
    vec3 Fd = pp.baseColor * (1.0 - F) * (1.0 - pp.metallic) / PI;
    vec3 Fr = F * D * V;

    return (Fd + Fr) * NoL * lightColor;
}

vec3 calcSHLighting(vec3 N)
{
    vec3 result = vec3(0.0);
    result += uSHCoeff[0].xyz * 0.5 * sqrt(1.0 / PI);
    result -= uSHCoeff[1].xyz * 0.5 * sqrt(3.0 / PI) * N.y;
    result += uSHCoeff[2].xyz * 0.5 * sqrt(3.0 / PI) * N.z;
    result -= uSHCoeff[3].xyz * 0.5 * sqrt(3.0 / PI) * N.x;
    result += uSHCoeff[4].xyz * 0.5 * sqrt(15.0 / PI) * N.x * N.y;
    result -= uSHCoeff[5].xyz * 0.5 * sqrt(15.0 / PI) * N.y * N.z;
    result += uSHCoeff[6].xyz * 0.25 * sqrt(5.0 / PI) * (3.0 * N.z * N.z - 1.0);
    result -= uSHCoeff[7].xyz * 0.5 * sqrt(15.0 / PI) * N.x * N.z;
    result += uSHCoeff[8].xyz * 0.25 * sqrt(15.0 / PI) * (N.x * N.x - N.y * N.y);
    return result;
}

vec3 evaluateIBL(in PixelParameters pp)
{
    vec2 brdf = texture(uBRDFLutTexture, vec2(pp.NoV, pp.roughness)).rg;
    vec3 E = mix(brdf.xxx, brdf.yyy, pp.F0);
    vec3 Fd = calcSHLighting(pp.normalWS) * pp.baseColor * (1.0 - E) * (1.0 - pp.metallic);
    vec3 Fr = textureLod(uPrefilterTexture, pp.reflDirWS, pp.roughness * (2.0 - pp.roughness) * 4.0).rgb * E;
    vec3 energyCompensation = 1.0 + pp.F0 * (1.0 / brdf.y - 1.0);
    return Fd + Fr * energyCompensation;
}

vec3 evaluateLighs(in PixelParameters pp)
{
    if (pp.shadingModel == 0)
        return vec3(0);
    
    vec3 color = vec3(0);
    uint directionalLightCount = min(uDirectionalLightCount, MAX_DIRECTIONAL_LIGHT_COUNT);
    for (uint i = 0; i < directionalLightCount; ++i)
    {
        color += evaluateDirectionalLight(pp, uDirectionalLight[i].color, uDirectionalLight[i].direction);
    }
    if (uEnableIBL)
    {
        color += evaluateIBL(pp);
    }
    return color;
}

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    float fragDepth = texelFetch(uDepthTexture, iTexcoord, 0).r;
    if (fragDepth == 1.0)
        discard;
    vec4 gbufferA = texelFetch(uGBufferATexture, iTexcoord, 0);
    vec4 gbufferB = texelFetch(uGBufferBTexture, iTexcoord, 0);
    vec4 gbufferC = texelFetch(uGBufferCTexture, iTexcoord, 0);
    vec4 gbufferD = texelFetch(uGBufferDTexture, iTexcoord, 0);

    PixelParameters pp;
    calcPixelParameters(gbufferA, gbufferB, gbufferC, gbufferD, fragDepth, pp);

    vec3 color = evaluateLighs(pp);

    fragData = vec4(color, 1.0);
}