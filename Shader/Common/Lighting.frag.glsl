#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uGBufferATexture;
uniform sampler2D uGBufferBTexture;
uniform sampler2D uGBufferCTexture;
uniform sampler2D uGBufferDTexture;
uniform sampler2D uDepthTexture;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};

void main()
{
    vec4 gbufferA = textureLod(uGBufferATexture, uv, 0);
    vec4 gbufferB = textureLod(uGBufferBTexture, uv, 0);
    vec4 gbufferC = textureLod(uGBufferCTexture, uv, 0);
    vec4 gbufferD = textureLod(uGBufferDTexture, uv, 0);
    float depth = textureLod(uDepthTexture, uv, 0).r;

    vec3 normalWS = gbufferA.rgb * 2.0 - 1.0;
    vec3 normalVS = mat3(uViewMatrix) * normalWS;

    vec4 fragPosVS = uInverseProjectionMatrix * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    fragPosVS.xyz *= 1.0 / fragPosVS.w;
    vec3 viewDirVS = normalize(-fragPosVS.xyz);

    float ndv = max(dot(normalVS, viewDirVS), 0.0);

    fragData = vec4(gbufferC * ndv);
}