#version 430 core
in vec2 uv;
out vec4 fragData;

uniform sampler2D uGBufferATexture;
uniform sampler2D uGBufferBTexture;
uniform sampler2D uGBufferCTexture;
uniform sampler2D uGBufferDTexture;
uniform sampler2D uDepthTexture;

#define PI 3.1415926

#define MAX_DIRECTIONAL_LIGHT_COUNT 2
struct DirectionalLight
{
    vec3 color;
    vec3 direction;
};
uniform DirectionalLight uDirectionalLight[MAX_DIRECTIONAL_LIGHT_COUNT];
uniform uint uDirectionalLightCount;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
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
    float NoV;
    vec3 reflDirWS;
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
    // 1.0 - vdh maybe equal to -0.0, pow will return nan
    return F0 + (1.0 - F0) * pow(abs(1.0 - VoH), 5.0);
}

vec3 F_Schlick(float VoH, vec3 F0, float F90)
{
    // 1.0 - vdh maybe equal to -0.0, pow will return nan
    return F0 + (F90 - F0) * pow(abs(1.0 - VoH), 5.0);
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
    vec3 diffuse = pp.baseColor * (1.0 - F) * (1.0 - pp.metallic) / PI;
    vec3 specular = F * D * V;
    vec3 color = (diffuse + specular) * NoL * lightColor;

    return color;
}

vec3 evaluateLighting(in PixelParameters pp)
{
    if (pp.shadingModel == 0)
        return vec3(0);
    
    vec3 color = vec3(0);
    uint directionalLightCount = min(uDirectionalLightCount, MAX_DIRECTIONAL_LIGHT_COUNT);
    for (uint i = 0; i < directionalLightCount; ++i)
    {
        color += evaluateDirectionalLight(pp, uDirectionalLight[i].color, uDirectionalLight[i].direction);
    }
    return color;
}

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    vec4 gbufferA = texelFetch(uGBufferATexture, iTexcoord, 0);
    vec4 gbufferB = texelFetch(uGBufferBTexture, iTexcoord, 0);
    vec4 gbufferC = texelFetch(uGBufferCTexture, iTexcoord, 0);
    vec4 gbufferD = texelFetch(uGBufferDTexture, iTexcoord, 0);
    float fragDepth = texelFetch(uDepthTexture, iTexcoord, 0).r;

    PixelParameters pp;
    calcPixelParameters(gbufferA, gbufferB, gbufferC, gbufferD, fragDepth, pp);

    if (pp.fragDepth == 1.0)
        discard;

    vec3 color = evaluateLighting(pp);

    fragData = vec4(color, 1.0);
}