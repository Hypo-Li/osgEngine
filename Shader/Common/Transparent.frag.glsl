#version 430 core
in vec3 v2f_fragPos;
in vec3 v2f_normal;
in vec4 v2f_color;
in float v2f_earthNdcZ;
in float v2f_totalNdcZ;
out vec4 fragData;

uniform sampler2D uEarthDepthTexture;

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

void main()
{
    vec3 normal = normalize(v2f_normal);
    vec3 viewDir = normalize(-v2f_fragPos);
    float ndv = max(dot(normal, viewDir), 0.0);
    float earthDepth = texelFetch(uEarthDepthTexture, ivec2(gl_FragCoord.xy), 0).r;
    float fragDepthInEarthNearFar = clamp(v2f_earthNdcZ * 0.5 + 0.5, 0.0, 1.0);
    if (fragDepthInEarthNearFar >= earthDepth)
        discard;
    float fragDepthInTotalNearFar = clamp(v2f_totalNdcZ * 0.5 + 0.5, 0.0000001, 0.9999999);
    gl_FragDepth = fragDepthInTotalNearFar;
    fragData = vec4(v2f_color.rgb, 0.5);
}