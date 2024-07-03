#version 460 core
#extension GL_GOOGLE_include_directive : enable
layout (local_size_x = 32, local_size_y = 32) in;
layout (rgba16f, binding = 0) uniform image2D uTransmittanceLutImage;
#define TRANSMITTANCE_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define LUT_WIDTH 256
#define LUT_HEIGHT 64

void main()
{
    vec2 uv = (gl_GlobalInvocationID.xy + 0.5) / vec2(LUT_WIDTH, LUT_HEIGHT);
    float viewHeight, viewZenithCos;
    uvToTransmittanceLutParameters(uv, viewHeight, viewZenithCos);
    vec3 worldPos = vec3(0.0, 0.0, viewHeight);
    vec3 worldDir = vec3(0.0, sqrt(1.0 - viewZenithCos * viewZenithCos), viewZenithCos);
    vec3 transmittance = exp(-getOpticalDepth(worldPos, worldDir, 40.0));
    imageStore(uTransmittanceLutImage, ivec2(gl_GlobalInvocationID.xy), vec4(transmittance, 1.0));
}