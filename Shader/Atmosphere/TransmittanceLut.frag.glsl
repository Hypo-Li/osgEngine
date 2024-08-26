#version 460 core
#extension GL_GOOGLE_include_directive : enable
in vec2 uv;
out vec4 fragData;
#define TRANSMITTANCE_LUT
#define BINDING_INDEX 1
#include "AtmosphereCommon.glsl"
#define LUT_WIDTH 256
#define LUT_HEIGHT 64

void main()
{
    float viewHeight, viewZenithCos;
    uvToTransmittanceLutParameters(uv, viewHeight, viewZenithCos);
    vec3 worldPos = vec3(0.0, 0.0, viewHeight);
    vec3 worldDir = vec3(0.0, sqrt(1.0 - viewZenithCos * viewZenithCos), viewZenithCos);
    vec3 transmittance = exp(-getOpticalDepth(worldPos, worldDir, 40.0));
    fragData = vec4(transmittance, 1.0);
}