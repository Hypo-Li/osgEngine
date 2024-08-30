#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uEarthColorTexture;
uniform sampler2D uEarthDepthTexture;
uniform sampler2D uGBufferColorTexture;
uniform sampler2D uGBufferDepthTexture;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    vec4 uEarthNearFar;
    vec4 uGBufferNearFar;
    vec4 uForwardOpaqueNearFar;
    vec4 uTotalNearFar;
};

#define MAX_DISTANCE 4294967296.0

float convertDepthToDistance(float depth, float near, float far)
{
    return mix(
        -2.0 * near * far / ((2.0 * depth - 1.0) * (far - near) - (far + near)),
        MAX_DISTANCE,
        depth == 1.0
    );
}

float convertDistanceToDepth(float dist, float near, float far)
{
    return (-2.0 * near * far / dist + far + near) / (far - near) * 0.5 + 0.5;
}

void main()
{
    vec4 earthColor = textureLod(uEarthColorTexture, uv, 0);
    float earthDepth = textureLod(uEarthDepthTexture, uv, 0).r;
    vec4 gbufferColor = textureLod(uGBufferColorTexture, uv, 0);
    float gbufferDepth = textureLod(uGBufferDepthTexture, uv, 0).r;
    float earthDist = convertDepthToDistance(earthDepth, uEarthNearFar.x, uEarthNearFar.y);
    float gbufferDist = convertDepthToDistance(gbufferDepth, uGBufferNearFar.x, uGBufferNearFar.y);
    fragData = mix(earthColor, gbufferColor, bvec4(gbufferDist < earthDist));
    
}