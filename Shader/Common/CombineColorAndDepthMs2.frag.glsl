#version 430 core
in vec2 uv;
out vec4 fragData;
uniform sampler2D uDepthTexture;
uniform sampler2DMS uColorMsTexture;
uniform sampler2DMS uDepthMsTexture;
uniform sampler2D uEarthDepthTexture;

layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    vec4 uEarthNearFar;
    vec4 uGbufferNearFar;
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

vec4 convertDepthToDistance(vec4 depth, vec4 near, vec4 far)
{
    return mix(
        -2.0 * near * far / ((2.0 * depth - 1.0) * (far - near) - (far + near)),
        vec4(MAX_DISTANCE),
        equal(depth, vec4(1.0))
    );
}

float convertDistanceToDepth(float dist, float near, float far)
{
    return (-2.0 * near * far / dist + far + near) / (far - near) * 0.5 + 0.5;
}

void main()
{
    ivec2 iTexcoord = ivec2(gl_FragCoord.xy);

    float depth = texelFetch(uDepthTexture, iTexcoord, 0).r;
    float depthMs0 = texelFetch(uDepthMsTexture, iTexcoord, 0).r;
    float depthMs1 = texelFetch(uDepthMsTexture, iTexcoord, 1).r;
    float depthMs2 = texelFetch(uDepthMsTexture, iTexcoord, 2).r;
    float depthMs3 = texelFetch(uDepthMsTexture, iTexcoord, 3).r;
    float minDepth = min(min(depthMs0, depthMs1), min(depthMs2, depthMs3));
    if (depth == 1.0 && minDepth == 1.0)
    {
        fragData = vec4(0);
        gl_FragDepth = 1.0;
        return;
    }

    float dist = convertDepthToDistance(depth, uGbufferNearFar.x, uGbufferNearFar.y);

    vec4 colorMs0 = texelFetch(uColorMsTexture, iTexcoord, 0);
    vec4 colorMs1 = texelFetch(uColorMsTexture, iTexcoord, 1);
    vec4 colorMs2 = texelFetch(uColorMsTexture, iTexcoord, 2);
    vec4 colorMs3 = texelFetch(uColorMsTexture, iTexcoord, 3);

    vec4 depthMs = vec4(depthMs0, depthMs1, depthMs2, depthMs3);
    vec4 distMs = convertDepthToDistance(depthMs, vec4(uForwardOpaqueNearFar.x), vec4(uForwardOpaqueNearFar.y));

    vec4 alphas = mix(step(distMs, vec4(dist)), vec4(0), equal(depthMs, vec4(1.0)));
    vec4 outColor = 0.25 * vec4(
		alphas.x * colorMs0.rgb +
		alphas.y * colorMs1.rgb +
		alphas.z * colorMs2.rgb +
		alphas.w * colorMs3.rgb,
		alphas.x + alphas.y + alphas.z + alphas.w
	);

    float outDist = min(min(distMs.x, distMs.y), min(distMs.z, distMs.w));

    float earthDepth = texelFetch(uEarthDepthTexture, iTexcoord, 0).r;
    float earthDist = convertDepthToDistance(earthDepth, uEarthNearFar.x, uEarthNearFar.y);
    if (outDist >= earthDist)
        outColor = vec4(0);

    float outNear = min(uGbufferNearFar.x, uForwardOpaqueNearFar.x);
    float outFar = max(uGbufferNearFar.y, uForwardOpaqueNearFar.y);
    float outDepth = convertDistanceToDepth(min(dist, outDist), outNear, outFar);

    fragData = outColor;
    gl_FragDepth = outDepth;
}