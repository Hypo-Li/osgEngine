#version 430 core
in vec2 uv;
out vec4 fragData;

uniform sampler2D uGBufferATexture;
uniform sampler2D uGBufferBTexture;
uniform sampler2D uGBufferCTexture;
uniform sampler2D uGBufferDTexture;
uniform sampler2D uDepthTexture;

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
    vec3 viewDirVS;
    vec3 viewDirWS;
};

void calcPixelParameters(vec4 gbufferA, vec4 gbufferB, vec4 gbufferC, vec4 gbufferD, float fragDepth, out PixelParameters pp)
{
    pp.shadingModel = uint(gbufferB.a * 255.0);
    pp.baseColor = gbufferC.rgb;
    pp.metallic = gbufferB.r;
    pp.roughness = gbufferB.g;
    pp.specular = gbufferB.b;
    pp.normalWS = gbufferA.rgb * 2.0 - 1.0;
    pp.occlusion = gbufferC.a;
    pp.fragDepth = fragDepth;
    vec4 fragPosNdc = vec4(vec3(uv, fragDepth) * 2.0 - 1.0, 1.0);
    vec4 fragPosVS = uInverseProjectionMatrix * fragPosNdc;
    pp.fragPosVS = fragPosVS.xyz * (1.0 / fragPosVS.w);
    pp.viewDirVS = normalize(-pp.fragPosVS);
    pp.viewDirWS = mat3(uInverseViewMatrix) * pp.viewDirVS;
}

vec3 evaluateDirectionalLight(in PixelParameters pp, vec3 color, vec3 direction)
{
    vec3 lightDirWS = normalize(-direction);
    float ndl = max(dot(pp.normalWS, lightDirWS), 0.0);
    return color * ndl * pp.baseColor;
}

vec3 evaluateLighting(in PixelParameters pp)
{
    if (pp.shadingModel == 0)
        return vec3(0);
    
    vec3 color = vec3(0);
    for (uint i = 0; i < uDirectionalLightCount; ++i)
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

    vec3 color = evaluateLighting(pp);

    fragData = vec4(color, 1.0);
}