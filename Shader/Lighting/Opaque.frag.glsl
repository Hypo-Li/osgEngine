#version 430 core
in vec2 uv;
out vec4 fragData;

uniform sampler2D uGBufferATexture;
uniform sampler2D uGBufferBTexture;
uniform sampler2D uGBufferCTexture;
uniform sampler2D uGBufferDTexture;
uniform sampler2D uDepthTexture;

#extension GL_GOOGLE_include_directive : enable
#include "Common.glsl"

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uReprojectionMatrix;
    vec2 uJitterPixels;
    vec2 uPrevJitterPixels;
};

void calcPixelParameters(vec4 gbufferA, vec4 gbufferB, vec4 gbufferC, vec4 gbufferD, float fragDepth, out PixelParameters pp)
{
    pp.shadingModel = uint(gbufferB.a * 255.0);
    pp.baseColor = pow(gbufferC.rgb, vec3(2.2));
    pp.metallic = gbufferB.r;
    pp.roughness = max(gbufferB.g, 0.05);
    float a = pp.roughness * pp.roughness;
    float a2 = a * a;
    pp.specular = gbufferB.b;
    pp.normalWS = normalize(gbufferA.rgb * 2.0 - 1.0);
    pp.occlusion = gbufferC.a;
    vec4 fragPosNdc = vec4(uv * 2.0 - 1.0, fragDepth, 1.0);;
    vec4 fragPosVS = uInverseProjectionMatrix * fragPosNdc;
    pp.fragPosVS = fragPosVS.xyz * (1.0 / fragPosVS.w);
    pp.viewDirWS = mat3(uInverseViewMatrix) * normalize(-pp.fragPosVS);
    pp.NoV = max(dot(pp.normalWS, pp.viewDirWS), 0.0);
    pp.reflDirWS = reflect(-pp.viewDirWS, pp.normalWS);
    pp.reflDirWS = mix(pp.reflDirWS, pp.normalWS, a2);
    pp.F0 = mix(vec3(pp.specular * 0.08), pp.baseColor, pp.metallic);
}

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);
    float fragDepth = texelFetch(uDepthTexture, iTexcoord, 0).r;
    // Reversed Z
    if (fragDepth == 0.0)
    {
        //discard;
        fragData = vec4(1, 1, 1, 1.0);
        return;
    }
    vec4 gbufferA = texelFetch(uGBufferATexture, iTexcoord, 0);
    vec4 gbufferB = texelFetch(uGBufferBTexture, iTexcoord, 0);
    vec4 gbufferC = texelFetch(uGBufferCTexture, iTexcoord, 0);
    vec4 gbufferD = texelFetch(uGBufferDTexture, iTexcoord, 0);

    PixelParameters pp;
    calcPixelParameters(gbufferA, gbufferB, gbufferC, gbufferD, fragDepth, pp);

    vec3 color = evaluateLighs(pp);

    fragData = vec4(color, 1.0);
}