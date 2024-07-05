#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 32, local_size_y = 27) in;
layout (r11f_g11f_b10f, binding = 0) uniform image2D uSkyViewLutImage;
uniform sampler2D uTransmittanceLutTexture;
uniform sampler2D uMultiScatteringLutTexture;
#define PLANET_TOP_AT_ABSOLUTE_WORLD_ORIGIN
layout(std140, binding = 0) uniform ViewData
{
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
};
#define SKY_VIEW_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define LUT_WIDTH 192
#define LUT_HEIGHT 108

void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(LUT_WIDTH, LUT_HEIGHT);
    vec3 worldPos = getWorldPos(uInverseViewMatrix[3].xyz);
    float viewHeight = max(length(worldPos), uGroundRadius + 0.005);
    float viewZenithCos, lightViewCos;
    uvToSkyViewLutParameters(viewHeight, uv, viewZenithCos, lightViewCos);
    vec3 upVector = vec3(worldPos / viewHeight);
    float sunZenithCos = dot(upVector, uSunDirection);
    vec3 sunDir = normalize(vec3(sqrt(max(1.0 - sunZenithCos * sunZenithCos, 0.0)), 0.0, sunZenithCos));
    worldPos = vec3(0.0, 0.0, viewHeight);
    float viewZenithSin = sqrt(max(1.0 - viewZenithCos * viewZenithCos, 0.0));
    vec3 worldDir;
    worldDir.x = viewZenithSin * lightViewCos;
    worldDir.y = viewZenithSin * sqrt(max(1.0 - lightViewCos * lightViewCos, 0.0));
    worldDir.z = viewZenithCos;
    if (!moveToTopAtmosphere(worldPos, worldDir))
    {
        imageStore(uSkyViewLutImage, ivec2(gl_GlobalInvocationID.xy), vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }
    vec3 lum, fms, trans;
    rayMarchAtmosphere(0.3, worldPos, worldDir, sunDir, 30.0, 9000000.0, lum, fms, trans);
    imageStore(uSkyViewLutImage, ivec2(gl_GlobalInvocationID.xy), vec4(lum, 1.0));
}